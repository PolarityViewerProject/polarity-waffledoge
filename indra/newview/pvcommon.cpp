// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvcommon.h
 * @brief Common function overrides and utilities
 * Based on FSCommon used in Firestorm Viewer

 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
 * Portions Copyright (C)
 *  2011 Wolfspirit Magi
 *  2011-2013 Techwolf Lupindo
 *  2012 Ansariel Hiller @ Second Life
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

#include "pvcommon.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llinventorymodel.h"
#include "llnotificationmanager.h"
#include "lltooldraganddrop.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "lltrans.h"
#include "material_codes.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <tchar.h>
using namespace boost::posix_time;
using namespace boost::gregorian;

//PVCommon *gPVCommon = nullptr;

extern S32 gMaxAgentGroups;

bool PVCommon::sAVX_Checked = false;
bool PVCommon::sAVXSupported = false;

S32 PVCommon::sObjectAddMsg = 0;

// constructor
//void PVCommon()
//{
//	gPVCommon = PVCommon::getInstance();
//}

// Fancy little macro to output a variable's name
#define VAR_NAME(stream,variable) (stream) <<#variable": "<<(variable) 

void PVCommon::reportToNearbyChat(const std::string& message, std::string fromName /* = APP_NAME */, EChatSourceType CHAT_TYPE /* = CHAT_SOURCE_SYSTEM */)
{
	LLChat chat;
	chat.mText = message;
	chat.mFromName = fromName;
	chat.mSourceType = CHAT_TYPE;
	LLNotificationsUI::LLNotificationManager::instance().onChat(chat, LLSD());
}

// <polarity> OOC Auto-close breaks ASCII art
// Example taken from http://stackoverflow.com/a/6605305/1570096
// Todo: Write a test for this. Use the fish.
int PVCommon::HasSpecialCharacters(const std::string& oocstring)
{
	bool hasSpecial = false; // return value

	// Convert the C++ string to a c-style string (char array) to check for special characters presence.
	const char * str = oocstring.c_str();
	hasSpecial = (str[strspn(str, " !	\"\'#$%&*+,-./0123456789:;?@AaBbCcDdEeFfGgHhIiİJjKkLlMmNnOo\
		PpQqRrSsTtUuVvWwXxYyZz|~¢£¤¦§¬®±²³¼½¾ßÀàÁáÂâÃãÄäÅåÆæÇçÈèÉéÊêËëÌìÍíÎîÏïÐðÑñÒòÓóÔôÕõÖöØøÙùÚúÛûÜüÝýÞþÿĀāĂăĄą\
		ĆćĈĉċĊČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğġĠĢģĤĥħĨĩĪīĬĭĮįŌōŎŏŐőŒœ‱€№℗℠™")] != 0);
	// The above returns true (0x1) if it contains a character other than the ones in the string.
	// P.S. Does not support Japanese and such.

	if (hasSpecial)
		LL_DEBUGS("PVCommon") << "\"" << oocstring << "\" is not a word! Not closing OOC parentheses." << LL_ENDL;
	else
		LL_DEBUGS("PVCommon") << "OOC parentheses closed after \"" << oocstring << "\"." << LL_ENDL;

	return hasSpecial;
}

std::string applyAutoCloseOoc(const std::string& message)
{
	static LLCachedControl<bool> autoclose_ooc(gSavedSettings, "PVChat_AutoCloseOOC", true);
	if (!autoclose_ooc)
	{
		return message;
	}

	std::string utf8_text(message);

	// Try to find any unclosed OOC chat (i.e. an opening
	// double parenthesis without a matching closing double
	// parenthesis.
	if (utf8_text.find("(( ") != std::string::npos && utf8_text.find("))") == std::string::npos)
	{
		// <polarity> OOC Auto-close breaks ASCII art
		if (PVCommon::HasSpecialCharacters(utf8_text.substr(4)))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += " ))";
	}
	else if (utf8_text.find("((") != std::string::npos && utf8_text.find("))") == std::string::npos)
	{
		if (utf8_text.at(utf8_text.length() - 1) == ')')
		{
			// cosmetic: add a space first to avoid a closing triple parenthesis
			utf8_text += " ";
		}
		// <polarity> OOC Auto-close breaks ASCII art
		if (PVCommon::HasSpecialCharacters(utf8_text.substr(3)))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += "))";
	}
	else if (utf8_text.find("[[ ") != std::string::npos && utf8_text.find("]]") == std::string::npos)
	{
		// <polarity> OOC Auto-close breaks ASCII art
		if (PVCommon::HasSpecialCharacters(utf8_text.substr(4)))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += " ]]";
	}
	else if (utf8_text.find("[[") != std::string::npos && utf8_text.find("]]") == std::string::npos)
	{
		if (utf8_text.at(utf8_text.length() - 1) == ']')
		{
			// cosmetic: add a space first to avoid a closing triple parenthesis
			utf8_text += " ";
		}
		// <polarity> OOC Auto-close breaks ASCII art
		if (PVCommon::HasSpecialCharacters(utf8_text.substr(3)))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += "]]";
	}

	return utf8_text;
}
std::string applyMuPose(const std::string& message)
{
#ifdef MU_POSE
	std::string utf8_text(message);

	static LLCachedControl<bool> mu_pose(gSavedSettings, "PVChat_AllowMUpose", true);
	if (!mu_pose)
	{
		return utf8_text;
	}

	// Convert MU*s style poses into IRC emotes here.
	if (utf8_text.find(":") == 0 && utf8_text.length() > 3)
	{
		if (isValidWord(std::string(utf8_text, 4))) // the first 4 characters should be enough to determine if it's a word or gibberish
		{
			if(utf8_text.find(":'") == 0) // don't break /me's and such
			{
				utf8_text.replace(0, 1, "/me");
			}
			else
			{
				utf8_text.replace(0, 1, "/me ");
			}
		}
	}

	return utf8_text;
#else
	return message;
#endif
}

bool isValidWord(const std::string& message)
{
	for(int i = 0; i < message.length(); i++)//for each char in string,
	{
		// check if it's a letter or other allowed character
		if (!isdigit(message.at(i)) && !ispunct(message.at(i)) && !isspace(message.at(i)))
		{
			// return as soon as it's valid
			return true;
		}
	}
	return false;
}

std::string formatString(std::string text, const LLStringUtil::format_map_t& args)
{
	LLStringUtil::format(text, args);
	return text;
}
LLSD PVCommon::populateGroupCount()
{
	LLStringUtil::format_map_t args;
	S32 groupcount = gAgent.mGroups.size();
	args["[COUNT]"] = llformat("%d", groupcount);
	args["[REMAINING]"] = llformat("%d", gMaxAgentGroups - groupcount);
	LLUIString groupcountstring = LLTrans::getString((gMaxAgentGroups ? "groupcountstring" : "groupcountunlimitedstring"), args);
	return LLSD(groupcountstring);
}

bool PVCommon::isDefaultTexture(const LLUUID& asset_id)
{
	if (asset_id == LL_DEFAULT_WOOD_UUID ||
		asset_id == LL_DEFAULT_STONE_UUID ||
		asset_id == LL_DEFAULT_METAL_UUID ||
		asset_id == LL_DEFAULT_GLASS_UUID ||
		asset_id == LL_DEFAULT_FLESH_UUID ||
		asset_id == LL_DEFAULT_PLASTIC_UUID ||
		asset_id == LL_DEFAULT_RUBBER_UUID ||
		asset_id == LL_DEFAULT_LIGHT_UUID ||
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f") ||	// UIImgWhiteUUID
		asset_id == LLUUID("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903") ||	// UIImgTransparentUUID
		asset_id == LLUUID("f54a0c32-3cd1-d49a-5b4f-7b792bebc204") ||	// UIImgInvisibleUUID
		asset_id == LLUUID("6522e74d-1660-4e7f-b601-6f48c1659a77") ||	// UIImgDefaultEyesUUID
		asset_id == LLUUID("7ca39b4c-bd19-4699-aff7-f93fd03d3e7b") ||	// UIImgDefaultHairUUID
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f")		// UIImgDefault for all clothing
		)
	{
		return true;
	}
	return false;
}

S32 PVCommon::secondsSinceEpochFromString(const std::string& format, const std::string& str)
{
	// LLDateUtil::secondsSinceEpochFromString does not handle time, only the date.
	// copied that function here and added the needed code to handle time fields.  -- TL
	time_input_facet *facet = new time_input_facet(format);
	std::stringstream ss;
	ss << str;
	ss.imbue(std::locale(ss.getloc(), facet));
	ptime time_t_date;
	ss >> time_t_date;
	ptime time_t_epoch(date(1970, 1, 1));
	time_duration diff = time_t_date - time_t_epoch;
	return diff.total_seconds();
}


bool PVCommon::isAVXSupported()
{
	if (sAVX_Checked) // Has been checked already, return the cached result
	{
		return sAVXSupported;
	}

	// Test for AVX Support
	//  Copied from: http://stackoverflow.com/a/22521619/922184
	// Improvements from https://github.com/Mysticial/FeatureDetector

	// If Visual Studio 2010 SP1 or later
#if (_MSC_FULL_VER >= 160040219)
	// Checking for AVX requires 3 things:
	// 1) CPUID indicates that the OS uses XSAVE and XRSTORE
	//     instructions (allowing saving YMM registers on context
	//     switch)
	// 2) CPUID indicates support for AVX
	// 3) XGETBV indicates the AVX registers will be saved and
	//     restored on context switch
	//
	// Note that XGETBV is only available on 686 or later CPUs, so
	// the instruction needs to be conditionally run.

	int cpuInfo[4];
	__cpuid(cpuInfo, 1);

	bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
	bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

	if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
	{
		// Check if the OS will save the YMM registers
		uint64_t xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
		sAVXSupported = (xcrFeatureMask & 0x6) == 0x6;
	}
#endif
	LL_INFOS() << "AVX support = " << sAVXSupported << LL_ENDL;

	// Early exit for the next time we need to run this check
	sAVX_Checked = true;

	return sAVXSupported;
}

std::string PVCommon::format_string(std::string text, const LLStringUtil::format_map_t& args)
{
	LLStringUtil::format(text, args);
	return text;
}
