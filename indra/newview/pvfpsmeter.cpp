// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvfpsmeter.cpp
 * @brief FPS meter logic (source)
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2017 Xenhat Liamano
 * Portions Copyright (C)
 *  2014 NiranV Dean
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
#include "llviewerprecompiledheaders.h"
#include "pvfpsmeter.h"
#include "lltracerecording.h"
#include "llstartup.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include "llviewercontrol.h"
#include "llwindowwin32.h"
#include "llstatbar.h"

constexpr U32 FRAME_NULL_ZONE = 1;

static LLTrace::BlockTimerStatHandle FTM_PV_FPS_METER("!PVFPSMeter");
static LLTrace::BlockTimerStatHandle FTM_PV_FPS_METER_UPDATE("Update");
static LLTrace::BlockTimerStatHandle FTM_PV_FPS_METER_GET_VAL("getValue");

// Default values, because static and shenanigans
U32 PVFPSMeter::mFPSNullZoneVSyncLower(0);
U32 PVFPSMeter::mFPSNullZoneTargetLower(0);
U32 PVFPSMeter::mFPSNullZoneTarget(0);
U32 PVFPSMeter::mFPSNullZoneVSync(0);
F32 PVFPSMeter::mFPSMeterValue(0.f);
bool PVFPSMeter::mFPSDirty(false);
LLColor4 PVFPSMeter::mFPSMeterColor(LLColor4::white);
LLFrameTimer PVFPSMeter::mStatusBarFPSCounterTimer = LLFrameTimer(); // IF there is a better way, please enlighten me.
std::string PVFPSMeter::sLastFPSMeterString("");

bool PVFPSMeter::start()
{
	if (mStatusBarFPSCounterTimer.getStarted())
	{
		return false;
	}
	mStatusBarFPSCounterTimer.start();

	return true;
}

bool PVFPSMeter::stop()
{
	if (!mStatusBarFPSCounterTimer.getStarted())
	{
		return false;
	}
	mStatusBarFPSCounterTimer.stop();
	return true;
}

bool PVFPSMeter::canUpdate()
{
	llassert(mStatusBarFPSCounterTimer.getStarted());
	mFPSDirty = (mStatusBarFPSCounterTimer.getElapsedTimeF32() > 1.f);
	return mFPSDirty;
}

void PVFPSMeter::preComputeFloorAndCeiling()
{
	const U32 target = gSavedSettings.getU32("PVRender_FPSLimiterTarget");
	// TODO: Handle refresh rate change during application runtime. Currently not supported.
	const U32 refresh_rate = LLWindowWin32::getRefreshRate();
	U32 mFPSNullZoneTargetUpper =  target + FRAME_NULL_ZONE;
	U32 mFPSNullZoneVSyncUpper =  refresh_rate + FRAME_NULL_ZONE;
	mFPSNullZoneTargetLower =  target - FRAME_NULL_ZONE;
	mFPSNullZoneVSyncLower =  refresh_rate - FRAME_NULL_ZONE;
	mFPSNullZoneTarget =  mFPSNullZoneTargetUpper - mFPSNullZoneTargetLower + 1;
	mFPSNullZoneVSync =  mFPSNullZoneVSyncUpper - mFPSNullZoneVSyncLower + 1;
}

bool PVFPSMeter::isCloseEnoughToTarget(const F32 value, const bool compare_with_refresh)
{
	if(compare_with_refresh)
	{
		return ((unsigned)(ll_round(value) - mFPSNullZoneVSyncLower) < mFPSNullZoneVSync);
	}
	return ((unsigned)(ll_round(value) - mFPSNullZoneTargetLower) < mFPSNullZoneTarget);
}

bool PVFPSMeter::update()
{
	LL_RECORD_BLOCK_TIME(FTM_PV_FPS_METER);
	static LLCachedControl<bool> fps_limiter_enabled(gSavedSettings, "PVRender_FPSLimiterEnabled");
	if (canUpdate())
	{
		static LLCachedControl<U32> fps_limiter_target(gSavedSettings, "PVRender_FPSLimiterTarget");
		// TODO: boost callback instaid of cachedcontrol check?
		static LLCachedControl<bool> fps_counter_visible(gSavedSettings, "PVUI_StatusBarShowFPSCounter");
		if (fps_counter_visible)
		{
			// Quick and Dirty FPS counter colors. Idea of NiranV Dean, from comments and leftover code in Nirans Viewer.
			static auto color_fps_default = LLUIColorTable::instance().getColor("EmphasisColor");
			static auto color_critical = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Critical", LLColor4::red);
			static auto color_low = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Low", LLColor4::orange);
			static auto color_medium = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Medium", LLColor4::yellow);
			static auto color_high = LLUIColorTable::instance().getColor("PVUI_FPSCounter_High", LLColor4::green);
			static auto color_outstanding = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Outstanding", LLColor4::cyan);
			static auto color_vsync = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Vsync", LLColor4::blue2);
			static auto color_limited = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Limited", LLColor4::purple);
			static LLCachedControl<U32> fps_critical(gSavedSettings, "PVUI_FPSCounter_Critical", 10);
			static LLCachedControl<U32> fps_low(gSavedSettings, "PVUI_FPSCounter_Low", 20);
			static LLCachedControl<U32> fps_medium(gSavedSettings, "PVUI_FPSCounter_Medium", 40);
			static LLCachedControl<U32> fps_high(gSavedSettings, "PVUI_FPSCounter_High", 50);
			static LLCachedControl<U32> fps_outstanding(gSavedSettings, "PVUI_FPSCounter_Outstanding", 120);

			// Update the FPS count value from the statistics system (This is the normalized value, like in the statisics floater)
			mFPSMeterValue = LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::FPS);

			// TODO: Add a "status indicator" textbox or two somewhere in the top bar AND the statistics floater
			// to show vsync'd and limited statuses.
			// e.g.
			//_______________________________
			// FPS Limited Vsync          72 |
			//-------------------------------|
			// FPS BAR HERE .    | .        ||
			// FPS BAR HERE  .   |     .    ||
			// FPS BAR HERE    . |  .       ||
			//­­­­­­-------------------------------|
			// ~/~
			static LLCachedControl<U32> vsync_mode(gSavedSettings, "PVRender_VsyncMode");
			if (fps_limiter_enabled && isCloseEnoughToTarget(mFPSMeterValue, false))
			{
				mFPSMeterColor = color_limited;
			}
			else if (vsync_mode && isCloseEnoughToTarget(mFPSMeterValue, true))
			{
				mFPSMeterColor = color_vsync;
			}
			else if (mFPSMeterValue <= fps_critical)
			{
				mFPSMeterColor = color_critical;
			}
			else if (mFPSMeterValue >= fps_critical && mFPSMeterValue < fps_medium)
			{
				mFPSMeterColor = color_low;
			}
			else if (mFPSMeterValue >= fps_low && mFPSMeterValue < fps_high)
			{
				mFPSMeterColor = color_medium;
			}
			else if (mFPSMeterValue >= fps_medium && mFPSMeterValue < fps_outstanding)
			{
				mFPSMeterColor = color_high;
			}
			else if (mFPSMeterValue >= fps_outstanding)
			{
				mFPSMeterColor = color_outstanding;
			}
			else
			{
				// all else fails, fallback to default color to prevent blackness
				mFPSMeterColor = color_fps_default;
			}
			mStatusBarFPSCounterTimer.reset(); // Reset the FPS timer so that we can count again
		}
	}
	return fps_limiter_enabled;
}

// Cap the amount of decimals we return
const char * getAutomaticPrecision(const F32& fps_in)
{
	static const char * decimal_precision_0 = "%.0f";
	static const char * decimal_precision_1 = "%.1f";
	static const char * decimal_precision_2 = "%.2f";
	if (fps_in >= 100.f)
	{
		return decimal_precision_0;
	}
	if (fps_in >= 10.f)
	{
		return decimal_precision_1;
	}
	return decimal_precision_2;
}

std::string PVFPSMeter::getValueWithRefreshRate()
{
	LL_RECORD_BLOCK_TIME(FTM_PV_FPS_METER_GET_VAL);
	if(mFPSDirty)
	{
		sLastFPSMeterString = (llformat(getAutomaticPrecision(mFPSMeterValue), mFPSMeterValue) + "/" + std::to_string(LLWindowWin32::getRefreshRate()));
	}
	return sLastFPSMeterString;
}
