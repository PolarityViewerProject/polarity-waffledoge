/**
 * @file pvdatacolorizer.cpp
 * @brief Set of functions to color names and related elements
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (C) 2015 Xenhat Liamano
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

#pragma once // <Polarity> Compilation speedup

#include "llsingleton.h"

class LLColor4;
class LLColor3;
class LLUUID;

class PVDataColorizer: public LLSingleton <PVDataColorizer>
{
private:
	friend class LLSingleton <PVDataColorizer>;
public:
	~PVDataColorizer()
	{
	}

	// Contains whatever the latest theme color should be.
	LLColor4 newCustomThemeColor;
	// Basically a color table-independent version of EmphasisColor
	LLColor4 currentThemeColor;
	// Our agent's pvdata color
	//LLColor4 pvdataAgentColor;
	// Cached answer for "does our user have a custom color?"
	bool bAgentHasCustomColor;

	// Gross hack
	LLUUID pAgentUUID;

	// Convert to HSV, modify hue, then convert back to RGB.
	static LLColor4 addOrSubstractHue(LLColor4 in_color4, const F32 new_hu_f32);

	// Convert color to HSV, modify saturation, then convert back to RGB.
	static LLColor4 addOrSubstractSaturation(LLColor4 in_color4, const F32 new_sat_f32);

	// Convert color to HSV, modify value, then convert back to RGB.
	static LLColor4 addOrSubstractLight(LLColor4 in_color4, const F32 new_light_f32);

	// Convert color to HSV, modify saturation and value, then convert back to RGB.
	static LLColor4 addOrSubstractSaturationAndLight(LLColor4 in_color4, const F32 new_sat_f32, const F32 new_light_f32);

	// Get a color for the specified agent (UUID version)
	static LLColor4 getColor(const LLUUID& avatar_id, const std::string& default_color, const bool& show_friend);

	void setNewSkinColorFromSelection();
	void initThemeColors();
	void refreshThemeColors();
	void setEmphasisColorSet();
	bool themeColorHasChanged() const;
};
