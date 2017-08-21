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

static const S32 MINIMUM_FPS_LIMIT = 15;

class PVFPSMeter
{
public:
	static bool update();

	static std::string getValueWithRefreshRate();

	static std::string getValue();

	static LLColor4 getColor()
	{
		return mFPSMeterColor;
	}

	static bool start();
	static bool stop();

	static bool canUpdate();

	static void preComputeFloorAndCeiling();

private:
	static LLColor4			mFPSMeterColor;
	static LLFrameTimer		mTextFPSTimer;
	static F32				mFPSMeterValue;
	static U32				mFPSNullZoneVSyncLower;
	static U32				mFPSNullZoneTargetLower;
	static U32				mFPSNullZoneTarget;
	static U32				mFPSNullZoneVSync;
	static std::string		sLastFPSMeterString;

	static bool isCloseEnoughToTarget(const F32 value, const bool compare_with_refresh);
};
#endif // PV_FPS_METER_H
