/**
 * @file oschatcommand.h
 * @brief OSChatCommand header for chat input commands
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Obsidian Viewer Source Code
 * Copyright (C) 2015, Drake Arconis.
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

#ifndef OS_CHATCOMMAND_H
#define OS_CHATCOMMAND_H

#include "llsingleton.h"
#include "llcontrol.h"

#include <map>

class OSChatCommand : public LLSingleton<OSChatCommand>
{
	typedef enum EChatCommands
	{
		CMD_DRAW_DISTANCE = 0,
		CMD_GO_TO_HEIGHT,
		CMD_GO_TO_GROUND,
		CMD_GO_TO_POS,
		CMD_REZ_PLAT,
		CMD_GO_HOME,
		CMD_SET_HOME,
		CMD_CALC,
		CMD_MAP_TO,
		CMD_CLEAR_CHAT,
		CMD_ESTATE_REGION_MSG,
		CMD_RESYNC_ANIM,
		CMD_TP_TO_CAM,
		CMD_HOVER_HEIGHT,
		CMD_PURGE_CHAT,
		CMD_PVDATA_REFRESH,
		//CMD_PVDATA_DUMP,
		CMD_GET_UPTIME,
		CMD_SYS_INFO,
		CMD_UNKNOWN
	} e_chat_commands;

	LLSINGLETON(OSChatCommand);

public:
	bool matchPrefix(const std::string& in_str, std::string* out_str);
	bool parseCommand(std::string data);
private:
	void refreshCommands();

	LLCachedControl<bool> mEnableChatCommands;
	std::map<std::string, EChatCommands> mChatCommands;
};

#endif // OS_CHATCOMMAND_H
