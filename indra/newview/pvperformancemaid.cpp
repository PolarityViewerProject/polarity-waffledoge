// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvperformancemaid.cpp
 * @brief Runtime performance maintenance utilities
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

#include "pvperformancemaid.h"
#include "llviewercontrol.h"
#include "pipeline.h"

void PVPerformanceMaid::TriggerPanicMode()
{
	if (!PVPerformanceMaid::instance().is_in_panic_mode_())
	{
		LL_WARNS() << "User is in panic! Disabling fancy graphics!" << LL_ENDL;
		// TODO PLVR: Store the settings somewhere in the XML File to restore AFTER relog?
		// TODO PLVR: An even better way would be to temporarily disable the render features without touching the settings at all.
		// TODO PLVR: We NEED to ensure that all checks to these settings are done against the runtime version when applicable.
		// Not saved to settings all that often, so get directly from the pipeline
		PVPerformanceMaid::instance().previous_vertex_mode_ = LLPipeline::VertexShaderEnable;
		PVPerformanceMaid::instance().previous_draw_distance_ = LLPipeline::RenderFarClip;
		gSavedSettings.setBOOL("VertexShaderEnable", false);
		gSavedSettings.setF32("RenderFarClip", 25.f);
		PVPerformanceMaid::instance().in_panic_mode_ = true;
	}
	else
	{
		LL_WARNS() << "Restoring previous settings" << LL_ENDL;
		gSavedSettings.setF32("RenderFarClip", PVPerformanceMaid::instance().previous_draw_distance_);
		gSavedSettings.setBOOL("VertexShaderEnable", PVPerformanceMaid::instance().previous_vertex_mode_);
		PVPerformanceMaid::instance().in_panic_mode_ = false;
	}
}

bool PVPerformanceMaidPanicButton::handleEvent(const LLSD& userdata)
{
	TriggerPanicMode();
	return true;
}
