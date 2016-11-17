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

S32Megabytes PVGPUInfo::vram_available_mb = S32Megabytes(0);
S32Megabytes PVGPUInfo::vram_in_use_mb = S32Megabytes(0);

void PVGPUInfo::updateValues()
{
	GLint memInfo; // in KB
	if (gGLManager.mIsATI)
	{
		glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &memInfo);
		vram_available_mb = S32Kilobytes(memInfo);
	}
	else if (gGLManager.mIsNVIDIA)
	{
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &memInfo);
		vram_available_mb = S32Kilobytes(memInfo);
	}
	else
	{
		// The Intel driver cannot reliably know how much memory is in use,
		// let's assume there is no free vram and skip drawing the grey bar.
		vram_available_mb = S32Kilobytes(0);
	}
	vram_in_use_mb = S32Megabytes(gGLManager.mVRAM - vram_available_mb.value());
	//LL_DEBUGS() << "vram_in_use_mb " << vram_in_use_mb << "\n" << "gGLManager.mVRAM " << gGLManager.mVRAM << "\n" << "vram_available_mb " << vram_available_mb << LL_ENDL;
}

S32Megabytes PVGPUInfo::getTotalVRAM()
{
	// return existing variable to avoid memory bloat
	return S32Megabytes(gGLManager.mVRAM);
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
