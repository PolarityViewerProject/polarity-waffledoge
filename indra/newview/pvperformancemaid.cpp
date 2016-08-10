/**
* @file pvperformancemaid.cpp
* @brief Performance utilities
*
* $LicenseInfo:firstyear=2016&license=viewerlgpl$
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

#include "pvperformancemaid.h"
#include "llviewercontrol.h"

void PVPerformanceMaid::TriggerPanicMode()
{
	auto pvMaid = getInstance();
	if (!pvMaid->is_in_panic_mode_())
	{
		LL_WARNS() << "User is in panic! Disabling fancy graphics!" << LL_ENDL;
		// TODO PLVR: Store the settings somewhere in the XML File to restore AFTER relog?
		// TODO PLVR: An even better way would be to temporarily disable the render features without touching the settings at all.
		pvMaid->previous_vertex_mode_ = gSavedSettings.getBOOL("VertexShaderEnable");
		pvMaid->previous_draw_distance_ = gSavedSettings.getBOOL("RenderFarClip");
		gSavedSettings.setBOOL("VertexShaderEnable", false);
		gSavedSettings.setF32("RenderFarClip", 5.f);
		pvMaid->in_panic_mode_ = true;
	}
	else
	{
		LL_WARNS() << "Restoring previous settings" << LL_ENDL;
		gSavedSettings.setF32("RenderFarClip", pvMaid->previous_draw_distance_);
		gSavedSettings.setBOOL("VertexShaderEnable", pvMaid->previous_vertex_mode_);
		pvMaid->in_panic_mode_ = true;
	}
}

bool PVPerformanceMaidMenuItemHandler::handleEvent(const LLSD& userdata)
{
	TriggerPanicMode();
	return true;
}
