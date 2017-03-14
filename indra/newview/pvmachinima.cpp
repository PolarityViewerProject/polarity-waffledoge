// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvmachinima.cpp
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

#include "llviewerprecompiledheaders.h"
#include "pvmachinima.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llviewerwindow.h"
#include "llmoveview.h"
#include "llagentcamera.h"
#include <llnotificationsutil.h>
#include "llchiclet.h"
#include "llchicletbar.h"
#include "llfloaterpreference.h" // for extra slider controls
#include "llvoavatar.h"

/////////////////////////////
// Polarity Cinematic Mode //
/////////////////////////////

bool PVMachinimaTools::cinematic_mode_enabled_ = false;

//static
bool PVMachinimaTools::isEnabled()
{
	LL_DEBUGS() << "returning cinematic_mode_enabled_=" << cinematic_mode_enabled_ << LL_ENDL;
	return cinematic_mode_enabled_;
}

void PVMachinimaTools::toggleCinematicMode()
{
	if(cinematic_mode_enabled_)
	{
		// TODO: Move to globals to avoid touching settings
		voice_indicator_variable_ = gSavedSettings.getControl("PVUI_VoiceIndicatorBehavior");
		hover_tips_variable_ = gSavedSettings.getControl("ShowHoverTips");

		cinematic_mode_enabled_ = false;
		LL_INFOS() << "Exiting Cinematic Mode" << LL_ENDL;

		 // TODO: use previous value instead of hard-coding these.
		gViewerWindow->setUIVisibility(true);
		LLChicletBar::getInstance()->showWellButton("notification_well", !cinematic_mode_enabled_);
		LLPanelStandStopFlying::getInstance()->setVisible(!cinematic_mode_enabled_); // FIXME: that doesn't always work

		LLPipeline::sShowHUDAttachments = previous_hud_visibility;
		LLVOAvatar::sRenderName = previous_render_name_;
		LLVOAvatar::sShowTyping = previous_show_typing_;
		voice_indicator_variable_->setValue(previous_voice_dot_setting_, false);
		hover_tips_variable_->setValue(previous_hovertips_setting_, false);

		return;
	}
	LL_INFOS() << "Entering Cinematic Mode" << LL_ENDL;
	// save user-configured value to restore it later.
	previous_voice_dot_setting_ = voice_indicator_variable_->getValue();
	previous_render_name_ = LLVOAvatar::sRenderName;
	previous_show_typing_ = LLVOAvatar::sShowTyping;
	previous_hovertips_setting_ = hover_tips_variable_->getValue();
	previous_hud_visibility = LLPipeline::sShowHUDAttachments;

	// ENABLE machinima mode:
	cinematic_mode_enabled_ = true;

	// Ordered to have a nice effect
	hover_tips_variable_->setValue(!cinematic_mode_enabled_, false);
	voice_indicator_variable_->setValue(static_cast<LLSD::Integer>(!cinematic_mode_enabled_), false);
	LLVOAvatar::sShowTyping = !cinematic_mode_enabled_;
	LLVOAvatar::sRenderName = LLVOAvatar::RENDER_NAME_NEVER;
	LLPipeline::sShowHUDAttachments = !cinematic_mode_enabled_;
	LLPanelStandStopFlying::getInstance()->setVisible(!cinematic_mode_enabled_); // FIXME: that doesn't always work
	LLChicletBar::getInstance()->showWellButton("notification_well", !cinematic_mode_enabled_);
	gViewerWindow->setUIVisibility(!cinematic_mode_enabled_);

	LL_DEBUGS() << "cinematic_mode_enabled_=" << cinematic_mode_enabled_ << LL_ENDL;
}

bool PVMachinimaTools::handleEvent(const LLSD& userdata)
{
	if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
	{
		// Do nothing
		return false;
	}
	
	LLNotification::Params params("CinematicConfirmHideUI");
	params.functor.function(boost::bind(&PVMachinimaTools::confirm, this, _1, _2));
	LLSD substitutions;
#if LL_DARWIN
	substitutions["SHORTCUT"] = "Ctrl+Alt+Shift+C";
#else
	substitutions["SHORTCUT"] = "Alt+Shift+C";
#endif
	params.substitutions = substitutions;
	if (!isEnabled())
	{
		// hiding, so show notification
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
	return true;
}

void PVMachinimaTools::confirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0) // OK
	{
		toggleCinematicMode();
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
