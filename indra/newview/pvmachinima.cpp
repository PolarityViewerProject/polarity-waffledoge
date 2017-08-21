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
	LL_INFOS() << "Toggling Cinematic Mode" << LL_ENDL;
	// Ordered to have a nice effect
	LLPanelStandStopFlying::getInstance()->setVisible(cinematic_mode_enabled_); // FIXME: that doesn't always work
	LLChicletBar::getInstance()->showWellButton("notification_well", cinematic_mode_enabled_);
	gViewerWindow->setUIVisibility(cinematic_mode_enabled_);
	// ENABLE machinima mode:
	cinematic_mode_enabled_ = !cinematic_mode_enabled_;
}

bool PVMachinimaSidebar::isVisible(const LLSD& userdata)
{
	return gSavedSettings.getBool("PVUI_MachinimaSidebar");
}
