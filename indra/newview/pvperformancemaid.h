/**
 * @file pvperformancemaid.h
 * @brief Runtime performance maintenance utilities
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

#ifndef PV_PERFORMANCEMAID_H
#define PV_PERFORMANCEMAID_H

#pragma once

#include "llmenugl.h"

///////////////////////////////
// Polarity Performance Maid //
///////////////////////////////
class PVPerformanceMaid : public LLSingleton<PVPerformanceMaid>
{
	bool in_panic_mode_;
	// settings backup.
	// TODO PLVR Use a class maybe?

	F32 previous_draw_distance_;
	BOOL previous_vertex_mode_;
	// TODO: Add automute settings backup to royally cripple everything.

public:
	bool is_in_panic_mode_() const
	{
		return in_panic_mode_;
	}

	static void TriggerPanicMode();
};

class PVPerformanceMaidPanicButton : public view_listener_t,
                                     public PVPerformanceMaid
{
	bool handleEvent(const LLSD& userdata) override;
};

#endif // PV_PERFORMANCEMAID_H
