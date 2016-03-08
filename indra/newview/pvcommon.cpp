/**
 * @file pvcommon.h
 * @brief Common function overrides and utilities
 * Based on FSCommon used in Firestorm Viewer

 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
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
// #include "fsradar.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llfloaterperms.h"
#include "llinventorymodel.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"	// <FS:CR> reportToNearbyChat
#include "lltooldraganddrop.h"
#include "llviewerinventory.h"
#include "llviewernetwork.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
// #include "rlvactions.h"
// #include "rlvhandler.h"

// <Polarity> Build Fixes
#include "lltrans.h"
#include "material_codes.h"
// </Polarity>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

static const std::string LL_LINDEN = "Linden";
static const std::string LL_MOLE = "Mole";
static const std::string LL_PRODUCTENGINE = "ProductEngine";
static const std::string LL_SCOUT = "Scout";
static const std::string LL_TESTER = "Tester";

extern S32 gMaxAgentGroups;

bool PVCommon::sAVX_Checked = false;
bool PVCommon::sAVXSupported = false;

S32 PVCommon::sObjectAddMsg = 0;

void reportToNearbyChat(const std::string& message)
{
	if (message.empty()) return;

	LLChat chat;
	chat.mText = message;
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	LLNotificationsUI::LLNotificationManager::instance().onChat(chat, LLSD());
}
void reportSpecialToNearbyChat(const std::string& message, EChatSourceType CHAT_TYPE, std::string fromName)
{
	LLChat chat;
	chat.mText = message;
	chat.mFromName = fromName;
	chat.mSourceType = CHAT_TYPE;
	LLNotificationsUI::LLNotificationManager::instance().onChat(chat, LLSD());
}

// <Polarity> OOC Auto-close breaks ASCII art
// Example taken from http://stackoverflow.com/a/6605305/1570096
// Todo: Write a test for this. Use the fish.
int PVCommon::HasSpecialCharacters(const std::string& oocstring)
{
	bool hasSpecial = 0; // return value

	// Convert the C++ string to a c-style string (char array) to check for special characters presence.
	const char * str = oocstring.c_str();
	hasSpecial = (str[strspn(str, " !	\"\'#$%&()*+,-./0123456789:;<=>?@[\\]^_`AaBbCcDdEeFfGgHhIiİJjKkLlMmNnOo\
    PpQqRrSsTtUuVvWwXxYyZz{|}~¢£¤¦§¬®±²³¼½¾ßÀàÁáÂâÃãÄäÅåÆæÇçÈèÉéÊêËëÌìÍíÎîÏïÐðÑñÒòÓóÔôÕõÖöØøÙùÚúÛûÜüÝýÞþÿĀāĂăĄą\
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
	if (!gSavedSettings.getBOOL("PVChat_AutoCloseOOC"))
	{
		return message;
	}

	std::string utf8_text(message);

	// <Polarity> OOC Auto-close breaks ASCII art
	// It might be more flexible to subtract the opening parentheses, brackets and other things
	// from the string and run the result through HasSpecialChacters but I'm not really interested
	// in doing more substring operations. -Xenhat

	// Moved down to avoid checking every single passed string
	// if (PVCommon::HasSpecialCharacters(utf8_text))
	// return message;

	// Try to find any unclosed OOC chat (i.e. an opening
	// double parenthesis without a matching closing double
	// parenthesis.
	if (utf8_text.find("(( ") != std::string::npos && utf8_text.find("))") == std::string::npos)
	{
		if (PVCommon::HasSpecialCharacters(utf8_text))
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
		if (PVCommon::HasSpecialCharacters(utf8_text))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += "))";
	}
	else if (utf8_text.find("[[ ") != std::string::npos && utf8_text.find("]]") == std::string::npos)
	{
		if (PVCommon::HasSpecialCharacters(utf8_text))
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
		if (PVCommon::HasSpecialCharacters(utf8_text))
			return message;
		// add the missing closing double parenthesis.
		utf8_text += "]]";
	}

	return utf8_text;
}
std::string applyMuPose(const std::string& message)
{
	std::string utf8_text(message);

	// Convert MU*s style poses into IRC emotes here.
	if (gSavedSettings.getBOOL("PVChat_AllowMUpose") && utf8_text.find(":") == 0 && utf8_text.length() > 3)
	{
		if (utf8_text.find(":'") == 0)
		{
			utf8_text.replace(0, 1, "/me");
		}
		else if (!isdigit(utf8_text.at(1)) && !ispunct(utf8_text.at(1)) && !isspace(utf8_text.at(1)))  // Do not prevent smileys and such.
		{
			utf8_text.replace(0, 1, "/me ");
		}
	}

	return utf8_text;
}

std::string formatString(std::string text, const LLStringUtil::format_map_t& args)
{
	LLStringUtil::format(text, args);
	return text;
}
#if 0
void PVCommon::applyDefaultBuildPreferences(LLViewerObject* object)
{
	if (!object)
	{
		return;
	}

	LLTextureEntry texture_entry;
	texture_entry.setID(LLUUID(gSavedSettings.getString("FSDefaultObjectTexture")));
	texture_entry.setColor(gSavedSettings.getColor4("PVTools_Color"));
	texture_entry.setAlpha((100.f - gSavedSettings.getF32("PVTools_Alpha")) / 100.f);
	texture_entry.setGlow(gSavedSettings.getF32("PVTools_Glow"));
	if (gSavedSettings.getBOOL("PVTools_FullBright"))
	{
		texture_entry.setFullbright(TEM_FULLBRIGHT_MASK);
	}

	U8 shiny = 0; // Default none
	std::string shininess = gSavedSettings.getString("PVTools_Shiny");
	if (shininess == "Low")
	{
		shiny = 1;
	}
	else if (shininess == "Medium")
	{
		shiny = 2;
	}
	else if (shininess == "High")
	{
		shiny = 3;
	}
	texture_entry.setShiny(shiny);

	for (U8 face = 0; face < object->getNumTEs(); face++)
	{
		object->setTE(face, texture_entry);
	}
	object->sendTEUpdate();

#if 0
	if (gSavedPerAccountSettings.getBOOL("PVTools_EmbedItem"))
	{
		LLUUID item_id(gSavedPerAccountSettings.getString("PVTools_Item"));
		if (item_id.notNull())
		{
			LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(gInventory.getObject(item_id));
			if (item)
			{
				if (item->getType() == LLAssetType::AT_LSL_TEXT)
				{
					LLToolDragAndDrop::dropScript(object, item, TRUE,
												  LLToolDragAndDrop::SOURCE_AGENT,
												  gAgentID);
				}
				else
				{
					LLToolDragAndDrop::dropInventory(object, item,
													 LLToolDragAndDrop::SOURCE_AGENT,
													 gAgentID);
				}
			}
		}
	}
#endif
	U32 object_local_id = object->getLocalID();

	if (!LLFloaterPermsDefault::getCapSent())
	{
		gMessageSystem->newMessageFast(_PREHASH_ObjectPermissions);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_HeaderData);
		gMessageSystem->addBOOLFast(_PREHASH_Override, (BOOL) FALSE);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerModify"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_MODIFY);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerCopy"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_COPY);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerTransfer"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_TRANSFER);
		gMessageSystem->sendReliable(object->getRegion()->getHost());
	}

	gMessageSystem->newMessage(_PREHASH_ObjectFlagUpdate);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
	gMessageSystem->addBOOLFast(_PREHASH_UsePhysics, gSavedSettings.getBOOL("PVTools_Physical"));
	gMessageSystem->addBOOL(_PREHASH_IsTemporary, gSavedSettings.getBOOL("PVTools_Temporary"));
	gMessageSystem->addBOOL(_PREHASH_IsPhantom, gSavedSettings.getBOOL("PVTools_Phantom"));
	gMessageSystem->addBOOL("CastsShadows", FALSE);
	gMessageSystem->sendReliable(object->getRegion()->getHost());
}
#endif

bool PVCommon::isLinden(const LLUUID& av_id)
{
	std::string first_name, last_name;
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(av_id, &av_name))
	{
		std::istringstream full_name(av_name.getUserName());
		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(av_id, first_name, last_name);
	}
	return (last_name == LL_LINDEN ||
			last_name == LL_MOLE ||
			last_name == LL_PRODUCTENGINE ||
			last_name == LL_SCOUT ||
			last_name == LL_TESTER);
}
#if 0
bool PVCommon::checkIsActionEnabled(const LLUUID& av_id, EFSRegistrarFunctionActionType action)
{
	bool isSelf = (av_id == gAgentID);

	if (action == FS_RGSTR_ACT_ADD_FRIEND)
	{
		return (!isSelf && !LLAvatarActions::isFriend(av_id));
	}
	else if (action == FS_RGSTR_ACT_REMOVE_FRIEND)
	{
		return (!isSelf && LLAvatarActions::isFriend(av_id));
	}
	else if (action == FS_RGSTR_ACT_SEND_IM)
	{
		return (!isSelf && RlvActions::canStartIM(av_id));
	}
// else if (action == FS_RGSTR_ACT_ZOOM_IN)
// {
//	return (!isSelf && LLAvatarActions::canZoomIn(av_id));
// }
	else if (action == FS_RGSTR_ACT_OFFER_TELEPORT)
	{
		return (!isSelf && LLAvatarActions::canOfferTeleport(av_id));
	}
// else if (action == FS_RGSTR_ACT_REQUEST_TELEPORT)
// {
//	return (!isSelf && LLAvatarActions::canRequestTeleport(av_id));
// }
	else if (action == FS_RGSTR_ACT_SHOW_PROFILE)
	{
		return (isSelf || !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
	}
// else if (action == FS_RGSTR_ACT_TRACK_AVATAR)
// {
//	return (!isSelf && FSRadar::getInstance()->getEntry(av_id) != NULL);
// }
// else if (action == FS_RGSTR_ACT_TELEPORT_TO)
// {
//	return (!isSelf && FSRadar::getInstance()->getEntry(av_id) != NULL);
// }

	return false;
}
#endif
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
