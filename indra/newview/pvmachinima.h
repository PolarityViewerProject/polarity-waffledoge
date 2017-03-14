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
class PVMachinimaTools : public view_listener_t,
                        public LLSingleton<PVMachinimaTools> // This enables the use of ::instance() to call this function/class without a boost bind. - Xenhat 2015.09.22
{
	LLSINGLETON(PVMachinimaTools);
	bool handleEvent(const LLSD& userdata) override;
	void confirm(const LLSD& notification, const LLSD& response);
protected:
	LLPointer<LLControlVariable> voice_indicator_variable_;
	LLPointer<LLControlVariable> name_tag_mode_variable_;
	LLPointer<LLControlVariable> hover_tips_variable_;
	// 0=Show all, 1= Hide all, 2= Dots only, 3=Waves Only
	S32 previous_voice_dot_setting_ = 0;
	// 0=Hidden, 1=Visible, 2=Auto-hide
	S32 previous_render_name_ = 2;
	// false=Show for all, true=Hide for all
	bool previous_chat_anim_setting_ = false;
	// Hover tips
	bool previous_hovertips_setting_ = false;
	bool previous_show_typing_ = false;
public:
	void toggleCinematicMode();

	bool previous_hud_visibility = false;
	// Keeps track of the Cinematic Mode status.
	static bool cinematic_mode_enabled_;
	// Wether or not we are in the Cinematic Mode.
	static bool isEnabled();
};


class PVMachinimaSidebar : public view_listener_t,
                           public LLSingleton<PVMachinimaTools>
{
	LLSINGLETON(PVMachinimaSidebar);
	bool handleEvent(const LLSD& userdata) override;
public:
	static bool isVisible(const LLSD& userdata);
};

#endif // PV_MACHINIMA_H
