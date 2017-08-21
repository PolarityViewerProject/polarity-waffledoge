/**
 * @file pvmachinima.h
 * @brief Machinima-related utilities
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

#ifndef PV_MACHINIMA_H
#define PV_MACHINIMA_H

#pragma once

#include "llmenugl.h"

/////////////////////////////
// Polarity Cinematic Mode //
/////////////////////////////
// Fixme: cannot be accessed from another file.

// This class can be used as a function to toggle Name Tags, User Interface, HUDs and voice dots as a whole
class PVMachinimaTools : public LLSingleton<PVMachinimaTools>
{
	LLSINGLETON_EMPTY_CTOR(PVMachinimaTools);
	// Keeps track of the Cinematic Mode status.
	static bool cinematic_mode_enabled_;
public:
	static void toggleCinematicMode();
	// Wether or not we are in the Cinematic Mode.
	static bool isEnabled();
};


class PVMachinimaSidebar : public LLSingleton<PVMachinimaTools>
{
	LLSINGLETON_EMPTY_CTOR(PVMachinimaSidebar);
public:
	static bool isVisible(const LLSD& userdata);
};

#endif // PV_MACHINIMA_H
