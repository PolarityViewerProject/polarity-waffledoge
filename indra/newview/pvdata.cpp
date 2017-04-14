// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvdata.cpp
 * @brief Downloadable metadata for viewer features.
 * Inspired by FSData by Techwolf Lupindo
 * Re-implented by Xenhat Liamano
 *
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include <stdlib.h> // for setenv
#include "pvdata.h"
#include "pvcommon.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloaterabout.h"
#include "llfloaterpreference.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llprogressview.h"
#include "llsdserialize.h"
#include "lltrans.h" // for getString
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "noise.h"
#include "pvpanellogin.h"

#include "rlvactions.h"

#include <boost/regex.hpp>

#include "pvtl.h" // for vector_to_string

#define LL_LINDEN "Linden"
#define LL_MOLE "Mole"
#define LL_PRODUCTENGINE "ProductEngine"
#define LL_SCOUT "Scout"
#define LL_TESTER "Tester"

PVSearchUtil*		gPVSearchUtil = NULL;
PVDataOldAPI*		gPVOldAPI = nullptr;

// Static initialization
std::string PVDataOldAPI::pvdata_url_full_ = "";
std::string PVDataOldAPI::pvdata_agents_url_full_ = "";
bool PVDataOldAPI::mBeggarCheckEnabled(true);

inline const std::string bts(bool b)
{
	return b ? "true" : "false";
}

template <typename T>
std::string pvitoa(T Number)
{
	std::stringstream ss;
	ss << Number;
	return ss.str();
}

#if LL_DARWIN
size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return(p ? p-s : n);
}
#endif // LL_DARWIN

void PVDataOldAPI::Dump(const std::string &name, const LLSD &map)
{
	std::stringstream str;
	LLSDSerialize::toPrettyXML(map, str);
	LL_DEBUGS("PVDebug") << "\n===========================\n<!--  <" + name + "> -->\n" + str.str() + "\n<!--  </" + name + "> -->\n===========================\n" << LL_ENDL;
}

// ########   #######  ##      ## ##    ## ##        #######     ###    ########  ######## ########
// ##     ## ##     ## ##  ##  ## ###   ## ##       ##     ##   ## ##   ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ####  ## ##       ##     ##  ##   ##  ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ## ## ## ##       ##     ## ##     ## ##     ## ######   ########
// ##     ## ##     ## ##  ##  ## ##  #### ##       ##     ## ######### ##     ## ##       ##   ##
// ##     ## ##     ## ##  ##  ## ##   ### ##       ##     ## ##     ## ##     ## ##       ##    ##
// ########   #######   ###  ###  ##    ## ########  #######  ##     ## ########  ######## ##     ##

bool PVDataOldAPI::can_proceed(U8& status_container) const
{
	LL_DEBUGS("PVDebug") << "Checking status" << LL_ENDL;
	switch (status_container)
	{
	case UNDEFINED:
		// try again!
		status_container = READY;
		return can_proceed(status_container);
		break;
	case READY:
		return true;
	case DOWNLOAD_IN_PROGRESS:
		LL_WARNS("PVData") << "Download already in progress, aborting." << LL_ENDL;
		return false;
	case DOWNLOAD_FAILURE:
		LL_WARNS("PVData") << "Download failed. Will retry later." << LL_ENDL;
		return true;
	case DOWNLOAD_OK:
		LL_DEBUGS("PVDebug") << "Download success, proceeding." << LL_ENDL;
		return true;
	case PARSING_IN_PROGRESS:
		LL_WARNS("PVData") << "Parser is already running, aborting." << LL_ENDL;
		return false;
	case PARSE_FAILURE:
		LL_WARNS("PVData") << "Parse failed. This is bad." << LL_ENDL;
		return false;
	default:
		LL_WARNS("PVData") << "PVData encountered a problem and has aborted. (STATUS='" << status_container << "')" << LL_ENDL;
		status_container = UNDEFINED;
		return false;
	}
}

void PVDataOldAPI::downloadData()
{
	if (can_proceed(pv_data_status_))
	{
		pv_data_status_ = DOWNLOAD_IN_PROGRESS;
		modularDownloader(data_file);
	}
	downloadAgents(); // chain-load
}

void PVDataOldAPI::downloadAgents()
{
	if (can_proceed(pv_agents_status_))
	{
		pv_agents_status_ = DOWNLOAD_IN_PROGRESS;
		modularDownloader(agents_file);
	}
}

void PVDataOldAPI::modularDownloader(const S8& pfile_name_in)
{
	//@todo: Move some place that won't get run twice

	pv_file_name_data_string_ = pv_file_names.find(data_file)->second;
	pv_file_name_agents_string_ = pv_file_names.find(agents_file)->second;

	// Sets up the variables we need for each object. Avoids call bloat in the class constructor.
	pvdata_user_agent_ = LLViewerMedia::getCurrentUserAgent();
	pvdata_viewer_version_ = LLVersionInfo::getChannelAndVersionStatic();
	static LLCachedControl<bool> pvdata_testing_branch(gSavedSettings, "PVData_UseTestingDataSource", false);
	if (pv_url_remote_base_string_ == "" || pvdata_testing_branch != pv_downloader_testing_branch)
	{
		pv_url_remote_base_string_ = "https://data.polarityviewer.org/" + (pvdata_testing_branch ? std::string("test/") : std::string("live/")) + std::to_string(pvdata_file_version) + "/";
		pv_downloader_testing_branch = pvdata_testing_branch;
	}

	// construct download url from file name
	headers_.insert("User-Agent", pvdata_user_agent_);
	headers_.insert("viewer-version", pvdata_viewer_version_);
	auto iterator = pv_file_names.find(pfile_name_in);
	std::string requested_file = iterator->second;

	if (requested_file == pv_file_name_data_string_)
	{
		pvdata_url_full_ = pv_url_remote_base_string_ + requested_file;
	}
	else if (requested_file == pv_file_name_agents_string_)
	{
		pvdata_agents_url_full_ = pv_url_remote_base_string_ + requested_file;
	}

	LL_DEBUGS("PVDebug") << "Downloading " + requested_file + " from " + pv_url_remote_base_string_ + requested_file<< LL_ENDL;
	//@todo: HTTP eTag support
	LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(pv_url_remote_base_string_ + requested_file,
		boost::bind(downloadComplete, _1, pv_url_remote_base_string_ + requested_file),
		boost::bind(downloadError, _1, pv_url_remote_base_string_ + requested_file));
}

void PVDataOldAPI::downloadComplete(const LLSD& aData, std::string& aURL)
{
	LLSD header = aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS][LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

	LLDate lastModified;
	if (header.has("last-modified"))
	{
		lastModified.secondsSinceEpoch(PVCommon::secondsSinceEpochFromString("%a, %d %b %Y %H:%M:%S %ZP", header["last-modified"].asString()));
	}

	LLSD data = aData;
	data.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

	handleResponseFromServer(data, aURL, false);
}

void PVDataOldAPI::downloadError(const LLSD& aData, std::string& aURL)
{
	LL_WARNS("PVData") << "Failed to download " << aURL << ": " << aData << LL_ENDL;
	handleResponseFromServer(aData, aURL, true);
}

void PVDataOldAPI::handleResponseFromServer(const LLSD& http_content,
	const std::string& http_source_url,
	const bool& download_failed
)
{
	static LLCachedControl<bool> dump_web_data(gSavedSettings, "PVDebug_DumpWebData", false);
	if (dump_web_data)
	{
		Dump(http_source_url, http_content);
	}
	LL_DEBUGS("PVDebug") << "http_content=" + http_content.asString()<< LL_ENDL;

	if (http_source_url == pvdata_url_full_)
	{
		LL_DEBUGS("PVDebug") << "Got DATA file" << LL_ENDL;
		gPVOldAPI->setDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS("PVData") << "DATA Download failure, aborting." << LL_ENDL;
			gPVOldAPI->setDataStatus(DOWNLOAD_FAILURE);
			gPVOldAPI->handleDataFailure();
		}
		else
		{
			gPVOldAPI->setDataStatus(DOWNLOAD_OK);
			gPVOldAPI->parsePVData(http_content);
		}
	}
	else if (http_source_url == pvdata_agents_url_full_)
	{
		LL_DEBUGS("PVDebug") << "Got AGENTS file" << LL_ENDL;
		gPVOldAPI->setAgentsDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS("PVData") << " AGENTS Download failure, aborting." << LL_ENDL;
			gPVOldAPI->setAgentsDataStatus(DOWNLOAD_FAILURE);
			gPVOldAPI->handleAgentsFailure();
		}
		else
		{
			gPVOldAPI->setAgentsDataStatus(DOWNLOAD_OK);
			//pv_agents_status_ = INIT; // Don't reset here, that would defeat the purpose.
			gPVOldAPI->parsePVAgents(http_content);
		}
	}
	else
	{
		LL_DEBUGS("PVDebug") << "Got SOMETHING we weren't expecting. what do?" << LL_ENDL;
	}
	LLPanelLogin::doLoginButtonLockUnlock();
}

// ########     ###    ########   ######  ######## ########   ######
// ##     ##   ## ##   ##     ## ##    ## ##       ##     ## ##    ##
// ##     ##  ##   ##  ##     ## ##       ##       ##     ## ##
// ########  ##     ## ########   ######  ######   ########   ######
// ##        ######### ##   ##         ## ##       ##   ##         ##
// ##        ##     ## ##    ##  ##    ## ##       ##    ##  ##    ##
// ##        ##     ## ##     ##  ######  ######## ##     ##  ######

void PVDataOldAPI::parsePVData(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset pv_data_status_ when parsing is needed again.
	if (!can_proceed(pv_data_status_))
	{
		// FIXME: why do we get 'pv_data_status_==PARSING' BEFORE it's actually being set? (see below)
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}
	LL_DEBUGS("PVDebug") << "Beginning to parse Data" << LL_ENDL;
	pv_data_status_ = PARSING_IN_PROGRESS;

	std::string section = pv_data_sections_.at(MinimumVersion);

	//@todo: Loop through sections and check section type to determine validity?
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		for (LLSD::map_const_iterator iter = blob.beginMap(); iter != blob.endMap(); ++iter)
		{
			auto version = iter->first;
			auto reason = iter->second;
			blob[version] = reason;
			minimum_version_[version] = reason;
			LL_DEBUGS("PVDebug") << "Minimum Version is " + version<< LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL;
	}

	section = pv_data_sections_.at(PVDataOldAPI::BlockedReleases);
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		LL_DEBUGS("PVDebug") << "Populating Blocked Releases list..." << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		setBlockedVersionsList(blob);
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL;
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	section = pv_data_sections_.at(PVDataOldAPI::MOTD);
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		gAgent.mMOTD.assign(blob);
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL; // Don't warn on this one
	}

#if PVDATA_MOTD_CHAT
	section = pv_data_sections_.at(ChatMOTD);
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		LLSD::array_const_iterator iter = blob.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(blob.size()))))->asString());
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL;
	}
#endif // PVDATA_MOTD_CHAT

	section = pv_data_sections_.at(EventsMOTD);
	// If the event falls within the current date, use that for MOTD instead.
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		setMotdEventsList(blob);
		auto event_motd = getEventMotdIfAny();
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL; // don't warn on this one
	}
#endif // PVDATA_MOTD

#if PVDATA_PROGRESS_TIPS
	section = pv_data_sections_.at(ProgressTip);
	//@todo: Split tips files
	// Load the progress screen tips
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		setProgressTipsList(blob);
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL;
	}

#endif // PVDATA_PROGRESS_TIPS

	section = pv_data_sections_.at(WindowTitles);
	LL_DEBUGS("PVDebug") << "Attempting to find " + section<< LL_ENDL;
	if (data_input.has(section))
	{
		LL_DEBUGS("PVDebug") << "Found " + section + "!" << LL_ENDL;
		auto blob = data_input[section];
		Dump(section, blob);
		setWindowTitlesList(blob);
	}
	else
	{
		LL_DEBUGS("PVDebug") << "No " + section + " found!" << LL_ENDL;
	}
	pv_data_status_ = READY;
	mPVData_llsd = data_input;
	LL_INFOS("PVData") << "Done parsing data" << LL_ENDL;
}

void PVDataOldAPI::addAgents(const LLSD& data_input)
{
	if (data_input.has("SpecialAgentsList"))
	{
		const LLSD& special_agents_llsd = data_input["SpecialAgentsList"];

		for (LLSD::map_const_iterator uuid_iterator = special_agents_llsd.beginMap();
			uuid_iterator != special_agents_llsd.endMap(); ++uuid_iterator)
		{
			LLUUID uuid;
			auto uuid_str = uuid_iterator->first;
			if (LLUUID::validate(uuid_str))
			{
				//create new data blob for this agent.
				//@note Is there a less leak-prone way to do this?
				auto this_agent = new PVAgent();

				LLUUID::parseUUID(uuid_str, &uuid);
				this_agent->uuid = uuid;

				const LLSD& data_map = uuid_iterator->second;
				if (data_map.has("Access") && data_map["Access"].type() == LLSD::TypeInteger)
				{
					//if (this_agent->flags & DEPRECATED_TITLE_OVERRIDE)
					//{
					//	this_agent->flags &= (~DEPRECATED_TITLE_OVERRIDE);
					//}
					this_agent->flags = data_map["Access"].asInteger();
				}
				if (data_map.has("HexColor") && data_map["HexColor"].type() == LLSD::TypeString)
				{
					this_agent->color = Hex2Color4(data_map["HexColor"].asString());
				}
				if (data_map.has("Title") && data_map["Title"].type() == LLSD::TypeString)
				{
					this_agent->title = data_map["Title"].asString();
				}
				if (data_map.has("BanReason") && data_map["BanReason"].type() == LLSD::TypeString)
				{
					this_agent->ban_reason = data_map["BanReason"].asString();
				}
				pvAgents.emplace(uuid, this_agent);
			}
		}
	}
}

void PVDataOldAPI::parsePVAgents(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset pv_data_status_ when parsing is needed again.
	if (!can_proceed(pv_agents_status_))
	{
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	pv_agents_status_ = PARSING_IN_PROGRESS;
	LL_INFOS("PVData") << "Beginning to parse Agents" << LL_ENDL;

	LL_DEBUGS("PVDebug") << "Attempting to find Agents root nodes" << LL_ENDL;
	addAgents(data_input);
	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			setVendorSupportGroup(LLUUID(itr->first));
			LL_DEBUGS("PVDebug") << "Added " + itr->first + " to support_group_" << LL_ENDL;
		}
	}

	mPVAgents_llsd = data_input;
	pv_agents_status_ = PVDataOldAPI::READY;
	LL_INFOS("PVData") << "Done parsing agents" << LL_ENDL;

	autoMuteFlaggedAgents();
}

void PVDataOldAPI::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS("PVData") << "Something went wrong downloading data file" << LL_ENDL;
	gAgent.mMOTD.assign("COULD NOT CONTACT MOTD SERVER");
	pv_data_status_ = DOWNLOAD_FAILURE;
}

void PVDataOldAPI::setFallbackAgentsData()
{
}

void PVDataOldAPI::handleAgentsFailure()
{
	LL_WARNS("PVData") << "Something went wrong downloading agents file" << LL_ENDL;
	setFallbackAgentsData();
	pv_agents_status_ = DOWNLOAD_FAILURE;
}

bool PVDataOldAPI::isVersionUnderMinimum()
{
	if (minimum_version_.size() == NULL) // empty!
	{
		// this will only happen if data download failed, so let's just thwart that.
		return true;
	}

	LL_DEBUGS("PVDebug") << "Parsing version..." << LL_ENDL;
	if (version_string_as_long(LLVersionInfo::getVersion()) >= version_string_as_long(minimum_version_.begin()->first))
	{
		return false;
	}

	setErrorMessage(minimum_version_.begin()->second["REASON"]);
	return true;
}

bool PVDataOldAPI::getDataDone()
{
	if (pv_data_status_ == READY)
	{
		return true;
	}
	return false;
}

bool PVDataOldAPI::getAgentsDone()
{
	if (pv_agents_status_ == READY)
	{
		return true;
	}
	return false;
}

std::string PVDataOldAPI::getNewProgressTip(bool forced)
{
	LL_DEBUGS("PVDebug") << "Entering function" << LL_ENDL;
	// Use the last tip if available
	std::string return_tip = "";
	if (last_login_tip != "")
	{
		return_tip = last_login_tip;
	}
	static LLCachedControl<F32> progress_tip_timout(gSavedSettings, "PVUI_ProgressTipTimer", 2.f);
	if (forced || (mTipCycleTimer.getStarted() && mTipCycleTimer.getElapsedTimeF32() >= progress_tip_timout))
	{
		LL_DEBUGS("PVDebug") << "mTipCycleTimer elapsed; getting a new random tip" << LL_ENDL;
		LL_DEBUGS("PVDebug") << "Last tip was '" << last_login_tip << "'" << LL_ENDL;

		// Most likely a teleport screen; let's add something.

		// Check for events MOTD first...
		return_tip = getEventMotdIfAny();
		if (return_tip == "")
		{
			return_tip = progress_tips_list_.getRandom();
			LL_INFOS("PVData") << "New tip from function is '" << return_tip << "'" << LL_ENDL;
		}

		if (!return_tip.empty() && return_tip != last_login_tip)
		{
			LL_INFOS("PVData") << "Setting new progress tip to '" << return_tip << "'" << LL_ENDL;
			last_login_tip = return_tip;
		}
		mTipCycleTimer.reset();
	}
	else
	{
		LL_DEBUGS("PVDebug") << "mTipCycleTimer not started!" << LL_ENDL;
	}

	gAgent.mMOTD = return_tip;
	return return_tip;
}

std::string PVDataOldAPI::getRandomWindowTitle()
{
	LL_DEBUGS("PVDebug") << "Getting random window title from this list:" << LL_ENDL;
	Dump("window_titles_list_", gPVOldAPI->window_titles_list_);
	std::string title = gPVOldAPI->window_titles_list_.getRandom();
	LL_DEBUGS("PVDebug") << "Returning  '" + title + "'" << LL_ENDL;
	return title;
}

void PVDataOldAPI::setVendorSupportGroup(const LLUUID& uuid)
{
	support_group_.insert(uuid);
}

void PVDataOldAPI::autoMuteFlaggedAgents()
{
	if (!gCacheName)
	{
		return;
	}
	//@todo re-implement
#if NEW_API_DONE
	for (std::map<LLUUID, S32>::iterator iter = pv_special_agent_flags_.begin(); iter != pv_special_agent_flags_.end(); ++iter)
	{
		if (iter->second & BAD_USER_AUTOMUTED)
		{
			LLUUID id = iter->first;
			std::string name;
			gCacheName->getFullName(id, name);
			LLMute mute(id, name, LLMute::EXTERNAL);
			LLMuteList::getInstance()->add(mute);
		}
	}
#endif
}

LLUUID PVDataOldAPI::getLockDownUUID()
{
// Workaround for missing CMAKE flags
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

// Remember to set STRING type in CMAKE
#ifndef PVDATA_UUID_LOCKTO
#define PVDATA_UUID_LOCKTO ""
#endif

	std::string temp = TOSTRING(PVDATA_UUID_LOCKTO);
	if (temp.length() != UUID_STR_LENGTH)
	{
		return LLUUID::null;
	}
	return static_cast<LLUUID>(temp);
}

bool PVAgent::isAllowedToLogin(const LLUUID& avatar_id)
{
	bool allowed = false;
	LL_INFOS("PVData") << "Evaluating access for " << avatar_id << "..." << LL_ENDL;

	gPVOldAPI->setErrorMessage("Generic PVData authentication failure (Please report this bug to the [APP_NAME] developers).");
#if PVDATA_UUID_LOCKDOWN
	LLUUID lockdown_uuid = getLockDownUUID();
	if (lockdown_uuid != LLUUID::null)
	{
		LL_INFOS("PVData") << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			allowed = true;
			LL_INFOS("PVData") << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
		}
		else
		{
			allowed = false;
			setErrorMessage("This build is locked down to another account.");
		}
	}
	if (lockdown_uuid != LLUUID::null)
	{
		allowed = false;
		setErrorMessage("Something went wrong, and the authentication checks have failed.");
	}
#endif
	auto agentPtr = find(avatar_id);
	if (agentPtr)
	{
		allowed = !agentPtr->isProviderBanned(true);
		LL_WARNS("PVData") << "HERE ARE YOUR FLAGS: " << agentPtr->getTitle(false) << LL_ENDL;
#if INTERNAL_BUILD
		LL_WARNS("PVData") << "Internal build, evaluating access for " << avatar_id << "'..." << LL_ENDL;
		if (agentPtr->isProviderDeveloper()
			|| agentPtr->isProviderSupportTeam()
			|| agentPtr->isProviderQATeam()
			|| agentPtr->isProviderTester())
		{
			allowed = true;
		}
		else
		{
			allowed = false; // safety never hurt anyone
		}
#endif
	}
	else
	{
#if INTERNAL_BUILD
		LL_WARNS("PVData") << "Access level: NONE" << LL_ENDL;
		gPVOldAPI->setErrorMessage("You do not have clearance to use this internal build of [APP_NAME].\nIf you believe this to be a mistake, contact the [APP_NAME] Viewer support. Otherwise, please download a public build at\n" + LLTrans::getString("ViewerDownloadURL") + ".");
		allowed = false;
#else
		allowed = true;
#endif
	}

	return allowed;
}

std::vector<int> split_version(const char *str, char separator = '.')
{
	std::vector<int> result;

	do
	{
		auto *begin = str;

		while (*str != separator && *str)
			str++;

		result.push_back(atoi(std::string(begin, str).c_str()));
	} while (0 != *str++);

	return result;
}

bool PVDataOldAPI::isBlockedRelease()
{
	static S32 blocked = -1;
	if (blocked >= 0)
	{
		return blocked;
	}

	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	auto blockedver_iterator = blocked_versions_.find(sCurrentVersion);

	setErrorMessage("Quit living in the past!");

	if (isVersionUnderMinimum())
	{
		blocked = TRUE;
		return true;
	}

	// Check if version is explicitly blocked
	if (blockedver_iterator == blocked_versions_.end()) // if the iterator's value is map::end, it is not in the array.
	{
		LL_DEBUGS("PVDebug") << sCurrentVersion << " not found in the blocked releases list" << LL_ENDL;
		setErrorMessage("");
		blocked = FALSE;
		return false;
	}

	// assign the iterator's associated value (the reason message) to the LLSD that will be returned to the calling function
	const LLSD& reason_llsd = blockedver_iterator->second;
		setErrorMessage(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << getErrorMessage() << ")" << LL_ENDL;
	LLFloaterAboutUtil::checkUpdatesAndNotify();
	blocked = TRUE;
	return true;
}

bool PVAgent::isLinden(const std::string& last_name)
{
	return last_name == LL_LINDEN;
}

bool PVAgent::isMole(const std::string& last_name)
{
	return last_name == LL_MOLE;
}

bool PVAgent::isProductEngine(const std::string& last_name)
{
	return last_name == LL_PRODUCTENGINE;
}

bool PVAgent::isScout(const std::string& last_name)
{
	return last_name == LL_SCOUT;
}

bool PVAgent::isLLTester(const std::string& last_name)
{
	return last_name == LL_TESTER;
}

void PVDataOldAPI::startRefreshTimer()
{
	if (!pvdata_refresh_timer_.getStarted())
	{
		LL_INFOS("PVData") << "Starting PVData refresh timer" << LL_ENDL;
		pvdata_refresh_timer_.start();
	}
	else
	{
		LL_WARNS("PVData") << "Timer already started!" << LL_ENDL;
	}
}

void PVDataOldAPI::refreshDataFromServer(bool force_refresh_now)
{
	// paranoia check
	if(!pvdata_refresh_timer_.getStarted())
	{
		return;
	}
	static LLCachedControl<U32> refresh_minutes(gSavedSettings, "PVData_RefreshTimeout", 60); // Minutes
	if (force_refresh_now || pvdata_refresh_timer_.getElapsedTimeF32() >= refresh_minutes * 60)
	{
		LL_INFOS("PVData") << "Attempting to live-refresh PVDataOldAPI" << LL_ENDL;
		downloadData();

		//LL_DEBUGS("PVDebug") << "Attempting to live-refresh Agents data" << LL_ENDL;
		//downloadAgents();
		if (!force_refresh_now)
		{
			LL_DEBUGS("PVDebug") << "Resetting timer" << LL_ENDL;
			pvdata_refresh_timer_.reset();
		}
	}
}

LLColor4 PVDataOldAPI::Hex2Color4(const std::string &color) const
{
	return Hex2Color4(stoul(color, nullptr, 16));
}

LLColor4 PVDataOldAPI::Hex2Color4(int hexValue)
{
	auto r = ((hexValue >> 16) & 0xFF) / 255.0f;  // Extract the RR byte
	auto g = ((hexValue >> 8) & 0xFF) / 255.0f;   // Extract the GG byte
	auto b = ((hexValue) & 0xFF) / 255.0f;        // Extract the BB byte
	return LLColor4(r, g, b, 1.0f);
}

void PVDataOldAPI::cleanup()
{
	if(!pvdata_refresh_timer_.getStarted())
	{
		return;
	}
	pvdata_refresh_timer_.stop();
}

U32 PVSearchUtil::PVSearchSeparatorSelected = gPVSearchUtil->separator_space;

U32 PVSearchUtil::getSearchSeparatorFromSettings()
{
	static LLCachedControl<U32> settings_separator(gSavedSettings, "PVUI_SubstringSearchSeparator", separator_space);
	LL_DEBUGS("PVDataOldAPI") << "Search separator index from settings: '" << settings_separator << "'" << LL_ENDL;
	return settings_separator;
}

void PVSearchUtil::setSearchSeparator(const U32 separator_in_u32)
{
	PVSearchSeparatorSelected = separator_in_u32;
	LL_DEBUGS("PVDataOldAPI") << "Setting search separator to '" << separator_in_u32 << "'" << "('" << getSearchSeparator() << "')" << LL_ENDL;
	gSavedSettings.setU32("PVUI_SubstringSearchSeparator", separator_in_u32);

}

std::string PVSearchUtil::getSearchSeparator()
{
	auto separator = gPVSearchUtil->PVSearchSeparatorAssociation[PVSearchSeparatorSelected];
	LL_DEBUGS("PVDataOldAPI") << "Search separator from runtime: '" << separator << "'" << LL_ENDL;
	return separator;
}

std::string PVSearchUtil::getSearchSeparator(const U32 separator_to_get_u32) const
{
	PVSearchSeparatorSelected = separator_to_get_u32;
	return getSearchSeparator();
}

//@todo replace with proper code fixes
std::string PVDataOldAPI::getPreferredName(const LLAvatarName& av_name)
{
	static LLCachedControl<bool> show_username(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> use_display_names(gSavedSettings, "UseDisplayNames");
	// Fallback
	std::string preferred_name = "";

	if (!use_display_names && !show_username)
	{
		return av_name.getUserName();
	}
	if (use_display_names && show_username)
	{
		preferred_name = av_name.getCompleteNameForced(); // Show everything
	}
	else if (use_display_names && !show_username)
	{
		preferred_name = av_name.getDisplayNameForced();
	}

	if (preferred_name == "")
	{
		// we shouldn't hit this, but a sane fallback can't hurt.
		preferred_name = av_name.getUserName();
		LL_WARNS("PVData") << "Preferred Name was unavailable, returning '" << preferred_name << "'" << LL_ENDL;
	}

	return preferred_name;
}

#if LL_WINDOWS
// Microsoft's runtime library doesn't support the standard setenv() function.
// http://stackoverflow.com/a/23616164
int setenv(const char *name, const char *value, int overwrite)
{
	int errcode = 0;
	if (!overwrite) {
		size_t envsize = 0;
		errcode = getenv_s(&envsize, NULL, 0, name);
		if (errcode || envsize) return errcode;
	}
	return _putenv_s(name, value);
}

#endif // LL_WINDOWS

std::string getRegKey(const std::string& name_) {
	//@todo implement failsafe
	// README: This assumes the variable is set.
	//setenv("PV_CHATLOGS_LOCATION_OVERRIDE", gSavedPerAccountSettings.getString("InstantMessageLogPath").c_str(), 1);
	// Borrowed from editenv.dll by Dan Moulding (Visual Leak Detector's author)

	//PBYTE       data;

	char *		data = nullptr;
	HKEY        key = HKEY_CURRENT_USER;
	DWORD		size = 1024 * sizeof(TCHAR);
	//LONG        status;
	HKEY        subKey;
	std::string value = "";

	long ret;
	ret = RegOpenKeyExA(HKEY_CURRENT_USER, "\\Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &subKey);
	RegQueryValueExA(subKey, name_.c_str(), 0, NULL, NULL, &size);
	if (ret != ERROR_SUCCESS) {
		LL_WARNS("PVData") << "Key [" << name_ << "] does not exist!" << LL_ENDL;
		return std::string();
	}
	// This environment variable already exists.
	ret = RegQueryValueExA(key, name_.c_str(), 0, 0, reinterpret_cast<LPBYTE>(data), &size);
	if (ret != ERROR_SUCCESS)
	{
		return std::string();
	}
	LL_WARNS("PVData") << "Key [" << name_ << "] exist!" << LL_ENDL;
	if (data != nullptr)
	{
		value = std::string(data);
	}
	delete[] data;

	LL_WARNS("PVData") << "Key [" << value << "] = exist!" << LL_ENDL;
	RegCloseKey(key);
	//@todo: Linux and OSX support
	return value;
}

void PVDataOldAPI::getChatLogsDirOverride()
{
	const std::string log_location_from_settings = gSavedPerAccountSettings.getString("InstantMessageLogPath");
	// ReSharper disable CppDeprecatedEntity // cross-platform needs std:: function
	char* log_location_from_registry = getenv("PV_CHATLOGS_LOCATION_OVERRIDE");

	auto log_location_from_runtime = gDirUtilp->getChatLogsDir();
	std::string new_chat_logs_dir = "";
	if (log_location_from_settings.empty() || log_location_from_registry == NULL)
	{
		new_chat_logs_dir = gDirUtilp->getOSUserAppDir();
	}
	else if (log_location_from_registry != NULL && log_location_from_registry[0] != '\0')
	{

		new_chat_logs_dir = log_location_from_registry;
	}
	//if (new_chat_logs_dir != log_location_from_settings || gDirUtilp->getChatLogsDir() != log_location_from_registry)
	//{
	LL_DEBUGS("PVDebug") << "Would set logs location to: " + new_chat_logs_dir<< LL_ENDL;
	LL_DEBUGS("PVDebug") << "gDirUtilp->getChatLogsDir() = " + gDirUtilp->getChatLogsDir()<< LL_ENDL;

	LL_WARNS("PVData") << "Chat log location = " << new_chat_logs_dir << LL_ENDL;
	//}
	if (new_chat_logs_dir.empty())
	{
		LL_ERRS() << "new_chat_logs_dir is null!" << LL_ENDL;
	}
	else if (new_chat_logs_dir == "")
	{
		LL_ERRS() << "new_chat_logs_dir is empty!" << LL_ENDL;
	}
	else
	{
		gDirUtilp->setChatLogsDir(new_chat_logs_dir);
	}

	if (new_chat_logs_dir != gDirUtilp->getChatLogsDir())
	{
		LL_DEBUGS("PVDebug") << "Hmmm strange, location mismatch: " + new_chat_logs_dir + " != " + gDirUtilp->getChatLogsDir()<< LL_ENDL;
	}

	gSavedPerAccountSettings.setString("InstantMessageLogPath", new_chat_logs_dir);
}

// Copied from LLFloaterPreferences because we need to run this without a floater instance existing.
bool PVDataOldAPI::moveTranscriptsAndLog(const std::string &userid) const
{
	std::string instantMessageLogPath(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
	std::string chatLogPath = gDirUtilp->add(instantMessageLogPath, userid);

	bool madeDirectory = false;

	//Does the directory really exist, if not then make it
	if (!LLFile::isdir(chatLogPath))
	{
		//mkdir success is defined as zero
		if (LLFile::mkdir(chatLogPath) != 0)
		{
			return false;
		}
		madeDirectory = true;
	}

	std::string originalConversationLogDir = LLConversationLog::instance().getFileName();
	std::string targetConversationLogDir = gDirUtilp->add(chatLogPath, "conversation.log");
	//Try to move the conversation log
	if (!LLConversationLog::instance().moveLog(originalConversationLogDir, targetConversationLogDir))
	{
		//Couldn't move the log and created a new directory so remove the new directory
		if (madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}
		return false;
	}

	//Attempt to move transcripts
	std::vector<std::string> listOfTranscripts;
	std::vector<std::string> listOfFilesMoved;

	LLLogChat::getListOfTranscriptFiles(listOfTranscripts);

	if (!LLLogChat::moveTranscripts(gDirUtilp->getChatLogsDir(),
		instantMessageLogPath,
		listOfTranscripts,
		listOfFilesMoved))
	{
		//Couldn't move all the transcripts so restore those that moved back to their old location
		LLLogChat::moveTranscripts(instantMessageLogPath,
			gDirUtilp->getChatLogsDir(),
			listOfFilesMoved);

		//Move the conversation log back
		LLConversationLog::instance().moveLog(targetConversationLogDir, originalConversationLogDir);

		if (madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}

		return false;
	}

	gDirUtilp->setChatLogsDir(instantMessageLogPath);
	gDirUtilp->updatePerAccountChatLogsDir();

	return true;
}

std::string PVDataOldAPI::getToken()
{
#if INTERNAL_BUILD
	return gSavedSettings.getString("PVAuth_TesterToken");
#else
	return "";
#endif
}

std::string PVDataOldAPI::getEventMotdIfAny()
{
	for (LLSD::map_const_iterator iter = motd_events_list_.beginMap(); iter != motd_events_list_.endMap(); ++iter)
	{
		auto name = iter->first;
		auto content = iter->second;

		if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
		{
			LL_DEBUGS("PVDebug") << "Setting EVENTS MOTD to " + name << LL_ENDL;
			//@todo: Shove into notification well.
			return content["EventMOTD"].asString();
		}
	}
	return "";
}

void PVDataOldAPI::setBlockedVersionsList(const LLSD& blob)
{
	//blocked_versions_ = blob; // v7?
	auto blocked = blob["BlockedReleases"];
	for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
	{
		auto version = iter->first;
		auto reason = iter->second;
		blocked_versions_[version] = reason;
		LL_DEBUGS("PVDebug") << "Added " + version + " to blocked_versions_ with reason '" + reason.asString() + "'" << LL_ENDL;
	}
}


void PVDataOldAPI::checkBeggar(const LLUUID& avatar_id, const std::string& message)
{
	if(!mBeggarCheckEnabled)
	{
		return;
	}
	boost::cmatch result;
	const boost::regex generic_beg_regex("(((can|)(someone|you)|(can|)(\\splease|)(\\slend me|)|urgently need).*(\\d+L).*(pay back|honest))", boost::regex::perl);
	bool is_generic_beg = boost::regex_search(message.c_str(), result, generic_beg_regex);
	if (is_generic_beg)
	{
		// todo: set pvdata flag
		LLSD args;
		args["AVATAR_ID"] = avatar_id;
		LLNotificationsUtil::add("GenericBeggarNotifyTip", args);
	}
}

void PVDataOldAPI::setBeggarCheck(const bool enabled)
{
	mBeggarCheckEnabled = enabled;
}

// NEW API BELOW

static LLTrace::BlockTimerStatHandle FTM_PVAGENT_GETDATAFOR("!PVAgentData Get Agent");
static LLTrace::BlockTimerStatHandle FTM_PVAGENT_GETCOLOR("!PVAgentData Get Color");
static LLTrace::BlockTimerStatHandle FTM_PVAGENT_GETCOLOROLD("!PVData Get Color");
static LLTrace::BlockTimerStatHandle FTM_PVAGENT_GETTITLEHUMANREADABLE("!PVAgentData Get Title HR");
static LLTrace::BlockTimerStatHandle FTM_PVAGENT_GETTITLE("!PVAgentData Get Title");

PVAgent* PVAgent::find(const LLUUID& avatar_id)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETDATAFOR);
#endif
	PVAgent* agentPtr = nullptr;
	auto it = pvAgents.find(avatar_id);
	if (it != pvAgents.end())
	{
		agentPtr = it->second;
	}
	return agentPtr;
}

PVAgent* PVAgent::create(const LLUUID& avatar_id, const LLColor3& color, const S32& flags, const std::string& custom_title, const std::string& ban_reason)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETDATAFOR);
#endif
	PVAgent* new_agent;
	auto it = pvAgents.find(avatar_id);
	if (it != pvAgents.end())
	{
		new_agent = it->second;
	}
	else
	{
		new_agent = new PVAgent();
	}
		
	new_agent->uuid = avatar_id;
	new_agent->color = color;
	new_agent->flags = flags;
	new_agent->title = custom_title;
	new_agent->ban_reason = ban_reason;

	pvAgents.emplace(avatar_id, new_agent);
	return new_agent;
}

bool PVAgent::hasSpecialColor(LLColor4& color_out)
{
	color_out = color;
	return color_out != LLColor4::black;
}
	
LLColor4 PVAgent::getColorInternal(const LLUIColorTable& cTablePtr)
{
	// The agent could have a special color without having any flags, so get this one first
	LLColor4 pv_color;
	if (!hasSpecialColor(pv_color))
	{
		// Not special, could be a linden
		if (!flags || flags & LINDEN_EMPLOYEE)
		{
			//@todo Linden Color in a non-horrible way, without this duplicated code bullshit...
			std::string first_name, last_name;
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(uuid, &av_name))
			{
				std::istringstream full_name(av_name.getUserName());
				full_name >> first_name >> last_name;
			}
			else
			{
				gCacheName->getFirstLastName(uuid, first_name, last_name);
			}
			if (isLinden(last_name)
				|| isMole(last_name)
				|| isProductEngine(last_name)
				|| isScout(last_name)
				|| isLLTester(last_name))
			{
				static auto linden_color = cTablePtr.getColor("PlvrLindenChatColor", LLColor4::cyan);
				pv_color = linden_color;
			}
		}
		if (isProviderDeveloper())
		{
			static auto dev_color = cTablePtr.getColor("PlvrDevChatColor", LLColor4::orange);
			pv_color = dev_color.get();
		}
		else if (isProviderQATeam())
		{
			static auto qa_color = cTablePtr.getColor("PlvrQAChatColor", LLColor4::red);
			pv_color = qa_color.get();
		}
		else if (isProviderSupportTeam())
		{
			static auto support_color = cTablePtr.getColor("PlvrSupportChatColor", LLColor4::magenta);
			pv_color = support_color.get();
		}
		else if (isProviderTester())
		{
			static auto tester_color = cTablePtr.getColor("PlvrTesterChatColor", LLColor4::yellow);
			pv_color = tester_color.get();
		}
		else if (isProviderBanned())
		{
			static auto banned_color = cTablePtr.getColor("PlvrBannedChatColor", LLColor4::grey2);
			pv_color = banned_color.get();
		}
		else if (isProviderMuted())
		{
			static auto muted_color = cTablePtr.getColor("PlvrMutedChatColor", LLColor4::grey);
			pv_color = muted_color.get();
		}
		// Unsupported users have no color.
		// This will trigger an error for agents who only have a title, so work around that.
		else if (title[0] != '\0')
		{
			// no-op
		}
		else
		{
			//@todo: Use localizable strings
			LLSD args;
			args["AVATAR_ID"] = uuid.asString();
			args["PV_FLAGS"] = getTitleHumanReadable(false).at(0);
			//@todo remove this duplicated code and make reusable function in pvtl
			args["PV_COLOR"] = llformat("<%.5f,%.5f,%.5f>", pv_color.mV[VX], pv_color.mV[VY], pv_color.mV[VZ]);
			args["MESSAGE"] = "Agent has invalid data set!";
			LLNotificationsUtil::add("PVData_ColorBug", args);
		}
		//if (av_flags & DEPRECATED_TITLE_OVERRIDE)
		//{
		//	LLSD args;
		//	args["AVATAR_ID"] = uuid;
		//	args["PV_FLAGS"] = getTitle(false);
		//	args["PV_COLOR"] = llformat("{%.5f , %.5f ,%.5f}", pv_color);
		//	args["MESSAGE"] = "Agent has deprecated flag 'DEPRECATED_TITLE_OVERRIDE'!";
		//	LLNotificationsUtil::add("PVData_ColorBug", args);
		//}
		color = pv_color; // store computed color in the agent blob to speed up further lookups
	}
	return pv_color;
}
	LLColor4 PVAgent::getColor(const LLUUID& avatar_id, const LLColor4 &default_color, bool show_buddy_status)
	{
#ifndef LL_RELEASE_FOR_DOWNLOAD
		LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETCOLOROLD);
#endif
		if (!RlvActions::canShowName(RlvActions::SNC_NAMETAG, avatar_id) || !RlvActions::canShowName(RlvActions::SNC_DEFAULT, avatar_id))
		{
			return default_color;
		}
		// Try to operate in the same instance, reduce call overhead
		LLUIColorTable* cTablePtr = LLUIColorTable::getInstance();

		auto return_color = default_color; // color we end up with at the end of the logic

		// Some flagged users CAN be muted.
		if (LLMuteList::instance().isMuted(avatar_id))
		{
			static auto muted_color = cTablePtr->getColor("PlvrMutedChatColor", LLColor4::grey); // ugh duplicated code
			return_color = muted_color.get();
			return return_color;
		}

		static LLCachedControl<bool> show_friends(gSavedSettings, "NameTagShowFriends");
		auto show_f = (show_friends && show_buddy_status && LLAvatarTracker::instance().isBuddy(avatar_id));
		static auto friend_color = cTablePtr->getColor("NameTagFriend", LLColor4::yellow);
		static LLCachedControl<bool> use_color_manager(gSavedSettings, "PVChat_ColorManager");
		if (!use_color_manager)
		{
			return show_f ? friend_color : default_color;
		}
		else
		{
			LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETCOLOR);
			auto pvdata_color = default_color;
			// if the agent isn't a special agent, nullptr is returned.
			auto agentPtr = find(avatar_id);
			if (agentPtr)
			{
				pvdata_color = agentPtr->getColorInternal(*cTablePtr);
			}

			/*	Respect user preferences
				Expected behavior:
				+Friend, +PVDATA, +lpf = show PVDATA
				+Friend, +PVDATA, -lpl = show FRIEND
				+Friend, -PVDATA, +lpl = show FRIEND
				+Friend, -PVDATA, -lpl = show FRIEND
				-Friend, +PVDATA, +lpl = show PVDATA
				-Friend, +PVDATA, -lpl = show PVDATA
				-Friend, -PVDATA, +lpl = show FALLBACK
				-Friend, -PVDATA, -lpl = show FALLBACK
			*/

			static LLCachedControl<bool> low_priority_friend_status(gSavedSettings, "PVColorManager_LowPriorityFriendStatus", true);
			// Lengthy but fool-proof.
			if (show_f && agentPtr && low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (show_f && agentPtr && !low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (show_f && !agentPtr && low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (show_f && !agentPtr && !low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (!show_f && agentPtr && low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (!show_f && agentPtr && !low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (!show_f && !agentPtr && low_priority_friend_status)
			{
				return_color = default_color;
			}
			if (!show_f && !agentPtr && !low_priority_friend_status)
			{
				return_color = default_color;
			}
		}
		return return_color;
	}

	S32 PVAgent::getFlags()
	{
		return flags;
	}

	std::vector<std::string> PVAgent::getTitleHumanReadable(bool get_custom_title)
	{
		LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETTITLEHUMANREADABLE);
		// contents: { raw_flags, custom_title_or_empty }
		std::vector<std::string> title_v; title_v.reserve(3);
		std::string raw_flags = getTitle(false);
		if (raw_flags.empty())
		{
			raw_flags = "None";
		}
		else
		{
			raw_flags = "[" + raw_flags + "]";
		}
		title_v.push_back(raw_flags);
		//@todo highest non-custom title, see comment in getTitle()
		title_v.push_back(getTitle(true));
		return title_v;
	}

	bool PVAgent::getTitleCustom(std::string& new_title)
	{
		new_title = title;
		return (!new_title.empty());
	}
	
	std::string PVAgent::getTitle(bool get_custom_title)
	{
		LL_RECORD_BLOCK_TIME(FTM_PVAGENT_GETTITLE);
		// Check for agents flagged through PVDataOldAPI
		std::vector<std::string> flags_list;
		if (get_custom_title)
		{
			std::string custom_title;
			if (getTitleCustom(custom_title))
			{
				// Custom tag present, drop previous title to use that one instead.
				flags_list.clear();
				flags_list.push_back(custom_title);
			}
		}
		if ((flags == 0 || flags & LINDEN_EMPLOYEE) && flags_list.empty())
		{
			// Only call this once, thanks.
			std::string first_name, last_name;
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(uuid, &av_name))
			{
				std::istringstream full_name(av_name.getUserName());
				full_name >> first_name >> last_name;
			}
			else
			{
				gCacheName->getFirstLastName(uuid, first_name, last_name);
			}
			if (isLinden(last_name))
			{
				flags_list.push_back("Linden Lab Employee");
			}
			if (isMole(last_name))
			{
				flags_list.push_back("Linden Lab Employee");
			}
			if (isProductEngine(last_name))
			{
				flags_list.push_back("Linden Lab Contractor");
			}
			if (isScout(last_name))
			{
				flags_list.push_back("Linden Lab Scout");
			}
			if (isLLTester(last_name))
			{
				flags_list.push_back("Linden Lab Scout");
			}
		}
		else if (flags != 0 && flags_list.empty())
		{
			//@todo add a way to only get the highest flag instead of a list.
			// here are the bad flags
			if (isProviderMuted())
			{
				flags_list.push_back("Nuisance");
			}
			if (isProviderBanned())
			{
				flags_list.push_back("Exiled");
			}
			if (isProviderUnsupported())
			{
				flags_list.push_back("Unsupported");
			}
			// And here are the good flags
			if (isProviderDeveloper())
			{
				flags_list.push_back("Developer");
			}
			if (isProviderQATeam())
			{
				flags_list.push_back("QA");
			}
			if (isProviderSupportTeam())
			{
				flags_list.push_back("Support");
			}
			if (isProviderTester())
			{
				flags_list.push_back("Tester");
			}
		}
		std::ostringstream agent_title;
		vector_to_string(agent_title, flags_list.begin(), flags_list.end());
		return agent_title.str();
	}

	bool PVAgent::isProviderDeveloper()
	{
		return (flags & STAFF_DEVELOPER);
	}

	bool PVAgent::isProviderSupportTeam()
	{
		return (flags & STAFF_SUPPORT);
	}

	bool PVAgent::isProviderQATeam()
	{
		return (flags & STAFF_QA);
	}

	bool PVAgent::isProviderTester()
	{
		return (flags & USER_TESTER);
	}

	bool PVAgent::isProviderUnsupported()
	{
		return (flags & BAD_USER_UNSUPPORTED);
	}

	bool PVAgent::isProviderMuted()
	{
		return (flags & BAD_USER_AUTOMUTED);
	}

	bool PVAgent::isProviderBanned(bool set_error)
	{
		if (set_error)
		{
			if (ban_reason != "")
			{
				gPVOldAPI->setErrorMessage(ban_reason);
			}
			else
			{
				static const std::string generic_ban_reason = "Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be a mistake, restart the viewer. Otherwise, Please download [https://get.secondlife.com another Viewer].";
				gPVOldAPI->setErrorMessage(generic_ban_reason);
			}
		}
		return (flags & BAD_USER_BANNED);
	}

	bool PVDataOldAPI::isSupportGroup(const LLUUID& group_id) const
	{
		return support_group_.count(group_id);
	}

	bool PVAgent::isPolarized()
	{
		//@todo: Re-order flags by hierarchy again and make this nicer
		//auto flags = getAgentFlags(avatar_id);
		//return (flags > BAD_USER_UNSUPPORTED && flags != DEPRECATED_TITLE_OVERRIDE);
		return ((!isProviderBanned() && !isProviderUnsupported() && !isProviderMuted()) &&
			(isProviderDeveloper() || isProviderQATeam() || isProviderSupportTeam() || isProviderTester()));
	}
//}
