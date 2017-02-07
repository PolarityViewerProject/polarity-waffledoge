// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file ospanelquicksettingspulldown.cpp
 * @brief Quick Settings popdown panel
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Obsidian Viewer Source Code
 * Copyright (C) 2016, Drake Arconis.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "ospanelquicksettingspulldown.h"

#include "llframetimer.h"

const F32 AUTO_CLOSE_FADE_START_TIME_SEC = 4.f;
const F32 AUTO_CLOSE_TOTAL_TIME_SEC = 5.f;

///----------------------------------------------------------------------------
/// Class OSPanelQuickSettingsPulldown
///----------------------------------------------------------------------------

// Default constructor
OSPanelQuickSettingsPulldown::OSPanelQuickSettingsPulldown() : LLPanel()
{
	mHoverTimer.stop();
	buildFromFile("panel_quick_settings_pulldown.xml");
}

//virtual
void OSPanelQuickSettingsPulldown::draw()
{
	F32 alpha = mHoverTimer.getStarted()
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), AUTO_CLOSE_FADE_START_TIME_SEC, AUTO_CLOSE_TOTAL_TIME_SEC, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}

	LLPanel::draw();
}

/*virtual*/
void OSPanelQuickSettingsPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x, y, mask);
}

/*virtual*/
void OSPanelQuickSettingsPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x, y, mask);
}

/*virtual*/
void OSPanelQuickSettingsPulldown::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
void OSPanelQuickSettingsPulldown::onVisibilityChange(BOOL new_visibility)
{
	new_visibility ? mHoverTimer.start() : mHoverTimer.stop();
}
