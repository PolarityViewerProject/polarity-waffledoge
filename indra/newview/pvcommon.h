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

#ifndef PV_COMMON_H
#define PV_COMMON_H

#pragma once

#include "llchat.h"
#include "llsingleton.h" // for instance()
#include "pvtypes.h"
#include "pvconstants.h"

class LLAvatarName;
class LLViewerObject;

// TODO PLVR: Move these into a class
std::string applyAutoCloseOoc(const std::string& message);
std::string applyMuPose(const std::string& message);
bool isValidWord(const std::string& message);
std::string formatString(std::string text, const LLStringUtil::format_map_t& args);

class PVCommon : public LLSingleton<PVCommon> // required for instance()
{
	LLSINGLETON_EMPTY_CTOR(PVCommon);
	typedef PVCommon _LL_CLASS_TO_LOG;
public:

	/**
	 * \brief Send a message to local chat history, privately to the user
	 * \param message string&
	 * \param fromName string
	 * \param CHAT_TYPE EChatSourceType
	 */
	void reportToNearbyChat(const std::string& message, const std::string &fromName = APP_NAME, EChatSourceType CHAT_TYPE = CHAT_SOURCE_SYSTEM);

	/** \brief Convert a string to a char array and check for special characters presence.
	 *
	 * We use this to check of the user is trying to send ASCII art and short-circuit
	 * the OOC logic if that's the case.
	 */
	static int HasSpecialCharacters(const std::string& oocstring);

	/** \brief Keep track of ObjectAdd messages sent to the simulator.
	 *
	 * HACK: This is used to work around a LL design flaw of the similular returning the same object update packet
	 * for _PREHASH_ObjectAdd, _PREHASH_RezObject, and _PREHASH_RezObjectFromNotecard.
	 *
	 */
	static  S32 sObjectAddMsg;

	// static bool checkIsActionEnabled(const LLUUID& av_id, EFSRegistrarFunctionActionType);
	static LLSD populateGroupCount();

	static bool isDefaultTexture(const LLUUID& asset_id);

	/** \brief Convert a string of a specified date format into seconds since the Epoch.
	 *
	 * Many of the format flags are those used by strftime(...), but not all.
	 * For the full list of supported time format specifiers
	 * see http://www.boost.org/doc/libs/1_47_0/doc/html/date_time/date_time_io.html#date_time.format_flags
	 *
	 * time support added by Techwolf Lupindo
	 *
	 * @param format Format characters string. Example: "%A %b %d, %Y"
	 * @param str    Date string containing the time in specified format.
	 *
	 * @return Number of seconds since 01/01/1970 UTC.
	 */
	static S32 secondsSinceEpochFromString(const std::string& format, const std::string& str);

	static bool isAVXSupported();

	/* \brief Wrapper around LLStringUtil::format.
	 *
	 * I'm not sure we we need this yet.
	 */
	static std::string format_string(std::string text, const LLStringUtil::format_map_t& args);

	/**
	 * \brief Attempt to set the chat logs location from environment if available
	 */
	static void getChatLogsDirOverride();
	bool moveTranscriptsAndLog(const std::string &userid) const;

private:
	static bool sAVX_Checked;
	static bool sAVXSupported;

};

/// <summary> Cached instance. Use this or findInstance() instead of getInstance if you can. </summary>
extern PVCommon* gPVCommon;

#endif // PV_COMMON_H
