// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
* @file oschatcommand.cpp
* @brief OSChatCommand implementation for chat input commands
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Copyright (C) 2013 Drake Arconis
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
**/

#include "llviewerprecompiledheaders.h"

#include "oschatcommand.h"

// lib includes
#include "llcalc.h"
#include "llstring.h"
#include "material_codes.h"
#include "object_flags.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llcommandhandler.h"
#include "llfloaterimnearbychat.h"
#include "llfloaterreg.h"
#include "llfloaterregioninfo.h"
#include "llnotificationsutil.h"
#include "llregioninfomodel.h"
#include "llstartup.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llvolume.h"
#include "llvolumemessage.h"

#ifdef PVDATA_SYSTEM
#include "pvdata.h"
#endif
#include "pvcommon.h"
#include "llappviewer.h"

OSChatCommand::OSChatCommand()
	: LLSingleton<OSChatCommand>()
	, mEnableChatCommands(gSavedSettings, "ObsidianChatCommandEnable", true)
{
	refreshCommands();
	gSavedSettings.getControl("ObsidianChatCommandDrawDistance")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandHeight")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandGround")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandPos")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandRezPlat")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandHome")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandSetHome")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandCalc")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandMapto")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandClearNearby")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandRegionMessage")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandResyncAnim")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandTeleportToCam")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("ObsidianChatCommandHoverHeight")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	// TODO: Do not autocomplete if not special user
	gSavedSettings.getControl("PVChatCommand_PVDataRefresh")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	//gSavedSettings.getControl("PVChatCommand_PVDataDump")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("PVChatCommand_PurgeChat")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
	gSavedSettings.getControl("PVChatCommand_Uptime")->getSignal()->connect(boost::bind(&OSChatCommand::refreshCommands, this));
}

void OSChatCommand::refreshCommands()
{
	mChatCommands.clear();
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandDrawDistance")), CMD_DRAW_DISTANCE);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandHeight")), CMD_GO_TO_HEIGHT);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandGround")), CMD_GO_TO_GROUND);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandPos")), CMD_GO_TO_POS);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandRezPlat")), CMD_REZ_PLAT);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandHome")), CMD_GO_HOME);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandSetHome")), CMD_SET_HOME);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandCalc")), CMD_CALC);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandMapto")), CMD_MAP_TO);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandClearNearby")), CMD_CLEAR_CHAT);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandRegionMessage")), CMD_ESTATE_REGION_MSG);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandResyncAnim")), CMD_RESYNC_ANIM);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandTeleportToCam")), CMD_TP_TO_CAM);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("ObsidianChatCommandHoverHeight")), CMD_HOVER_HEIGHT);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("PVChatCommand_PVDataRefresh")), CMD_PVDATA_REFRESH);
	//mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("PVChatCommand_PVDataDump")), CMD_PVDATA_DUMP); 
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("PVChatCommand_PurgeChat")), CMD_PURGE_CHAT);
	mChatCommands.emplace(utf8str_tolower(gSavedSettings.getString("PVChatCommand_Uptime")), CMD_GET_UPTIME);
}

bool OSChatCommand::matchPrefix(const std::string& in_str, std::string* out_str)
{
	if (!mEnableChatCommands)
		return false;

	std::string str_to_match(utf8str_tolower(in_str));
	size_t in_len = str_to_match.length();

	//return whole trigger, if received text equals to it
	auto it = mChatCommands.find(str_to_match);
	if (it != mChatCommands.end())
	{
		*out_str = it->first;
		return true;
	}

	//return common chars, if more than one cmd matches the prefix
	std::string rest_of_match;
	std::string buf;
	for (auto it = mChatCommands.cbegin(), it_end = mChatCommands.cend(); it != it_end; ++it)
	{
		const std::string cmd = it->first;

		if (in_len > cmd.length())
		{
			// too short, bail out
			continue;
		}

		std::string cmd_trunc = cmd;
		LLStringUtil::truncate(cmd_trunc, in_len);
		if (str_to_match == cmd_trunc)
		{
			std::string cur_rest_of_match = cmd.substr(str_to_match.size());
			if (rest_of_match.empty())
			{
				rest_of_match = cur_rest_of_match;
			}
			buf = LLStringUtil::null;
			size_t i = 0;

			while (i < rest_of_match.length() && i < cur_rest_of_match.length())
			{
				if (rest_of_match[i] == cur_rest_of_match[i])
				{
					buf.push_back(rest_of_match[i]);
				}
				else
				{
					if (i == 0)
					{
						rest_of_match = LLStringUtil::null;
					}
					break;
				}
				i++;
			}
			if (rest_of_match.empty())
			{
				return true;
			}
			else
			{
				rest_of_match = buf;
			}

		}
	}

	if (!rest_of_match.empty())
	{
		*out_str = str_to_match + rest_of_match;
		return true;
	}

	return false;
}

bool OSChatCommand::parseCommand(std::string data)
{
	if (!mEnableChatCommands || data.empty())
		return false;

	std::istringstream input(data);
	std::string cmd;

	if (!(input >> cmd))
		return false;

	EChatCommands command = CMD_UNKNOWN;
	auto it = mChatCommands.find(utf8str_tolower(cmd));
	if (it != mChatCommands.end())
		command = it->second;
	else
		return false;

	switch (command)
	{
	default:
	case CMD_UNKNOWN:
		LL_WARNS() << "Unknown chat command." << LL_ENDL;
		return false;
	case CMD_DRAW_DISTANCE:
	{
		F32 dist;
		if (input >> dist)
		{
			dist = llclamp(dist, 16.f, 8192.f); // <polarity>
			gSavedSettings.setF32("RenderFarClip", dist);
			gAgentCamera.mDrawDistance = dist;
			return true;
		}
		break;
	}
	case CMD_GO_TO_HEIGHT:
	{
		F64 z;
		if (input >> z)
		{
			LLVector3d pos_global = gAgent.getPositionGlobal();
			pos_global.mdV[VZ] = z;
			gAgent.teleportViaLocation(pos_global);
			return true;
		}
		break;
	}
	case CMD_GO_TO_GROUND:
	{
		LLVector3d pos_global = gAgent.getPositionGlobal();
		pos_global.mdV[VZ] = 0.0;
		gAgent.teleportViaLocation(pos_global);
		return true;
	}
	case CMD_GO_TO_POS:
	{
		F64 x, y, z;
		if ((input >> x) && (input >> y) && (input >> z))
		{
			LLViewerRegion* regionp = gAgent.getRegion();
			if (regionp)
			{
				LLVector3d target_pos = regionp->getPosGlobalFromRegion(LLVector3((F32) x, (F32) y, (F32) z));
				gAgent.teleportViaLocation(target_pos);
			}
			return true;
		}
		break;
	}
	case CMD_REZ_PLAT:
	{
		F32 size;
		static LLCachedControl<F32> platSize(gSavedSettings, "ObsidianChatCommandRezPlatSize");
		if (!(input >> size))
			size = static_cast<F32>(platSize);

		const LLVector3& agent_pos = gAgent.getPositionAgent();
		const LLVector3 rez_pos(agent_pos.mV[VX], agent_pos.mV[VY], agent_pos.mV[VZ] - ((gAgentAvatarp->getScale().mV[VZ] / 2.f) + 0.25f + (gAgent.getVelocity().magVec() * 0.333f)));

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectAdd);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU8Fast(_PREHASH_PCode, LL_PCODE_VOLUME);
		msg->addU8Fast(_PREHASH_Material, LL_MCODE_STONE);
		msg->addU32Fast(_PREHASH_AddFlags, agent_pos.mV[VZ] > 4096.f ? FLAGS_CREATE_SELECTED : 0U);

		LLVolumeParams volume_params;
		volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
		volume_params.setBeginAndEndS(0.f, 1.f);
		volume_params.setBeginAndEndT(0.f, 1.f);
		volume_params.setRatio(1.f, 1.f);
		volume_params.setShear(0.f, 0.f);
		LLVolumeMessage::packVolumeParams(&volume_params, msg);

		msg->addVector3Fast(_PREHASH_Scale, LLVector3(size, size, 0.25f));
		msg->addQuatFast(_PREHASH_Rotation, LLQuaternion());
		msg->addVector3Fast(_PREHASH_RayStart, rez_pos);
		msg->addVector3Fast(_PREHASH_RayEnd, rez_pos);
		msg->addUUIDFast(_PREHASH_RayTargetID, LLUUID::null);
		msg->addU8Fast(_PREHASH_BypassRaycast, TRUE);
		msg->addU8Fast(_PREHASH_RayEndIsIntersection, FALSE);
		msg->addU8Fast(_PREHASH_State, FALSE);
		msg->sendReliable(gAgent.getRegionHost());

		return true;
	}
	case CMD_GO_HOME:
	{
		gAgent.teleportHome();
		return true;
	}
	case CMD_SET_HOME:
	{
		gAgent.setStartPosition(START_LOCATION_ID_HOME);
		return true;
	}
	case CMD_CALC:
	{
		if (data.length() > cmd.length() + 1)
		{
			F32 result = 0.f;
			std::string expr = data.substr(cmd.length() + 1);
			LLStringUtil::toUpper(expr);
			if (LLCalc::getInstance()->evalString(expr, result))
			{
				LLSD args;
				args["EXPRESSION"] = expr;
				args["RESULT"] = result;
				LLNotificationsUtil::add("ChatCommandCalc", args);
				return true;
			}
			LLSD args;
			args["MESSAGE"] = LLTrans::getString("ChatCommandCalcFailed");
			LLNotificationsUtil::add("SystemMessageTip", args);
			return true;
		}
		break;
	}
	case CMD_MAP_TO:
	{
		const std::string::size_type length = cmd.length() + 1;
		if (data.length() > length)
		{
			const LLVector3d& pos = gAgent.getPositionGlobal();
			LLSD params;
			params.append(data.substr(length));
			params.append(fmodf(static_cast<F32>(pos.mdV[VX]), REGION_WIDTH_METERS));
			params.append(fmodf(static_cast<F32>(pos.mdV[VY]), REGION_WIDTH_METERS));
			params.append(fmodf(static_cast<F32>(pos.mdV[VZ]), REGION_HEIGHT_METERS));
			LLCommandDispatcher::dispatch("teleport", params, LLSD(), NULL, "clicked", true);
			return true;
		}
		break;
	}
	case CMD_PURGE_CHAT:
	{
		LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
		if (nearby_chat)
		{
			nearby_chat->purgeChatHistory();
			// <polarity> The clear chat command does not reset scroll index
			// as the scroll bar is only reset when a new message is appended to the chat log.
			// We can either update the scrollbar manually, or add a log entry to record that
			// the char was cleared and clear again to hide it away said entry.
			PVCommon::getInstance()->reportToNearbyChat("Clearing chat window...");
			nearby_chat->purgeChatHistory();
			return true;
			// </polarity>
		}
		break;
	}

	case CMD_CLEAR_CHAT:
	{
		LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
		if (nearby_chat)
		{
			nearby_chat->reloadMessages(true);
		}
		return true;
	}
	case CMD_ESTATE_REGION_MSG:
	{
		if (data.length() > cmd.length() + 1)
		{
			std::string notification_message = data.substr(cmd.length() + 1);
			std::vector<std::string> strings(5, "-1");
			// [0] grid_x, unused here
			// [1] grid_y, unused here
			strings[2] = gAgentID.asString(); // [2] agent_id of sender
			// [3] senter name
			std::string name;
			LLAgentUI::buildFullname(name);
			strings[3] = name;
			strings[4] = notification_message; // [4] message
			LLRegionInfoModel::sendEstateOwnerMessage(gMessageSystem, "simulatormessage", LLFloaterRegionInfo::getLastInvoice(), strings);
			return true;
		}
		break;
	}
	case CMD_TP_TO_CAM:
	{
		gAgent.teleportViaLocation(gAgentCamera.getCameraPositionGlobal());
		return true;
	}
	case CMD_HOVER_HEIGHT:
	{
		F32 height;
		if (input >> height)
		{
			gSavedPerAccountSettings.set("AvatarHoverOffsetZ",
				llclamp<F32>(height, MIN_HOVER_Z, MAX_HOVER_Z));
			return true;
		}
		break;
	}
	case CMD_RESYNC_ANIM:
	{
		for (S32 i = 0; i < gObjectList.getNumObjects(); i++)
		{
			LLViewerObject* object = gObjectList.getObject(i);
			if (object && object->isAvatar())
			{
				LLVOAvatar* avatarp = (LLVOAvatar*) object;
				if (avatarp)
				{
					for (const std::pair<LLUUID, S32>& playpair : avatarp->mPlayingAnimations)
					{
						avatarp->stopMotion(playpair.first, TRUE);
						avatarp->startMotion(playpair.first);
					}
				}
			}
		}
		return true;
	}
#ifdef PVDATA_SYSTEM
	case CMD_PVDATA_REFRESH:
	{
		auto pv_agent = PVAgent::find(gAgentID);
		if (pv_agent && pv_agent->isPolarized())
		{
			gPVOldAPI->refreshDataFromServer(true);
		}
		return true;
		break;
	}
		/* Fix later
	case CMD_PVDATA_DUMP:
	{
		gPVData::getInstance()->Dump("Runtime",PVDataOldAPI->mPVData_llsd);
		gPVData::getInstance()->Dump("Runtime", PVDataOldAPI->mPVAgents_llsd);
		return true;
		break;
	}
		*/
#endif
	case CMD_GET_UPTIME:
		{
			PVCommon::getInstance()->reportToNearbyChat(LLAppViewer::secondsToTimeString(gUptimeTimer.getElapsedTimeF32()), "Session Uptime");
			return true;
		}
	}
	return false;
}
