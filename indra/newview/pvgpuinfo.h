/**
* @file FILE pvgpuinfo.h
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

#ifndef PV_GPUINFO_H
#define PV_GPUINFO_H

#pragma once

//#include "llerror.h"
#include "llsingleton.h"
#include "llunits.h"

// fallback for cases where video memory is not detected properly
static const S64Bytes HW_MIN_VRAM_AMNT = S64Megabytes(32);

class PVGPUInfo : public LLSingleton<PVGPUInfo>
{
	LLSINGLETON(PVGPUInfo);

	/*
	* \brief Fetched "Free" memory. Should be entire black bar, or the on-board VRAM minus the other bars.
	*/
	static S64Bytes vram_free_;

	/*
	* \brief Computed "In Use" memory. Should be the biggest non-free value and be the sum of all colored bars.
	*/
	static S64Bytes vram_used_total_;
	/*
	* \brief Computed "Other Programs" memory. Should be the grey bar.
	*/
	static S64Bytes vram_used_by_others_;

	/*
	* \brief Computed "On-Board" memory. Should be the total range of the bar.
	*/
	static S64Bytes vram_on_board_;

	static LLFrameTimer gpuInfoRefreshTimer;
public:

	/*
	* \brief Computed "Our Own" memory. Should be the sum of the red, yellow and blue bars.
	*/
	static S64Bytes vram_used_by_viewer_;

	/*
	* \brief Fetched "Free" memory. Should be entire black bar, or the on-board VRAM minus the other bars.
	*/
	static S64Bytes vRAMGetFree()
	{
		return llmax((S64Bytes)0, vram_free_);
	}

	/*
	 * \brief Computed "In Use" memory. Should be the biggest non-free value and be the sum of all colored bars, included grey.
	 *  otherwise said, what's not free.
	 */
	static S64Bytes vRAMGetUsedTotal()
	{
		return llmax((S64Bytes)0, vram_used_total_);
	}

	/*
	* \brief Computed "Our Own" memory. Should be the sum of the red, yellow and blue bars.
	*/
	static S64Bytes vRAMGetUsedViewer()
	{
		return llmax((S64Bytes)0, vram_used_by_viewer_);
	}

	/*
	* \brief Computed "Other Programs" memory. Should be the grey bar.
	*/
	static S64Bytes vRAMGetUsedOthers()
	{
		return llmax((S64Bytes)0, vram_used_by_others_);
	}

	static bool hasEnoughVRAMForSnapshot(const S32 tentative_x, const S32 tentative_y);

	/**
	 * \brief proprietary API-provided replacement for GLManager::mVRAM
	 * \return S32Megabytes
	 */
	// TODO: Do something about gGLManager.mVRAM being a duplicate of this.
	static S64Bytes vRAMGetTotalOnboard()
	{
		return llmax(HW_MIN_VRAM_AMNT,vram_on_board_);
	}
	
	static S64Bytes computeOnboardVRAM();

	static void updateValues();

	// Enables logging for this class
	typedef PVGPUInfo _LL_CLASS_TO_LOG;
};

#endif // PV_GPUINFO_H
