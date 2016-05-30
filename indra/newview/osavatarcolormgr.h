/**
 * @file oschatcommand.h
 * @brief OSAvatarColorMgr header for avatar color selection
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
 * $/LicenseInfo$
 */

#pragma once

#include "llsingleton.h"

#include <map>

class LLColor4;
class LLUUID;

class OSAvatarColorMgr : public LLSingleton < OSAvatarColorMgr >
{
private:
	friend class LLSingleton < OSAvatarColorMgr > ;
public:
	typedef enum e_custom_colors
	{
		E_FIRST_COLOR = 0,
		E_SECOND_COLOR,
		E_THIRD_COLOR,
		E_FOURTH_COLOR
	} EAvatarColors;

	OSAvatarColorMgr(){}
	~OSAvatarColorMgr(){}

	void addOrUpdateCustomColor(const LLUUID& id, EAvatarColors color);
	void clearCustomColor(const LLUUID& id);
	const LLColor4& getColor(const LLUUID& id);

private:
	typedef std::map<LLUUID, EAvatarColors> uuid_color_map_t;
	uuid_color_map_t mCustomColors;
};
