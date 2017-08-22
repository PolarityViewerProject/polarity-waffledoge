// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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
#include "llmemory.h"
#include "llviewercontrol.h"

static const S64Megabytes INTEL_GPU_MAX_VRAM = S64Megabytes(2048);

S64Bytes PVGPUInfo::vram_free_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_total_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_by_others_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_used_by_viewer_ = S64Bytes(0);
S64Bytes PVGPUInfo::vram_on_board_ = S64Bytes(0);
LLFrameTimer PVGPUInfo::gpuInfoRefreshTimer = LLFrameTimer();

static LLTrace::BlockTimerStatHandle FTM_PV_GPU_INFO("!PVGPUInfo");
static LLTrace::BlockTimerStatHandle FTM_PV_GPU_INFO_UPDATE("updateValues");
static LLTrace::BlockTimerStatHandle FTM_PV_GPU_INFO_GET_VRAM("getTotalOnboard");

void PVGPUInfo::updateValues()
{
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO);
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO_UPDATE);
	if (gpuInfoRefreshTimer.getStarted())
	{
		if (gpuInfoRefreshTimer.getElapsedSeconds() <= 2)
		{
			return;
		}
	}
	else
	{
		gpuInfoRefreshTimer.start();
	}

	vram_used_by_viewer_ = LLViewerTexture::sTotalTextureMemory + LLViewerTexture::sBoundTextureMemory;

	/// Vendor-Dependant functions.
	GLint free_memory = 0; // in KB
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
	else
	{
		// TODO: Learn how to use the Intel driver API. Microsoft could, can't be that hard!
	}
	vram_free_ = S64Kilobytes(free_memory);
	//if (!gGLManager.mIsIntel)
	{
		vram_used_total_ = vram_on_board_ - vram_free_;
		//@todo make sure this is more or less accurate
		vram_used_by_others_ = vram_on_board_ - vram_free_ - vram_used_by_viewer_;
	}
	gpuInfoRefreshTimer.reset();
}

S64Bytes PVGPUInfo::computeOnboardVRAM()
{
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO);
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO_GET_VRAM);
	// set internal vram value to forced one if present
	static S64Bytes vram_s64B = llmax(HW_MIN_VRAM_AMNT,S64Bytes(S64Megabytes(gSavedSettings.getS32("PVDebug_ForcedVideoMemory"))));
	if(vram_s64B == HW_MIN_VRAM_AMNT)
	{
		vram_s64B = llmax(HW_MIN_VRAM_AMNT, S64Bytes(S64Megabytes(gGLManager.mVRAM)));
	}
	
	if (gGLManager.mIsIntel)
	{
		// Intel driver seriously lacks any reporting capability aside "in use", in DirectX.
		// Method to obtain in use memory from OpenGL does not appear to exist or work right now.
		// Hotwiring max VRAM on Intel gpus to half the system memory.
		LLMemoryInfo gSysMemory;
		static const S64Bytes phys_mem_mb = S64Bytes(gSysMemory.getPhysicalMemoryKB());
		// Notify me me when Intel iGPU can use more than 2GB without becoming unbearably slow - Xenhat
		vram_s64B = llmin(S64Bytes(gSysMemory.getPhysicalMemoryKB() * 0.5f), S64Bytes(INTEL_GPU_MAX_VRAM));
	}
	LL_DEBUGS() << "Computed On-Board VRAM: " << vram_s64B << LL_ENDL;
	vram_on_board_ = vram_s64B;
	return vram_s64B;
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
