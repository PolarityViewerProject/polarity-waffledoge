/**
* @file FILE pvgpuinfo.cpp
* @brief Class containing useful information about the Graphic processor in use
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
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
#include "llviewercontrol.h"

static const S64Megabytes INTEL_GPU_MAX_VRAM = S64Megabytes(2048);

S64Bytes PVGPUInfo::vram_free_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_total_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_by_viewer_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_by_others_ = S64Bytes(0);

S64Bytes PVGPUInfo::vram_bound_mem = S64Bytes(0);
S64Bytes PVGPUInfo::vram_max_bound_mem = S64Bytes(0);
S64Bytes PVGPUInfo::vram_total_mem = S64Bytes(0);
S64Bytes PVGPUInfo::vram_max_total_texture_mem = S64Bytes(0);
S64Bytes PVGPUInfo::vram_bar_fbo = S64Bytes(0);

void PVGPUInfo::updateValues()
{
	// @todo deduplicate calls to this and use value from this class across the rest of the viewer

	//LLMemory::updateMemoryInfo();
	vram_bar_fbo				= S64Bytes(LLRenderTarget::sBytesAllocated);
	vram_bound_mem 				= LLViewerTexture::sBoundTextureMemory;
	vram_max_bound_mem 			= S64Megabytes(LLViewerTexture::sMaxBoundTextureMemory.valueInUnits<LLUnits::Megabytes>());
	vram_max_total_texture_mem 	= LLViewerTexture::sMaxTotalTextureMem;
	vram_total_mem 				= LLViewerTexture::sTotalTextureMemory;

	// Don't count the FBO to see if this fixes the texture bar
	//vram_used_by_viewer_ = S64Bytes(vram_total_mem + vram_bar_fbo + vram_bound_mem);
	vram_used_by_viewer_ = S64Bytes(vram_total_mem + vram_bound_mem);
	
	GLint free_memory = 0; // in KB
	// Note: glGet* calls are slow. Instead consider using something like:
	//     INT  wglGetGPUInfoAMD(UINT id, INT property, GLenum dataType, UINT size, void *data);
	
	if (gGLManager.mIsNVIDIA)
	{
		// Only the NVIDIA driver can reliably know how much memory is in use,
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_memory);
	}
	else if (gGLManager.mIsATI /* && gGLManager.mHasATIMemInfo */) // still testing
	{
		// The AMD/ATI driver does not always report memory info, this appear to depend on both the hardware
		// and the driver. If no memory info available, let's assume there is no reserved vram.
		// glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &memInfoAMD);
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &free_memory);
	}
	vram_free_ = S64Kilobytes(free_memory);

	// we really need these unit tests...
	// @note If someone manages to make better math, please contribute.
	
	if (!gGLManager.mIsIntel)
	{
		auto on_board = vRAMGetTotalOnboard();
		// yes, there's a reason to buy real GPUs; WORKING API.
		vram_used_total_ = on_board - vram_free_;
		//@todo make sure this is more or less accurate
		vram_used_by_others_ = on_board - vram_free_ - vram_used_by_viewer_;
	}
}

S64Bytes PVGPUInfo::vRAMGetTotalOnboard()
{
	static const S64Megabytes MINIMUM_VRAM_AMOUNT = S64Megabytes(1024); // fallback for cases where video memory is not detected properly
	static S64Megabytes vram_s64_megabytes = S64Megabytes(gGLManager.mVRAM);
	if (!gGLManager.mIsIntel)
	{
		// Global catch-all in case shit goes left still...
		if (vram_s64_megabytes < MINIMUM_VRAM_AMOUNT)
		{
			LL_WARNS() << "VRAM amount not detected or less than " << MINIMUM_VRAM_AMOUNT << ", defaulting to " << MINIMUM_VRAM_AMOUNT << LL_ENDL;
			vram_s64_megabytes = MINIMUM_VRAM_AMOUNT;
			LL_DEBUGS() << "VRAM SUCESSFULLY OVERRIDED" << LL_ENDL;
		}
		// set internal vram value to forced one if present
		static LLCachedControl<S32> forced_vram(gSavedSettings, "PVDebug_ForcedVideoMemory");
		if (forced_vram > 0)
		{
			vram_s64_megabytes = S64Megabytes(forced_vram);
		}
		// return existing variable to avoid memory bloat
		if (gGLManager.mVRAM != (S32)vram_s64_megabytes.value())
		{
			gGLManager.mVRAM = S32(vram_s64_megabytes.value());
		}
		return vram_s64_megabytes;
	}

	// Intel driver seriously lacks any reporting capability aside "in use", in DirectX.
	// Method to obtain in use memory from OpenGL does not appear to exist or work right now.
	// Hotwiring max VRAM on Intel gpus to half the system memory.
	LLMemoryInfo gSysMemory;
	const S64Bytes phys_mem_mb = S64Bytes(gSysMemory.getPhysicalMemoryKB());
	const S64Bytes intel_half_sysram_clamped = llmin(S64Bytes(phys_mem_mb.value() / 2), S64Bytes(INTEL_GPU_MAX_VRAM)); // Raise me when Intel iGPU is usable above 2GB

	return S64Megabytes(intel_half_sysram_clamped);
}

bool PVGPUInfo::hasEnoughVRAMForSnapshot(const S32 tentative_x, const S32 tentative_y)
{
	S32 tentative_pixel_count = tentative_x * tentative_y;
	if (tentative_pixel_count > (gGLManager.mGLMaxTextureSize * 2) || tentative_pixel_count > vRAMGetFree().value())
	{
		// fallback to something
		LL_WARNS() << "Available VRAM is smaller than the requested texture size ( " << tentative_x << " x " << tentative_y << ")!" << LL_ENDL;
		return false;
	}

	return true;
}
