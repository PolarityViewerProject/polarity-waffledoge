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

class PVCommon
{
public:

	/**
	 * \brief Send a message to local chat history, privately to the user
	 * \param message string&
	 * \param fromName string
	 * \param CHAT_TYPE EChatSourceType
	 */
	static void reportToNearbyChat(const std::string& message, const std::string &fromName = "", EChatSourceType CHAT_TYPE = CHAT_SOURCE_SYSTEM);
};

#endif // PV_COMMON_H
