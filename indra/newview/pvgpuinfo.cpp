/**
* @file FILE pvgpuinfo.cpp
* @brief Class containing useful information about the Graphic processor in use
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
* Polarity Viewer Source Code
* Copyright (C) 2016 Xenhat Liamano
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* The Polarity Viewer Project
* http://www.polarityviewer.org
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h" // ewwww...
#include "llgl.h"
#include "pvgpuinfo.h"
#include "llrendertarget.h"
#include "llviewertexture.h"
#include "llmemory.h"

#define INTEL_GPU_MAX_VRAM 2048

S32Megabytes PVGPUInfo::vram_available_mb = S32Megabytes(0);
S32Megabytes PVGPUInfo::vram_in_use_mb = S32Megabytes(0);
S32Megabytes PVGPUInfo::vram_used_by_us_mb = S32Megabytes(0);
S32Megabytes PVGPUInfo::vram_used_by_others_mb = S32Megabytes(0);

void PVGPUInfo::updateValues()
{
	// @todo deduplicate calls to this and use value from this class across the rest of the viewer

	LLMemory::updateMemoryInfo();
	auto total_texture_mem = LLViewerTexture::sTotalTextureMemory.valueInUnits<LLUnits::Megabytes>();
	auto fbo = U32Bytes(LLRenderTarget::sBytesAllocated).valueInUnits<LLUnits::Megabytes>();
	S32Megabytes bound_mem = LLViewerTexture::sBoundTextureMemory;
	vram_used_by_us_mb = S32Megabytes(bound_mem.value() + total_texture_mem + fbo);
	
	GLint memInfo = 0; // in KB
	// Note: glGet* calls are slow. Instead consider using something like:
	//     INT  wglGetGPUInfoAMD(UINT id, INT property, GLenum dataType, UINT size, void *data);
	
	if (gGLManager.mIsNVIDIA)
	{
		// Only the NVIDIA driver can reliably know how much memory is in use,
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &memInfo);
	}
	else if (gGLManager.mIsATI /* && gGLManager.mHasATIMemInfo */) // still testing
	{
		// The AMD/ATI driver does not always report memory info, this appear to depend on both the hardware
		// and the driver. If no memory info available, let's assume there is no reserved vram.
		// glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &memInfoAMD);
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &memInfo);
		vram_available_mb = S32Kilobytes(memInfo);
	}
	vram_available_mb = S32Kilobytes(memInfo);

	// we really need these unit tests...
	// @note If someone manages to make better math, please contribute.
	
	if (!gGLManager.mIsIntel)
	{
		// yes, there's a reason to buy real GPUs; WORKING API.
		vram_in_use_mb = getTotalVRAM() - vram_available_mb;
		vram_used_by_others_mb = vram_in_use_mb - vram_used_by_us_mb;
	}
}

S32Megabytes PVGPUInfo::getTotalVRAM()
{

	if (!gGLManager.mIsIntel)
	{
		// return existing variable to avoid memory bloat
		return S32Megabytes(gGLManager.mVRAM);
	}

	// Intel driver seriously lacks any reporting capability aside "in use", in DirectX.
	// Method to obtain in use memory from OpenGL does not appear to exist or work right now.
	// Hotwiring max VRAM on Intel gpus to half the system memory.
	LLMemoryInfo gSysMemory;
	const S32 phys_mem_mb = gSysMemory.getPhysicalMemoryKB().valueInUnits<LLUnits::Megabytes>();
	const S32 intel_half_sysram_clamped = llmin(phys_mem_mb / 2, INTEL_GPU_MAX_VRAM); // Raise me when Intel iGPU is usable above 2GB

	return S32Megabytes(intel_half_sysram_clamped);
}

bool PVGPUInfo::hasEnoughVRAMForSnapshot(const S32 tentative_x, const S32 tentative_y)
{
	S32 tentative_pixel_count = tentative_x * tentative_y;
	if (tentative_pixel_count > (gGLManager.mGLMaxTextureSize * 2) || tentative_pixel_count > getAvailableVRAM().value())
	{
		// fallback to something
		LL_WARNS() << "Available VRAM is smaller than the requested texture size ( " << tentative_x << " x " << tentative_y << ")!" << LL_ENDL;
		return false;
	}

	return true;
}
