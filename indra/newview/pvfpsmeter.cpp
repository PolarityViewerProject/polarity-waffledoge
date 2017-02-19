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

// KNOWN BUG: FPS Limiter slider in preferences refuses to properly display the proper value on reset (see PVFPSMeter::setLimit(-1) behavior)

const S32 FRAME_NULL_ZONE = 1;

// Default values, because static and shenanigans
F32 PVFPSMeter::mFPSMeterValue(0.f);
S32 PVFPSMeter::mFPSLimiterTarget(0);
LLColor4 PVFPSMeter::mFPSMeterColor(LLColor4::white);
LLFrameTimer PVFPSMeter::mStatusBarFPSCounterTimer = LLFrameTimer(); // IF there is a better way, please enlighten me.
bool PVFPSMeter::mFPSLimiterEnabled(0);

bool PVFPSMeter::start()
{
	if (mStatusBarFPSCounterTimer.getStarted())
	{
		return false;
	}

	// Still doesn't appear to really work yet...
	enableLimiter(gSavedSettings.getBOOL("PVRender_FPSLimiterEnabled"));
	setLimit(gSavedSettings.getS32("PVRender_FPSLimiterTarget"));

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

bool PVFPSMeter::update()
{
	if (!mStatusBarFPSCounterTimer.getStarted())
	{
		//llassert(mStatusBarFPSCounterTimer.getStarted());
		return false;
	}

	// Temporary workaround until I figure out why settings are wrong at startup
	//static LLCachedControl<bool> fps_limiter_enabled(gSavedSettings, "PVRender_FPSLimiterEnabled");
	//static LLCachedControl<S32> fps_limiter_target(gSavedSettings, "PVRender_FPSLimiterTarget");
	//enableLimiter(fps_limiter_enabled);
	//setLimit(fps_limiter_target);
	// </workaround>

	// TODO: boost callback instaid of cachedcontrol check?
	static LLCachedControl<bool> fps_counter_visible(gSavedSettings, "PVUI_StatusBarShowFPSCounter");
	if (fps_counter_visible)
	{
		// Throttle a bit to avoid making faster FPS heavier to process
		if (mStatusBarFPSCounterTimer.getElapsedTimeF32() > 0.25)
		{// Quick and Dirty FPS counter colors. Idea of NiranV Dean, from comments and leftover code in Nirans Viewer.
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
			auto frame_recording = LLTrace::get_frame_recording();  // capture sample of the frame recording, I think.
			static F32 current_fps_sampled;
			current_fps_sampled = frame_recording.getPeriodMeanPerSec(LLStatViewer::FPS, 2);

			// Update the values
			mFPSMeterValue = frame_recording.getPeriodMeanPerSec(LLStatViewer::FPS); // current fps showed to the user

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
			if (mFPSLimiterEnabled && (mFPSMeterValue <= (mFPSLimiterTarget + FRAME_NULL_ZONE) && mFPSMeterValue >= (mFPSLimiterTarget - FRAME_NULL_ZONE))
				//&& mFPSMeterColor != color_limited
				)
			{
				mFPSMeterColor = color_limited;
			}
			else if ((vsync_mode == 1 || vsync_mode == 2) && (mFPSMeterValue <= (LLWindowWin32::getRefreshRate() + FRAME_NULL_ZONE) && mFPSMeterValue >= (LLWindowWin32::getRefreshRate() - FRAME_NULL_ZONE))
				//&& mFPSMeterColor != color_vsync
				)
			{
				mFPSMeterColor = color_vsync;
			}
			else if (current_fps_sampled <= fps_critical
				//&& mFPSMeterColor != color_critical
				)
			{
				mFPSMeterColor = color_critical;
			}
			else if (current_fps_sampled >= fps_critical && current_fps_sampled < fps_medium
				//&& mFPSMeterColor != color_low
				)
			{
				mFPSMeterColor = color_low;
			}
			else if (current_fps_sampled >= fps_low && current_fps_sampled < fps_high
				//&& mFPSMeterColor != color_medium
				)
			{
				mFPSMeterColor = color_medium;
			}
			else if (current_fps_sampled >= fps_medium && current_fps_sampled < fps_outstanding
				//&& mFPSMeterColor != color_high
				)
			{
				mFPSMeterColor = color_high;
			}
			else if (current_fps_sampled >= fps_outstanding
				//&& mFPSMeterColor != color_outstanding
				)
			{
				mFPSMeterColor = color_outstanding;
			}
			else /* if (mFPSMeterColor != color_fps_default) */
			{
				// all else fails, fallback to default color to prevent blackness
				mFPSMeterColor = color_fps_default;
			}
			mStatusBarFPSCounterTimer.reset(); // Reset the FPS timer so that we can count again
		}
	}
	return mFPSLimiterEnabled && (mFPSLimiterTarget != 0);
}
std::string PVFPSMeter::getValueWithRefreshRate()
{
	// Cap the amount of decimals we return
	auto decimal_precision = "%.2f";
	if (mFPSMeterValue > 100.f)
	{
		decimal_precision = "%.0f";
	}
	// else
	if (mFPSMeterValue > 10.f)
	{
		decimal_precision = "%.1f";
	}
	// else
	return (llformat(decimal_precision, mFPSMeterValue) + "/" + std::to_string(LLWindowWin32::getRefreshRate()));
}

void PVFPSMeter::setLimit(const S32& new_limit)
{
	if (new_limit >= 0)
	{
		mFPSLimiterTarget = new_limit;
	}
	else
	{
		mFPSLimiterTarget = LLWindowWin32::getRefreshRate();
		// fix up FPS setting to avoid looping here and also breaking the slider
		gSavedSettings.setS32("PVRender_FPSLimiterTarget", mFPSLimiterTarget);
	}
}
