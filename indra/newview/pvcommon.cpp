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
#if FIXED_SMART_PTR_ERROR
#include "llnotificationmanager.h"

 void PVCommon::reportToNearbyChat(const std::string& message, const std::string &fromName /* = APP_NAME */, EChatSourceType CHAT_TYPE /* = CHAT_SOURCE_SYSTEM */)
{
	// LLChat chat;
	// chat.mText = message;
	// chat.mFromName = fromName;
	// chat.mSourceType = CHAT_TYPE;
	// LLNotificationsUI::LLNotificationManager::instance().onChat(chat, LLSD());
}

#endif
