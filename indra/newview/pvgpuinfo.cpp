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
	vram_free_ = S64Kilobytes(free_memory);
	if (!gGLManager.mIsIntel)
	{
		auto on_board = vRAMGetTotalOnboard();
		// yes, there's a reason to buy real GPUs; WORKING API.
		vram_used_total_ = on_board - vram_free_;
		//@todo make sure this is more or less accurate
		vram_used_by_others_ = on_board - vram_free_ - vram_used_by_viewer_;
	}
	gpuInfoRefreshTimer.reset();
}

S32Megabytes PVGPUInfo::vRAMGetTotalOnboard()
{
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO);
	LL_RECORD_BLOCK_TIME(FTM_PV_GPU_INFO_GET_VRAM);
	static const S64Megabytes MINIMUM_VRAM_AMOUNT = S64Megabytes(1024); // fallback for cases where video memory is not detected properly
	static S64Megabytes vram_s64_megabytes = S64Megabytes(gGLManager.mVRAM);
	if (gGLManager.mIsIntel)
	{
		// sometimes things can go wrong
		if (vram_s64_megabytes.valueInUnits<LLUnits::Megabits>() < 256)
		{
			return (S32Megabytes)256;
		}
	}
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
