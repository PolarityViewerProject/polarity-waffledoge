/**
 * @file pvmachinima.cpp
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

#include "llviewerprecompiledheaders.h"
#include "pvmachinima.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llviewerwindow.h"
#include "llmoveview.h"
#include "llagentcamera.h"
#include <llnotificationsutil.h>

/////////////////////////////
// Polarity Cinematic Mode //
/////////////////////////////

// Wether or not we are in the Cinematic Mode.
bool PVCinematicMode::is_in_cinematic_mode_() const
{
	LL_DEBUGS() << "returning cinematic_mode_=" << cinematic_mode_ << LL_ENDL;
	return cinematic_mode_;
}

void PVCinematicMode::enter_cinematic_mode()
{
	LL_INFOS() << "Entering Cinematic Mode" << LL_ENDL;
	// save user-configured value to restore it later.
	previous_voice_dot_setting_ = gSavedSettings.getU32("PVUI_VoiceIndicatorBehavior");
	previous_name_tag_setting_ = gSavedSettings.getS32("AvatarNameTagMode");
	previous_hovertips_setting_ = gSavedSettings.getBOOL("ShowHoverTips");
	// Hide stuff
	gSavedSettings.setU32("PVUI_VoiceIndicatorBehavior", 1);
	gSavedSettings.setS32("AvatarNameTagMode", 0);
	gSavedSettings.setBOOL("PVChat_HideTypingForAll", true);
	gSavedSettings.setBOOL("ShowHoverTips", false);
	previous_hud_visibility = LLPipeline::sShowHUDAttachments;
	// Cinematic HIDES elements, so we set elements to NOT VISIBLE when machinima mode is ON
	gViewerWindow->setUIVisibility(false); // Show/hide Interface
	LLPanelStandStopFlying::getInstance()->setVisible(false);
	LLPipeline::sShowHUDAttachments = false;
	// Sanity Check
	cinematic_mode_ = true;
	LL_DEBUGS() << "cinematic_mode_=" << cinematic_mode_ << LL_ENDL;
}

void PVCinematicMode::exit_cinematic_mode()
{
	LL_INFOS() << "Exiting Cinematic Mode" << LL_ENDL;
	gViewerWindow->setUIVisibility(true); // Show/hide Interface
	LLPanelStandStopFlying::getInstance()->setVisible(true);
	LLPipeline::sShowHUDAttachments = previous_hud_visibility;
	// restore user-configured values
	gSavedSettings.setU32("PVUI_VoiceIndicatorBehavior", previous_voice_dot_setting_);
	gSavedSettings.setS32("AvatarNameTagMode", previous_name_tag_setting_);
	gSavedSettings.setBOOL("PVChat_HideTypingForAll", previous_name_tag_setting_);
	gSavedSettings.setBOOL("ShowHoverTips", previous_hovertips_setting_);
	// Sanity Check
	cinematic_mode_ = false;
	LL_DEBUGS() << "cinematic_mode_=" << cinematic_mode_ << LL_ENDL;
}

bool PVCinematicMode::handleEvent(const LLSD& userdata)
{
	if (gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
	{
		LLNotification::Params params("CinematicConfirmHideUI");
		params.functor.function(boost::bind(&PVCinematicMode::confirm, this, _1, _2));
		LLSD substitutions;
#if LL_DARWIN
		substitutions["SHORTCUT"] = "Ctrl+Alt+Shift+C";
#else
		substitutions["SHORTCUT"] = "Alt+Shift+C";
#endif
		params.substitutions = substitutions;
		if (!is_in_cinematic_mode_())
		{
			// hiding, so show notification
			LLNotifications::instance().add(params);
		}
		else
		{
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return true;
}

void PVCinematicMode::confirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0) // OK
	{
		if (!cinematic_mode_)
		{
			// Not in Cinematic mode already
			enter_cinematic_mode();
		}
		else
		{
			// Is already in machinima mode, unhide stuff
			exit_cinematic_mode();
		}
	}
}


bool PVMachinimaSidebar::handleEvent(const LLSD& userdata)
{
	static LLCachedControl<bool> sidebar_visible(gSavedSettings, "PVUI_MachinimaSidebar", false);
	gSavedSettings.setBOOL("PVUI_MachinimaSidebar",!sidebar_visible);
	return true;
}

bool PVMachinimaSidebar::isVisible(const LLSD& userdata)
{
	static LLCachedControl<bool> sidebar_visible(gSavedSettings, "PVUI_MachinimaSidebar", false);
	return sidebar_visible;
}
