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

#include "llviewerprecompiledheaders.h"
#include "pvdatacolorizer.h"
 // lib includes
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include "lluuid.h"
#include "v4color.h"
// This makes ReSharper Happy
//#include "boost/regex.hpp"
#include <boost/regex/v4/regex.hpp>

// viewer includes
#include "llcallingcard.h"
#include "llmutelist.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llversioninfo.h"

#include "pvdata.h"
#include "pvcommon.h"

#include "rlvhandler.h"

LLColor4 PVDataColorizer::addOrSubstractHue(const LLColor4 in_color4, const F32 new_hue_f32)
{
	F32 hue, sat, val;
	LLColor4(in_color4).calcHSL(&hue, &sat, &val);
	LLColor4 out_color;
	out_color.setHSL((hue + new_hue_f32), sat, val);
	return out_color;
}

LLColor4 PVDataColorizer::addOrSubstractSaturation(const LLColor4 in_color4, const F32 new_sat_f32)
{
	F32 hue, sat, val;
	LLColor4(in_color4).calcHSL(&hue, &sat, &val);
	LLColor4 out_color;
	out_color.setHSL(hue, (sat + new_sat_f32), val);
	return out_color;
}

LLColor4 PVDataColorizer::addOrSubstractLight(const LLColor4 in_color4, const F32 new_light_f32)
{
	F32 hue, sat, val;
	LLColor4(in_color4).calcHSL(&hue, &sat, &val);
	LLColor4 out_color;
	out_color.setHSL(hue, sat, (val + new_light_f32));
	return out_color;
}

LLColor4 PVDataColorizer::addOrSubstractSaturationAndLight(const LLColor4 in_color4, const F32 new_sat_f32, const F32 new_light_f32)
{
	F32 hue, sat, val;
	LLColor4(in_color4).calcHSL(&hue, &sat, &val);
	LLColor4 out_color;
	out_color.setHSL(hue, (sat + new_sat_f32), (val + new_light_f32));
	return out_color;
}

LLColor4 PVDataColorizer::getColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& is_buddy_and_show_it)
{
	// Coloring will break immersion and identify agents even if their name is replaced, so return default color in that case.
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
	{
		return default_color;
	}
	LLColor4 return_color = default_color; // color we end up with at the end of the logic
	LLColor4 pvdata_color; // User color from PVData if user has one, equals return_color otherwise.

	static bool pvdata_color_is_valid = false;

	static const LLUIColor linden_color = LLUIColorTable::instance().getColor("PlvrLindenChatColor", LLColor4::cyan);
	static const LLUIColor muted_color = LLUIColorTable::instance().getColor("PlvrMutedChatColor", LLColor4::grey);

	// we'll need this later. Defined here to avoid multiple calls in the same code path.
	//LLAvatarName av_name;

	// Called first to seed av_name
	/*
	if (PVData::instance().isLinden(avatar_id, av_name))
	{
		// TODO: Make sure we only hit this code path once per Linden (make sure they get added properly)
		// This means we need to save the linden list somewhere probably when refreshing pvdata, or just use
		// an entirely different list. Another solution (probably the most lightweight one) would be to check
		// if a custom title has been attributed to them here instead of down there.
		return_color = linden_color.get();
	}
	*/
	// Some PVData-flagged users CAN be muted.
	if (LLMuteList::instance().isMuted(avatar_id))
	{
		return_color = muted_color.get();
		return return_color;
	}

	// Check if agent is flagged through PVData
	signed int av_flags = PVData::instance().getAgentFlags(avatar_id);
	bool has_flags = (av_flags > 0);
	if (has_flags)
	{
		pvdata_color_is_valid = true;
		if (av_flags & PVData::FLAG_USER_HAS_TITLE && !(av_flags & PVData::FLAG_TITLE_OVERRIDE))
		{
			// Do not warn when the user only has a title and no special color since it is acceptable
		}
		else if (av_flags & PVData::FLAG_LINDEN_EMPLOYEE)
		{
			// was previously flagged as employee, so will end up in this code path
			pvdata_color = linden_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_DEV)
		{
			static const LLUIColor dev_color = LLUIColorTable::instance().getColor("PlvrDevChatColor", LLColor4::orange);
			pvdata_color = dev_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_QA)
		{
			static const LLUIColor qa_color = LLUIColorTable::instance().getColor("PlvrQAChatColor", LLColor4::red);
			pvdata_color = qa_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_SUPPORT)
		{
			static const LLUIColor support_color = LLUIColorTable::instance().getColor("PlvrSupportChatColor", LLColor4::magenta);
			pvdata_color = support_color.get();
		}
		else if (av_flags & PVData::FLAG_USER_BETA_TESTER)
		{
			static const LLUIColor tester_color = LLUIColorTable::instance().getColor("PlvrTesterChatColor", LLColor4::yellow);
			pvdata_color = tester_color.get();
		}
		else if (av_flags & PVData::FLAG_USER_BANNED)
		{
			static const LLUIColor banned_color = LLUIColorTable::instance().getColor("PlvrBannedChatColor", LLColor4::grey2);
			pvdata_color = banned_color.get();
		}
		else
		{
			LL_WARNS("PVData") << "Color Manager caught a bug! Agent is supposed to be special but no code path exists for this case!\n" << "(This is most likely caused by a missing agent flag)" << LL_ENDL;
			LL_WARNS("PVData") << "~~~~~~~ COLOR DUMP ~~~~~~~" << LL_ENDL;
			LL_WARNS("PVData") << "avatar_id = " << avatar_id << LL_ENDL;
			LL_WARNS("PVData") << "av_flags = " << av_flags << LL_ENDL;
			LL_WARNS("PVData") << "would-be pvdata_color = " << pvdata_color << LL_ENDL;
			LL_WARNS("PVData") << "~~~ END OF COLOR DUMP ~~~" << LL_ENDL;
			LL_WARNS("PVData") << "Report this occurence and send the lines above to the Polarity Developers" << LL_ENDL;
			//pvdata_color = default_color;
			pvdata_color_is_valid = false; // to be sure
		}
		// Special color, when defined, overrides all colors
		//pvdata_color = PVData::instance().getAgentColor(avatar_id);
		LLColor4 agent_color = static_cast<LLColor4>(PVData::instance().mAgentColors[avatar_id]);
		if (agent_color != LLColor4::black && agent_color != LLColor4::magenta)
		{
			// No custom color defined, set as white.
			// TODO: Use a color defined in colors.xml
			//static const LLUIColor default_tag_color = LLUIColorTable::instance().getColor("NameTagMatch", LLColor4::white);
			pvdata_color = agent_color;
			pvdata_color_is_valid = true;
		}
		if (!pvdata_color_is_valid)
		{
			pvdata_color = default_color;
		}
	}

	/*	Respect user preferences
		Expected behavior:
			+Friend, +PVDATA, +lpf = show PVDATA
			+Friend, +PVDATA, -lpl = show FRIEND
			+Friend, -PVDATA, +lpl = show FRIEND
			+Friend, -PVDATA, -lpl = show FRIEND
			-Friend, +PVDATA, +lpl = show PVDATA
			-Friend, +PVDATA, -lpl = show PVDATA
			-Friend, -PVDATA, +lpl = show FALLBACK
			-Friend, -PVDATA, -lpl = show FALLBACK
	*/
	static LLCachedControl<bool> show_friends(gSavedSettings, "NameTagShowFriends");
	static LLCachedControl<bool> low_priority_friend_status(gSavedSettings, "PVColorManager_LowPriorityFriendStatus", true);
	bool show_f = (show_friends && is_buddy_and_show_it);

	// Lengthy but fool-proof.
	if (show_f && has_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (show_f && has_flags && !low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !has_flags && low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !has_flags && !low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (!show_f && has_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && has_flags && !low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && !has_flags && low_priority_friend_status)
	{
		return_color = default_color;
	}
	if (!show_f && !has_flags && !low_priority_friend_status)
	{
		return_color = default_color;
	}

	return return_color;
}

// Call this very early.
void PVDataColorizer::initThemeColors()
{
	LL_INFOS("PVData") << "Initializing theme default color..." << LL_ENDL;
	// Initial theme color setup. A bit savage, but this is work in progress.
	//bAgentHasCustomColor = false;

	// TODO: Figure a way to cache agent color to speed up login WITHOUT breaking pvdata color change.
	// We might need the pvdata live refresh for this.
	LLColor4 init_color4 = LLUIColorTable::instance().getColor("PVData_ThisAgentCachedSkinColor");
	if (init_color4 == LLColor4::black || init_color4 == LLColor4::magenta)
	{
		// First run, force automatic color
		LL_INFOS("PVData") << "something went wrong, fixing colors..." << LL_ENDL;
		const auto old_choice = gSavedSettings.getU32("PVUI_ThemeColorSelection");
		// TODO: Optimize me!
		gSavedSettings.setU32("PVUI_ThemeColorSelection", 0);
		setNewSkinColorFromSelection();
		gSavedSettings.setU32("PVUI_ThemeColorSelection", old_choice);
	}
	else
	{
		setNewSkinColorFromSelection();
	}
	LL_INFOS("PVData") << "Theme default color initialized!" << LL_ENDL;
}

void PVDataColorizer::setNewSkinColorFromSelection()
{
	LL_INFOS("PVDebug") << "Getting theme colors from selection dropdown..." << LL_ENDL;
	// Force specific theme color
	// 0 = automatic - channel-based
	// 1 = release
	// 2 = Project
	// 3 = Beta
	// 4 = Test
	const U32 current_theme_choice = gSavedSettings.getU32("PVUI_ThemeColorSelection");

	if (current_theme_choice == 0)
	{
		// no l10n problem because channel is always an english string
		std::string channel = LLVersionInfo::getChannel();
		static const boost::regex is_beta_channel("\\bBeta\\b");
		static const boost::regex is_project_channel("\\bProject\\b");
		static const boost::regex is_release_channel("\\bRelease$"); // <polarity/>
		static const boost::regex is_test_channel("\\bTest$");

		//	While skinning the viewer. there are times where coloring something based on the current channel
		//	is a good idea. An example of this is the text on the login screen. To do so, we reference a
		//	color named "EmphasisColor", which is set using the code below at each viewer run.
		if (boost::regex_search(channel, is_release_channel))
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelReleaseColor");
		}
		else if (boost::regex_search(channel, is_test_channel))
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelTestColor");
		}
		else if (boost::regex_search(channel, is_beta_channel))
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelBetaColor");
		}
		else if (boost::regex_search(channel, is_project_channel))
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelProjectColor");
		}
	}
	else
	{
		if (1 == current_theme_choice)
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelReleaseColor");
		}
		else if (2 == current_theme_choice)
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelProjectColor");
		}
		else if (3 == current_theme_choice)
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelBetaColor");
		}
		else if (4 == current_theme_choice)
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("ChannelTestColor");
		}
		else if (5 == current_theme_choice)
		{
			newCustomThemeColor = LLUIColorTable::instance().getColor("CustomThemeColor");
		}
	}
	refreshThemeColors();
}

void PVDataColorizer::setEmphasisColorSet()
{
	LL_INFOS("PVData") << "Setting emphasis color..." << LL_ENDL;
	// <polarity> Replace the color in the color table.
	const LLColor4 input_new_color = newCustomThemeColor; // Temporary variable to avoid creating a ton of new ones
	LLColor4 tmp_color = input_new_color;

	if (tmp_color == LLColor4::black)
	{
		LL_WARNS("PVData") << "OH GOD HELP MY COLORS WENT BLACK" << LL_ENDL;
		return;
	}
	// Override the entire accent color set. Wheeeee. \o.o/
	LL_WARNS("PVData") << "New theme color = " << tmp_color << LL_ENDL;
	LLUIColorTable::instance().setColor("EmphasisColor", LLColor4(input_new_color));

	LLColor4 tmp_color_13 = LLColor4(input_new_color.mV[0], input_new_color.mV[1], input_new_color.mV[2], 0.13f);
	LLUIColorTable::instance().setColor("EmphasisColor_13", tmp_color_13);
	//LL_WARNS("PVData") << "EmphasisColor_13 = " << tmp_color_13 << LL_ENDL;

	LLColor4 tmp_color_35 = LLColor4(input_new_color.mV[0], input_new_color.mV[1], input_new_color.mV[2], 0.35f);
	LLUIColorTable::instance().setColor("EmphasisColor_35", tmp_color_35);
	//LL_WARNS("PVData") << "EmphasisColor_35 = " << tmp_color_35 << LL_ENDL;

	LLUIColorTable::instance().setColor("FilterTextColor", tmp_color_13);

	// This one is fairly dark. Try not to over-use.
	std::vector<std::string> ListEmphasisColors = {
		"ColorPaletteEntry64" // reserved until additional block for it in color picker
		"ContextSilhouetteColor",
		"MenuItemHighlightBgColor", // Menu highlight background
		"DefaultHighlightLight",
		"InventorySearchStatusColor",
		"PVLineEditorBackgroundSelected",
		"SelectedOutfitTextColor",
		"SilhouetteParentColor",
		"StatBarMinMaxDots",
		"TextEmbeddedItemColor",
		"PVLineEditorHighlight"
		"FocusColor",
	};
	for (auto const& value : ListEmphasisColors)
	{
		LLUIColorTable::instance().setColor(value, input_new_color);
	}

	//
	// Some procedurally altered colors for dynamic theming
	//
	// Brighter color for things that need to be VERY visible. PLEASE DON'T USE TOO MUCH.
	LLColor4 emphasis_bright_color4 = addOrSubstractLight(input_new_color, 0.35f);
	std::vector<std::string> EmphasisColorsBright = {
		"NameTagStatusText",
	};
	for (auto const& value : EmphasisColorsBright)
	{
		LLUIColorTable::instance().setColor(value, emphasis_bright_color4);
	}

	// Paler (desaturated) color for things that needs to flash and such
	LLColor4 emphasis_desaturated_color4 = addOrSubstractSaturation(input_new_color, -0.35f);
	std::vector<std::string> ListEmphasisColorPale = {
		"EmphasisColorPale",
		"ButtonFlashBgColor",
		"MenuItemFlashBgColor",
	};
	for (auto const& value : ListEmphasisColorPale)
	{
		LLUIColorTable::instance().setColor(value, emphasis_desaturated_color4);
	}

	// Paler (milkier) color for visible but not eye-gouging things
	LLColor4 emphasis_milky_color4 =  addOrSubstractSaturationAndLight(input_new_color, -0.35f, 0.25f);
	std::vector<std::string> ListEmphasisColorMilky = {
		"EmphasisColorMilky",
		"EmphasisColorLight",
		"ButtonFlashBgColor",
		"MenuItemFlashBgColor",
		"ChatHeaderDisplayNameColor",
		"ChatHeaderUserNameColor",
		"ChatHeaderTimestampColor",
		// "NameTagChat",
		"DefaultHighlightDark",
		// "MenuItemHighlightFgColor",
		"ButtonSelectedBgColor",
		"ScrollHoveredColor",
		"TitleBarFocusColor",
		"PCMSelectionHighlight",
		"GroupActive",
		"ProfileOnlineIndicatorColor",
		"SpeakingColor",
	};
	for (auto const& value : ListEmphasisColorMilky)
	{
		LLUIColorTable::instance().setColor(value, emphasis_milky_color4);
	}

	// Needed for Avatar Cloud
	LLUIColorTable::instance().setColor("MutedCloudStart", LLColor3(0.49412, 0.64314, 0.27843));
	LLUIColorTable::instance().setColor("MutedCloudEnd", LLColor3(0.60784, 0.75294, 0.16863));
	//LLUIColorTable::instance().setColor("CustomThemeColor", newCustomThemeColor);

	// Finally update our new color as "current"
	currentThemeColor = newCustomThemeColor;

	LL_INFOS("PVData") << "Theme colors set!" << LL_ENDL;
}

bool PVDataColorizer::themeColorHasChanged() const
{
	LL_WARNS("PVData") << "Theme color has changed!" << LL_ENDL;
	return (newCustomThemeColor != currentThemeColor);
}

// This one is called after the colors has been initially set.
void PVDataColorizer::refreshThemeColors()
{
	if (themeColorHasChanged())
	{
		LL_INFOS("PVData") << "Refreshing theme colors!" << LL_ENDL;
		setEmphasisColorSet();
	}
}
