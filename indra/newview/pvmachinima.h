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
#include "llfloaterpreference.h"

/////////////////////////////
// Polarity Cinematic Mode //
/////////////////////////////
// Fixme: cannot be accessed from another file.

// This class can be used as a function to toggle Name Tags, User Interface, HUDs and voice dots as a whole
class PVMachinimaTools : public LLSingleton<PVMachinimaTools>
{
	LLSINGLETON_EMPTY_CTOR(PVMachinimaTools);
	// 0=Show all, 1= Hide all, 2= Dots only, 3=Waves Only
	static S32 previous_voice_dot_setting_;
	// 0=Hidden, 1=Visible, 2=Auto-hide
	static S32 previous_render_name_;
	// false=Show for all, true=Hide for all
	static bool previous_chat_anim_setting_;
	// Hover tips
	static bool previous_hovertips_setting_;
	static bool previous_show_typing_;
	static bool previous_hud_visibility;
	// Keeps track of the Cinematic Mode status.
	static bool cinematic_mode_enabled_;
public:
	static void toggleCinematicMode();
	// Wether or not we are in the Cinematic Mode.
	static bool isEnabled();
};


class PVMachinimaSidebar : public LLFloaterPreference, public LLSingleton<PVMachinimaTools>
{
	LLSINGLETON(PVMachinimaSidebar);
public:
	static bool isVisible(const LLSD& userdata);
};

#endif // PV_MACHINIMA_H
