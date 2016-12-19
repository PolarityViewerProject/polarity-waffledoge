/**
* @file FILE pvgpuinfo.h
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

#ifndef PV_GPUINFO_H
#define PV_GPUINFO_H

#pragma once

//#include "llerror.h"
#include "llsingleton.h"
#include "llunits.h"

class PVGPUInfo : public LLSingleton<PVGPUInfo>
{
	// @todo Write unit tests
public:

	/*
	* \brief Fetched "Free" memory. Should be entire black bar, or the on-board VRAM minus the other bars.
	*/
	static S32Megabytes getAvailableVRAM()
	{
		return vram_available_mb;
	}

	/*
	 * \brief Computed "In Use" memory. Should be the biggest non-free value and be the sum of all colored bars, included grey.
	 *  otherwise said, what's not free.
	 */
	static S32Megabytes geComputedNonFreeVRAM()
	{
		return vram_in_use_mb;
	}

	/*
	* \brief Computed "Our Own" memory. Should be the sum of the red, yellow and blue bars.
	*/
	static S32Megabytes getLocalUsedVRAM()
	{
		return vram_used_by_us_mb;
	}

	/*
	* \brief Computed "Other Programs" memory. Should be the grey bar.
	*/
	static S32Megabytes getReservedVRAM()
	{
		return vram_used_by_others_mb;
	}

	static bool hasEnoughVRAMForSnapshot(const S32 tentative_x, const S32 tentative_y);

	/**
	 * \brief Wrapper around gGLManager.mVRAM
	 * \return S32
	 */
	static S32Megabytes getTotalVRAM();

	
	static void updateValues();

	// Enables logging for this class
	typedef PVGPUInfo _LL_CLASS_TO_LOG;

private:
	/*
	* \brief Fetched "Free" memory. Should be entire black bar, or the on-board VRAM minus the other bars.
	*/
	static S32Megabytes vram_available_mb;
	/*
	* \brief Computed "In Use" memory. Should be the biggest non-free value and be the sum of all colored bars.
	*/
	static S32Megabytes vram_in_use_mb;
	/*
	* \brief Computed "Our Own" memory. Should be the sum of the red, yellow and blue bars.
	*/
	static S32Megabytes vram_used_by_us_mb;
	/*
	* \brief Computed "Other Programs" memory. Should be the grey bar.
	*/
	static S32Megabytes vram_used_by_others_mb;
};

#endif // PV_GPUINFO_H
