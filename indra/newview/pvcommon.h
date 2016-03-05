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

#ifndef PV_COMMON_H
#define PV_COMMON_H

#include "llchat.h"

class LLAvatarName;
class LLViewerObject;

void reportToNearbyChat(const std::string& message);
// <Polarity>
void reportSpecialToNearbyChat(const std::string& message, EChatSourceType CHAT_TYPE, std::string fromName);
bool isPolarityDeveloper(const LLUUID& av_id);
// </Polarity>

std::string applyAutoCloseOoc(const std::string& message);
std::string applyMuPose(const std::string& message);
std::string formatString(std::string text, const LLStringUtil::format_map_t& args);

class PVCommon
{
public:
	/**
	* Convert a string to a char array and check for special characters presence.
	* We use this to check of the user is trying to send ASCII art and short-circuit
	* the OOC logic if that's the case.
	*/
	static int HasSpecialCharacters(const std::string& oocstring); \

	// apply default build preferences to the object
		static void applyDefaultBuildPreferences(LLViewerObject* object);

	static bool isLinden(const LLUUID& av_id);

	/**
	 * HACK
	 *
	 * This is used to work around a LL design flaw of the similular returning the same object update packet
	 * for _PREHASH_ObjectAdd, _PREHASH_RezObject, and _PREHASH_RezObjectFromNotecard.
	 *
	 * keep track of ObjectAdd messages sent to the similular.
	 */
	static  S32 sObjectAddMsg;

	// static bool checkIsActionEnabled(const LLUUID& av_id, EFSRegistrarFunctionActionType);
	static LLSD populateGroupCount();

	static bool isDefaultTexture(const LLUUID& asset_id);

	static bool isAVXSupported();
private:
	static bool sAVX_Checked;
	static bool sAVXSupported;
};

#endif // PV_COMMON_H
