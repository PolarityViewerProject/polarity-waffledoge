/**
 * @file ospanelquicksettingspulldown.h
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

#ifndef OS_OSPANELQUICKSETTINGSPULLDOWN_H
#define OS_OSPANELQUICKSETTINGSPULLDOWN_H

#include "llpanel.h"

class LLFrameTimer;

class OSPanelQuickSettingsPulldown : public LLPanel
{
public:
	OSPanelQuickSettingsPulldown();
	void draw() override;
	void onMouseEnter(S32 x, S32 y, MASK mask) override;
	void onMouseLeave(S32 x, S32 y, MASK mask) override;
	void onTopLost() override;
	void onVisibilityChange(BOOL new_visibility) override;

private:
	LLFrameTimer mHoverTimer;
};

#endif // OS_OSPANELQUICKSETTINGSPULLDOWN_H
