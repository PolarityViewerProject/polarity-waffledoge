/**
 * @file pvfpsmeter.h
 * @brief FPS meter logic (header)
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
#pragma once
#ifndef PV_FPS_METER_H
#define PV_FPS_METER_H
#include "v4color.h"
#include "llframetimer.h"

static const U32 MINIMUM_FPS_LIMIT = 15;

class PVFPSMeter
{
public:
	PVFPSMeter();
	static void setLimit(const F32& new_limit_f32);
	static void refresh();

	static std::string getValueWithRefreshRate();

	static F32 getValue()
	{
		return mFPSMeterValue;
	}

	static F32 getLimit()
	{
		return mFPSLimiterTarget;
	}

	static LLColor4 getColor()
	{
		return mFPSMeterColor;
	}

	static bool start();
	static bool stop();
	static bool validateFPSLimiterTarget();
	static bool getLimiterEnabled()
	{
		return mFPSLimiterTarget > MINIMUM_FPS_LIMIT && mFPSLimiterEnabled;
	}

private:
	static LLColor4			mFPSMeterColor;
	static LLFrameTimer		mStatusBarFPSCounterTimer;
	//static U32				mMonitorRefreshRate;
	static U32				mFPSLimiterTarget;
	static F32				mFPSMeterValue;
	static bool				mFPSLimiterEnabled;
};
#endif // PV_FPS_METER_H
