/**
 * @file pvmachinima.h
 * @brief Machinima-related utilities
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

#ifndef PV_MACHINIMA_H
#define PV_MACHINIMA_H

#pragma once

#include "llmenugl.h"

/////////////////////////////
// Polarity Cinematic Mode //
/////////////////////////////
// Fixme: cannot be accessed from another file.

// This class can be used as a function to toggle Name Tags, User Interface, HUDs and voice dots as a whole
class PVCinematicMode : public view_listener_t,
                        public LLSingleton<PVCinematicMode> // This enables the use of ::instance() to call this function/class without a boost bind. - Xenhat 2015.09.22
{
	bool handleEvent(const LLSD& userdata) override;
	void confirm(const LLSD& notification, const LLSD& response);
protected:
	// 0=Show all, 1= Hide all, 2= Dots only, 3=Waves Only
	U32 previous_voice_dot_setting_ = 0;
	// 0=Hidden, 1=Visible, 2=Auto-hide
	S32 previous_name_tag_setting_ = 2;
	// false=Show for all, true=Hide for all
	bool previous_chat_anim_setting_ = false;
	// Hover tips
	bool previous_hovertips_setting_ = false;
public:
	bool previous_hud_visibility = false;
	// Keeps track of the Cinematic Mode status.
	bool cinematic_mode_ = false;
	// Wether or not we are in the Cinematic Mode.
	bool is_in_cinematic_mode_() const;
	void enter_cinematic_mode();
	void exit_cinematic_mode();
};


class PVMachinimaSidebar : public view_listener_t,
                           public LLSingleton<PVCinematicMode>
{
	bool handleEvent(const LLSD& userdata) override;
public:
	static bool isVisible(const LLSD& userdata);
};

#endif // PV_MACHINIMA_H
