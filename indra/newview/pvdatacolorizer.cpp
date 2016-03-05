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

// #if RLV_SUPPORT
// #include "rlvhandler.h"
// #endif // RLV_SUPPORT

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

LLColor4 PVDataColorizer::getColor(const LLUUID& avatar_id, const std::string& default_color, const bool& should_show_friend)
{
	/*NOT static*/ LLUIColor name_color; // Initialized as black by default.

	// handle friend color first since it overrides a lot.
#if 0
	static LLCachedControl<bool> show_as_friend(gSavedSettings, "NameTagShowFriends");
	static LLCachedControl<bool> override_friend_color(gSavedSettings, "PVColorManager_OverrideFriendColor");
	if (!override_friend_color && ((should_show_friend && (show_as_friend && (LLAvatarTracker::instance().isBuddy(avatar_id)) && (gAgent.getID() != avatar_id)
#if RLV_SUPPORT
								&& !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)
#endif // RLV_SUPPORT
								))))
	{
		name_color = LLUIColorTable::instance().getColor("MapAvatarFriendColor", LLColor4::green);
	}
	else
#endif
	{
		static const LLUIColor dev_color = LLUIColorTable::instance().getColor("PlvrDevChatColor", LLColor4::orange);
		static const LLUIColor linden_color = LLUIColorTable::instance().getColor("PlvrLindenChatColor", LLColor4::cyan);
		static const LLUIColor muted_color = LLUIColorTable::instance().getColor("PlvrMutedChatColor", LLColor4::grey);
		static const LLUIColor qa_color = LLUIColorTable::instance().getColor("PlvrQAChatColor", LLColor4::red);
		static const LLUIColor support_color = LLUIColorTable::instance().getColor("PlvrSupportChatColor", LLColor4::magenta);
		static const LLUIColor tester_color = LLUIColorTable::instance().getColor("PlvrTesterChatColor", LLColor4::yellow);
		static const LLUIColor banned_color = LLUIColorTable::instance().getColor("PlvrBannedChatColor", LLColor4::grey2);

		// Check for agents flagged through PVData
		signed int av_flags = PVData::instance().getAgentFlags(avatar_id);
		if (av_flags == 0)
		{
			// Agent is not flagged by PVData, start user-specific logic
			LLAvatarName av_name;
			LLAvatarNameCache::get(avatar_id, &av_name);
			std::string user_name = av_name.getUserName();
			if (LLMuteList::instance().isLinden(user_name))
			{
				name_color = linden_color.get();
			}
			else if (LLMuteList::instance().isMuted(avatar_id, user_name))
			{
				name_color = muted_color.get();
			}
			// Maybe find a way to indentify partner?
			else
			{
				// Fall back to specified default color
				name_color = LLUIColorTable::instance().getColor(default_color, LLColor4::magenta);
			}
		}
		else
		{
			// Special color overrides all colors
			if (av_flags & PVData::FLAG_USER_HAS_COLOR)
			{
				// Gross hack.
				// TODO: Remove typecast once fixed in getAgentColor funtion
				name_color = static_cast<LLColor4>(PVData::instance().getAgentColor(avatar_id).getValue());
				if (name_color == LLColor4::black)
				{
					LL_WARNS("PVData") << "Color Manager caught a bug! Agent is supposed to have a color but none is	defined!" << LL_ENDL;
					LL_WARNS("PVData") << "avatar_id = " << avatar_id << LL_ENDL;
					LL_WARNS("PVData") << "av_flags = " << av_flags << LL_ENDL;
					LL_WARNS("PVData") << "would-be name_color = " << name_color << LL_ENDL;
					LL_WARNS("PVData") << "Report this occurence and send the lines above to the Polarity Developers" << LL_ENDL;
				}
			}
			else if (av_flags & PVData::FLAG_STAFF_DEV)
			{
				name_color = dev_color.get();
			}
			else if (av_flags & PVData::FLAG_STAFF_QA)
			{
				name_color = qa_color.get();
			}
			else if (av_flags & PVData::FLAG_STAFF_SUPPORT)
			{
				name_color = support_color.get();
			}
			else if (av_flags & PVData::FLAG_USER_BETA_TESTER)
			{
				name_color = tester_color.get();
			}
			else if (av_flags & PVData::FLAG_USER_BANNED)
			{
				name_color = banned_color.get();
			}
			else
			{
				LL_WARNS("PVData") << "Color Manager caught a bug! Agent is supposed to be special but no code path exists for this case!\n" << "(This is most likely caused by a missing agent flag)" << LL_ENDL;
				LL_WARNS("PVData") << "~~~~~~~ COLOR DUMP ~~~~~~~" << LL_ENDL;
				LL_WARNS("PVData") << "avatar_id = " << avatar_id << LL_ENDL;
				LL_WARNS("PVData") << "av_flags = " << av_flags << LL_ENDL;
				LL_WARNS("PVData") << "would-be name_color = " << name_color << LL_ENDL;
				LL_WARNS("PVData") << "~~~ END OF COLOR DUMP ~~~" << LL_ENDL;
				LL_WARNS("PVData") << "Report this occurence and send the lines above to the Polarity Developers" << LL_ENDL;
			}
		}
	}

	//LL_DEBUGS("PVData") << "Returning " << name_color << LL_ENDL;
	return name_color;
}

// Call this very early.
void PVDataColorizer::initThemeColors()
{
	LL_INFOS("LLViewerWindow") << "Initializing theme default color..." << LL_ENDL;
	// Initial theme color setup. A bit savage, but this is work in progress.
	//bAgentHasCustomColor = false;

	// TODO: Figure a way to cache agent color to speed up login WITHOUT breaking pvdata color change.
	// We might need the pvdata live refresh for this.
	LLColor4 init_color4 = LLUIColorTable::instance().getColor("PVData_ThisAgentCachedSkinColor");
	if (init_color4 == LLColor4::black || init_color4 == LLColor4::magenta)
	{
		// First run, force automatic color
		LL_INFOS("LLViewerWindow") << "something went wrong, fixing colors..." << LL_ENDL;
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
	LL_INFOS("LLViewerWindow") << "Theme default color initialized!" << LL_ENDL;
}

void PVDataColorizer::setNewSkinColorFromSelection()
{
	LL_DEBUGS("LLViewerWindow") << "Getting theme colors from selection dropdown..." << LL_ENDL;
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
		static const boost::regex is_release_channel("\\bRelease$"); // <Polarity/>
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
	LL_INFOS("LLViewerWindow") << "Setting emphasis color..." << LL_ENDL;
	// <Polarity> Replace the color in the color table.
	const LLColor4 input_new_color = newCustomThemeColor; // Temporary variable to avoid creating a ton of new ones
	LLColor4 tmp_color = input_new_color;

	if (tmp_color == LLColor4::black)
	{
		LL_WARNS("LLViewerWindow") << "OH GOD HELP MY COLORS WENT BLACK" << LL_ENDL;
		return;
	}
	// Override the entire accent color set. Wheeeee. \o.o/
	LL_WARNS("LLViewerWindow") << "New theme color = " << tmp_color << LL_ENDL;
	LLUIColorTable::instance().setColor("EmphasisColor", LLColor4(input_new_color));

	LLColor4 tmp_color_13 = LLColor4(input_new_color.mV[0], input_new_color.mV[1], input_new_color.mV[2], 0.13f);
	LLUIColorTable::instance().setColor("EmphasisColor_13", tmp_color_13);
	//LL_WARNS("LLViewerWindow") << "EmphasisColor_13 = " << tmp_color_13 << LL_ENDL;

	LLColor4 tmp_color_35 = LLColor4(input_new_color.mV[0], input_new_color.mV[1], input_new_color.mV[2], 0.35f);
	LLUIColorTable::instance().setColor("EmphasisColor_35", tmp_color_35);
	//LL_WARNS("LLViewerWindow") << "EmphasisColor_35 = " << tmp_color_35 << LL_ENDL;

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

	LL_INFOS("LLViewerWindow") << "Theme colors set!" << LL_ENDL;
}

bool PVDataColorizer::themeColorHasChanged() const
{
	LL_WARNS("LLViewerWindow") << "Theme color has changed!" << LL_ENDL;
	return (newCustomThemeColor != currentThemeColor);
}

// This one is called after the colors has been initially set.
void PVDataColorizer::refreshThemeColors()
{
	if (themeColorHasChanged())
	{
		LL_INFOS("LLViewerWindow") << "Refreshing theme colors!" << LL_ENDL;
		setEmphasisColorSet();
	}
}
