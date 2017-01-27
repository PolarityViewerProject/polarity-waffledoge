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

PVSearchUtil*		gPVSearchUtil = NULL;
PVDataOldAPI*		gPVOldAPI = nullptr;

std::string PVDataOldAPI::pvdata_url_full_ = "";
std::string PVDataOldAPI::pvdata_agents_url_full_ = "";

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

void PVDataOldAPI::PV_DEBUG(const std::string& log_in_s, const LLError::ELevel& level)
{
	static LLCachedControl<bool> pv_debug(gSavedSettings, "PVData_LoggingEnabled", false);
	if (!pv_debug)
	{
		return;
	}
	LL_VLOGS(level, log_in_s.c_str()) << LL_ENDL;
}

void PVDataOldAPI::Dump(const std::string name, const LLSD& map)
{
	std::stringstream str;
	LLSDSerialize::toPrettyXML(map, str);
	PV_DEBUG("\n===========================\n<!--  <" + name + "> -->\n" + str.str() + "\n<!--  </" + name + "> -->\n===========================\n", LLError::LEVEL_DEBUG);
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
	PV_DEBUG("Checking status", LLError::LEVEL_DEBUG);
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
		LL_WARNS() << "Download already in progress, aborting." << LL_ENDL;
		return false;
	case DOWNLOAD_FAILURE:
		LL_WARNS() << "Download failed. Will retry later." << LL_ENDL;
		return true;
	case DOWNLOAD_OK:
		LL_DEBUGS() << "Download success, proceeding." << LL_ENDL;
		return true;
	case PARSING_IN_PROGRESS:
		LL_WARNS() << "Parser is already running, aborting." << LL_ENDL;
		return false;
	case PARSE_FAILURE:
		LL_WARNS() << "Parse failed. This is bad." << LL_ENDL;
		return false;
	default:
		LL_WARNS() << "PVData encountered a problem and has aborted. (STATUS='" << status_container << "')" << LL_ENDL;
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

	PV_DEBUG("Downloading " + requested_file + " from " + pv_url_remote_base_string_ + requested_file, LLError::LEVEL_INFO);
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
	LL_WARNS() << "Failed to download " << aURL << ": " << aData << LL_ENDL;
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
	PV_DEBUG("http_content=" + http_content.asString(), LLError::LEVEL_DEBUG);

	if (http_source_url == pvdata_url_full_)
	{
		PV_DEBUG("Got DATA file", LLError::LEVEL_DEBUG);
		gPVOldAPI->setDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS() << "DATA Download failure, aborting." << LL_ENDL;
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
		PV_DEBUG("Got AGENTS file", LLError::LEVEL_DEBUG);
		gPVOldAPI->setAgentsDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS() << " AGENTS Download failure, aborting." << LL_ENDL;
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
		PV_DEBUG("Got SOMETHING we weren't expecting. what do?", LLError::LEVEL_WARN);
	}
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
		LL_WARNS() << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}
	PV_DEBUG("Beginning to parse Data", LLError::LEVEL_DEBUG);
	pv_data_status_ = PARSING_IN_PROGRESS;

	std::string section = pv_data_sections_.at(MinimumVersion);

	//@todo: Loop through sections and check section type to determine validity?
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		for (LLSD::map_const_iterator iter = blob.beginMap(); iter != blob.endMap(); ++iter)
		{
			auto version = iter->first;
			auto reason = iter->second;
			blob[version] = reason;
			minimum_version_[version] = reason;
			PV_DEBUG("Minimum Version is " + version, LLError::LEVEL_INFO);
		}
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG);
	}

	section = pv_data_sections_.at(PVDataOldAPI::BlockedReleases);
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		PV_DEBUG("Populating Blocked Releases list...", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		setBlockedVersionsList(blob);
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG);
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	section = pv_data_sections_.at(PVDataOldAPI::MOTD);
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		gAgent.mMOTD.assign(blob);
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG); // Don't warn on this one
	}

#if PVDATA_MOTD_CHAT
	section = pv_data_sections_.at(ChatMOTD);
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		LLSD::array_const_iterator iter = blob.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(blob.size()))))->asString());
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}
#endif // PVDATA_MOTD_CHAT

	section = pv_data_sections_.at(EventsMOTD);
	// If the event falls within the current date, use that for MOTD instead.
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		setMotdEventsList(blob);
		auto event_motd = getEventMotdIfAny();
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG); // don't warn on this one
	}
#endif // PVDATA_MOTD

#if PVDATA_PROGRESS_TIPS
	section = pv_data_sections_.at(ProgressTip);
	//@todo: Split tips files
	// Load the progress screen tips
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		setProgressTipsList(blob);
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}

#endif // PVDATA_PROGRESS_TIPS

	section = pv_data_sections_.at(WindowTitles);
	PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		Dump(section, blob);
		setWindowTitlesList(blob);
	}
	else
	{
		PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}
	pv_data_status_ = READY;
	mPVData_llsd = data_input;
	LL_INFOS() << "Done parsing data" << LL_ENDL;
}

void PVDataOldAPI::parsePVAgents(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset pv_data_status_ when parsing is needed again.
	if (!can_proceed(pv_agents_status_))
	{
		LL_WARNS() << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	pv_agents_status_ = PARSING_IN_PROGRESS;
	LL_INFOS() << "Beginning to parse Agents" << LL_ENDL;

	PV_DEBUG("Attempting to find Agents root nodes", LLError::LEVEL_DEBUG);
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
				pvAgents.insert(std::pair<LLUUID, PVAgent*>(uuid, this_agent));

				LL_INFOS() << "flags for " << uuid << " is: " << this_agent->flags << LL_ENDL;
				LL_INFOS() << "color for " << uuid << " is: " << this_agent->color << LL_ENDL;
				LL_INFOS() << "title for " << uuid << " is: " << this_agent->title << LL_ENDL;
				LL_INFOS() << "ban_reason for " << uuid << " is: " << this_agent->ban_reason << LL_ENDL;
				LL_DEBUGS() << "Pointer to pvagent blob for " << uuid << " is " << this_agent << LL_ENDL;
			}
		}
	}
	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			setVendorSupportGroup(LLUUID(itr->first));
			PV_DEBUG("Added " + itr->first + " to support_group_", LLError::LEVEL_DEBUG);
		}
	}

	mPVAgents_llsd = data_input;
	pv_agents_status_ = PVDataOldAPI::READY;
	LL_INFOS() << "Done parsing agents" << LL_ENDL;

	autoMuteFlaggedAgents();
}

void PVDataOldAPI::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS() << "Something went wrong downloading data file" << LL_ENDL;
	gAgent.mMOTD.assign("COULD NOT CONTACT MOTD SERVER");
	pv_data_status_ = DOWNLOAD_FAILURE;
}

void PVDataOldAPI::setFallbackAgentsData()
{
}

void PVDataOldAPI::handleAgentsFailure()
{
	LL_WARNS() << "Something went wrong downloading agents file" << LL_ENDL;
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

	this->PV_DEBUG("Parsing version...", LLError::LEVEL_DEBUG);
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
	LL_DEBUGS() << "Entering function" << LL_ENDL;
	// Use the last tip if available
	std::string return_tip = "";
	if (last_login_tip != "")
	{
		return_tip = last_login_tip;
	}
	static LLCachedControl<F32> progress_tip_timout(gSavedSettings, "PVUI_ProgressTipTimer", 2.f);
	if (forced || (mTipCycleTimer.getStarted() && mTipCycleTimer.getElapsedTimeF32() >= progress_tip_timout))
	{
		LL_DEBUGS() << "mTipCycleTimer elapsed; getting a new random tip" << LL_ENDL;
		LL_DEBUGS() << "Last tip was '" << last_login_tip << "'" << LL_ENDL;

		// Most likely a teleport screen; let's add something.

		// Check for events MOTD first...
		return_tip = getEventMotdIfAny();
		if (return_tip == "")
		{
			return_tip = progress_tips_list_.getRandom();
			LL_INFOS() << "New tip from function is '" << return_tip << "'" << LL_ENDL;
		}

		if (!return_tip.empty() && return_tip != last_login_tip)
		{
			LL_INFOS() << "Setting new progress tip to '" << return_tip << "'" << LL_ENDL;
			last_login_tip = return_tip;
		}
		mTipCycleTimer.reset();
	}
	else
	{
		LL_DEBUGS() << "mTipCycleTimer not started!" << LL_ENDL;
	}

	gAgent.mMOTD = return_tip;
	return return_tip;
}

std::string PVDataOldAPI::getRandomWindowTitle()
{
	PV_DEBUG("Getting random window title from this list:", LLError::LEVEL_INFO);
	Dump("window_titles_list_", gPVOldAPI->window_titles_list_);
	std::string title = gPVOldAPI->window_titles_list_.getRandom();
	PV_DEBUG("Returning  '" + title + "'", LLError::LEVEL_INFO);
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
bool PVDataOldAPI::isAllowedToLogin(const LLUUID& avatar_id) const
{
	bool allowed = false;
	LL_INFOS() << "Evaluating access for " << avatar_id << "..." << LL_ENDL;
	
	gPVOldAPI->setErrorMessage("Generic clearance failure.");
#if PVDATA_UUID_LOCKDOWN
	LLUUID lockdown_uuid = getLockDownUUID();
	if (lockdown_uuid != LLUUID::null)
	{
		LL_INFOS() << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			allowed = true;
			LL_INFOS() << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
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
	//@todo replace with member functions
	auto av_flags = PVAgent::getDataFor(avatar_id)->getFlags();
	LL_WARNS() << "HERE ARE YOUR FLAGS: " << av_flags << LL_ENDL;
	if (av_flags & BAD_USER_BANNED)
	{
		gPVOldAPI->setErrorMessage("Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be a mistake, restart the viewer. Otherwise, Please download [https://get.secondlife.com another Viewer].");
		allowed = false;
	}
	if (LLVersionInfo::getCompiledChannel() == APP_NAME + " Development" && (av_flags & STAFF_DEVELOPER) == false)
	{
		gPVOldAPI->setErrorMessage("Sorry, this build is reserved for [APP_NAME] developers. Please download a public build at " + LLTrans::getString("ViewerDownloadURL") + ".");
		allowed = false;
	}
#if INTERNAL_BUILD
	LL_WARNS() << "Internal build, evaluating access for " << avatar_id << "'..." << LL_ENDL;
	if (av_flags & STAFF_DEVELOPER)
	{
		LL_WARNS() << "Access level: DEVELOPER" << LL_ENDL;
		allowed = true;
	}
	else if (av_flags & STAFF_SUPPORT)
	{
		LL_WARNS() << "Access level: SUPPORT" << LL_ENDL;
		allowed = true;
	}
	else if (av_flags & STAFF_QA)
	{
		LL_WARNS() << "Access level: QA" << LL_ENDL;
		allowed = true;
	}
	else if (av_flags & USER_TESTER)
	{
		LL_WARNS() << "Access level: TESTER" << LL_ENDL;
		allowed = true;
	}
	else
	{
		LL_WARNS() << "Access level: NONE" << LL_ENDL;
		gPVOldAPI->setErrorMessage("You do not have clearance to use this build of [APP_NAME].\nIf you believe this to be a mistake, contact the [APP_NAME] Viewer support. Otherwise, please download a public build at\n" + LLTrans::getString("ViewerDownloadURL") + ".");
		allowed = false;
	}
#else
	allowed = true;
#endif
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
		PV_DEBUG(sCurrentVersion + " not found in the blocked releases list", LLError::LEVEL_DEBUG);
		setErrorMessage("");
		blocked = FALSE;
		return false;
	}

	// assign the iterator's associated value (the reason message) to the LLSD that will be returned to the calling function
	const LLSD& reason_llsd = blockedver_iterator->second;
		setErrorMessage(reason_llsd["REASON"]);
		LL_WARNS() << sCurrentVersion << " is not allowed to be used anymore (" << getErrorMessage() << ")" << LL_ENDL;
	LLFloaterAboutUtil::checkUpdatesAndNotify();
	blocked = TRUE;
	return true;
}

bool PVDataOldAPI::isLinden(const std::string& last_name)
{
	return last_name == LL_LINDEN;
}

bool PVDataOldAPI::isMole(const std::string& last_name)
{
	return last_name == LL_MOLE;
}

bool PVDataOldAPI::isProductEngine(const std::string& last_name)
{
	return last_name == LL_PRODUCTENGINE;
}

bool PVDataOldAPI::isScout(const std::string& last_name)
{
	return last_name == LL_SCOUT;
}

bool PVDataOldAPI::isLLTester(const std::string& last_name)
{
	return last_name == LL_TESTER;
}

void PVDataOldAPI::startRefreshTimer()
{
	if (!pvdata_refresh_timer_.getStarted())
	{
		LL_INFOS() << "Starting PVData refresh timer" << LL_ENDL;
		pvdata_refresh_timer_.start();
	}
	else
	{
		LL_WARNS() << "Timer already started!" << LL_ENDL;
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
		LL_INFOS() << "Attempting to live-refresh PVDataOldAPI" << LL_ENDL;
		downloadData();

		PV_DEBUG("Attempting to live-refresh Agents data", LLError::LEVEL_DEBUG);
		downloadAgents();
		if (!force_refresh_now)
		{
			PV_DEBUG("Resetting timer", LLError::LEVEL_DEBUG);
			pvdata_refresh_timer_.reset();
		}
	}
}

LLColor4 PVDataOldAPI::Hex2Color4(const std::string color) const
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

U32 PVSearchUtil::PVSearchSeparatorSelected = gPVSearchUtil->separator_space;

U32 PVSearchUtil::getSearchSeparatorFromSettings()
{
	static LLCachedControl<U32> settings_separator(gSavedSettings, "PVUI_SubstringSearchSeparator", separator_space);
	LL_DEBUGS("PVDataOldAPI") << "Search separator index from settings: '" << settings_separator << "'" << LL_ENDL;
	return settings_separator;
}

void PVSearchUtil::setSearchSeparator(const U32 separator_in_u32)
{
	PVSearchSeparatorSelected = separator_in_u32;;
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
		LL_WARNS() << "Preferred Name was unavailable, returning '" << preferred_name << "'" << LL_ENDL;
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
	DWORD		size = 1024 * sizeof(TCHAR);;
	//LONG        status;
	HKEY        subKey;
	std::string value = "";

	long ret;
	ret = RegOpenKeyExA(HKEY_CURRENT_USER, "\\Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &subKey);
	RegQueryValueExA(subKey, name_.c_str(), 0, NULL, NULL, &size);
	if (ret != ERROR_SUCCESS) {
		LL_WARNS() << "Key [" << name_ << "] does not exist!" << LL_ENDL;
		return std::string();
	}
	// This environment variable already exists.
	ret = RegQueryValueExA(key, name_.c_str(), 0, 0, reinterpret_cast<LPBYTE>(data), &size);
	if (ret != ERROR_SUCCESS)
	{
		return std::string();
	}
	LL_WARNS() << "Key [" << name_ << "] exist!" << LL_ENDL;
	if (data != nullptr)
	{
		value = std::string(data);
	}
	delete[] data;

	LL_WARNS() << "Key [" << value << "] = exist!" << LL_ENDL;
	RegCloseKey(key);
	//@todo: Linux and OSX support
	return value;
}

void PVDataOldAPI::getChatLogsDirOverride()
{
	std::string log_location_from_settings = gSavedPerAccountSettings.getString("InstantMessageLogPath");
	std::string registry_key = "PV_CHATLOGS_LOCATION_OVERRIDE"; //@todo: Move to global
																// ReSharper disable CppDeprecatedEntity // cross-platform needs std:: function
	char* log_location_from_registry = getenv(registry_key.c_str());

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
	PV_DEBUG("Would set logs location to: " + new_chat_logs_dir, LLError::LEVEL_WARN);
	PV_DEBUG("gDirUtilp->getChatLogsDir() = " + gDirUtilp->getChatLogsDir(), LLError::LEVEL_WARN);

	LL_WARNS() << "Chat log location = " << new_chat_logs_dir << LL_ENDL;
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
		PV_DEBUG("Hmmm strange, location mismatch: " + new_chat_logs_dir + " != " + gDirUtilp->getChatLogsDir(), LLError::LEVEL_WARN);
	}

	gSavedPerAccountSettings.setString("InstantMessageLogPath", new_chat_logs_dir);
}

// Copied from LLFloaterPreferences because we need to run this without a floater instance existing.
bool PVDataOldAPI::moveTranscriptsAndLog(std::string userid) const
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

// NEW API BELOW

PVAgent::PVAgent()
{
	if (gPVOldAPI != PVDataOldAPI::getInstance())
	{
		gPVOldAPI = PVDataOldAPI::getInstance();
	}
}
//namespace PVDataOldAPI
//{
	PVAgent* PVAgent::getDataFor(const LLUUID& avatar_id)
	{
		auto it = pvAgents.find(avatar_id);
		if(it == pvAgents.end())
		{
			return nullptr;
		}
		return it->second;
	}

	bool PVAgent::isSpecialAgentColored() const
	{
		return color != LLColor3::black;
	}

	LLColor4 PVAgent::getColorCustom() const
	{
		return LLColor4(color);
	}

	// Do not call directly!
	LLColor4 PVAgent::getColor(PVAgent* pv_agent, S32 av_flags, LLUIColorTable* uiCT) const
	{
		LLColor4 pv_color = no_color;
		// Check if agent already has a special color
		if (pv_agent->isSpecialAgentColored())
		{
			pv_color = pv_agent->getColorCustom();
		}
		else
		{
			// Not special, could be a linden
			if (av_flags == 0 || av_flags & LINDEN_EMPLOYEE)
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
				if (gPVOldAPI->isLinden(last_name)
					|| gPVOldAPI->isMole(last_name)
					|| gPVOldAPI->isProductEngine(last_name)
					|| gPVOldAPI->isScout(last_name)
					|| gPVOldAPI->isLLTester(last_name))
				{
					static LLUIColor linden_color = uiCT->getColor("PlvrLindenChatColor", LLColor4::cyan);
					pv_color = linden_color;
				}
			}
			if (pv_agent->isUserDevStaff())
			{
				static LLUIColor dev_color = uiCT->getColor("PlvrDevChatColor", LLColor4::orange);
				pv_color = dev_color.get();
			}
			else if (pv_agent->isUserQAStaff())
			{
				static LLUIColor qa_color = uiCT->getColor("PlvrQAChatColor", LLColor4::red);
				pv_color = qa_color.get();
			}
			else if (pv_agent->isUserSupportStaff())
			{
				static LLUIColor support_color = uiCT->getColor("PlvrSupportChatColor", LLColor4::magenta);
				pv_color = support_color.get();
			}
			else if (pv_agent->isUserTester())
			{
				static LLUIColor tester_color = uiCT->getColor("PlvrTesterChatColor", LLColor4::yellow);
				pv_color = tester_color.get();
			}
			else if (pv_agent->isUserBanned())
			{
				static LLUIColor banned_color = uiCT->getColor("PlvrBannedChatColor", LLColor4::grey2);
				pv_color = banned_color.get();
			}
			else if (pv_agent->isUserAutoMuted())
			{
				static LLUIColor muted_color = uiCT->getColor("PlvrMutedChatColor", LLColor4::grey);
				pv_color = muted_color.get();
			}
			// Unsupported users have no color.
			else
			{
				//@todo: Use localizable strings
				LLSD args;
				args["AVATAR_ID"] = uuid;
				args["PV_FLAGS"] = getTitle(false);
				args["PV_COLOR"] = llformat("{%.5f , %.5f ,%.5f}", pv_color);
				args["MESSAGE"] = "Agent has deprecated or unhandled flags associated to it!";
				LLNotificationsUtil::add("PVData_ColorBug", args);
			}
			if (av_flags & DEPRECATED_TITLE_OVERRIDE)
			{
				LLSD args;
				args["AVATAR_ID"] = uuid;
				args["PV_FLAGS"] = getTitle(false);
				args["PV_COLOR"] = llformat("{%.5f , %.5f ,%.5f}", pv_color);
				args["MESSAGE"] = "Agent has deprecated flag 'DEPRECATED_TITLE_OVERRIDE'!";
				LLNotificationsUtil::add("PVData_ColorBug", args);
			}
		}
		return pv_color;
	}

	LLColor4 PVDataOldAPI::getColor(const LLUUID& avatar_id, const LLColor4& default_color, bool show_buddy_status)
	{
		// Try to operate in the same instance, reduce call overhead
		LLUIColorTable* uiCT = LLUIColorTable::getInstance();

		LLColor4 return_color = default_color; // color we end up with at the end of the logic
		
		// Some flagged users CAN be muted.
		if (LLMuteList::instance().isMuted(avatar_id))
		{
			static LLUIColor muted_color = uiCT->getColor("PlvrMutedChatColor", LLColor4::grey); // ugh duplicated code
			return_color = muted_color.get();
			return return_color;
		}

		static LLCachedControl<bool> show_friends(gSavedSettings, "NameTagShowFriends");
		auto show_f = (show_friends && show_buddy_status && LLAvatarTracker::instance().isBuddy(avatar_id));
		auto friend_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
		static LLCachedControl<bool> use_color_manager(gSavedSettings, "PVChat_ColorManager");
		if (use_color_manager)
		{
			LLColor4 pvdata_color = default_color; // User color from PVData if user has one, equals return_color otherwise.

			auto pv_agent = PVAgent::getDataFor(avatar_id);
			S32 av_flags = (pv_agent) ? pv_agent->getFlags() : 0;
			// if the agent isn't a special agent, nullptr is returned.
			if (pv_agent)
			{
				pvdata_color = pv_agent->getColor(pv_agent, av_flags, uiCT);
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
			if (show_f && av_flags && low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (show_f && av_flags && !low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (show_f && !av_flags && low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (show_f && !av_flags && !low_priority_friend_status)
			{
				return_color = friend_color;
			}
			if (!show_f && av_flags && low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (!show_f && av_flags && !low_priority_friend_status)
			{
				return_color = pvdata_color;
			}
			if (!show_f && !av_flags && low_priority_friend_status)
			{
				return_color = default_color;
			}
			if (!show_f && !av_flags && !low_priority_friend_status)
			{
				return_color = default_color;
			}
		}
		else
		{
			return_color = show_f ? friend_color : default_color;
		}
		return return_color;
	}

	S32 PVAgent::getFlags() const
	{
		return flags;
	}

	bool PVAgent::getTitleCustom(std::string& new_title) const
	{
		new_title = title;
		return (!new_title.empty());
	}

	std::string PVAgent::getTitle(bool get_custom_title) const
	{
		// Check for agents flagged through PVDataOldAPI
		std::vector<std::string> flags_list;
		auto pv_agent = PVAgent::getDataFor(uuid);
		auto pv_flags = pv_agent->getFlags();
		if (get_custom_title)
		{
			std::string custom_title;
			if (pv_agent->getTitleCustom(custom_title))
			{
				// Custom tag present, drop previous title to use that one instead.
				flags_list.clear();
				flags_list.push_back(custom_title);
			}
		}
		if ((pv_flags == 0 || pv_flags & LINDEN_EMPLOYEE) && flags_list.empty())
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
			if (gPVOldAPI->isLinden(last_name))
			{
				flags_list.push_back("Linden Lab Employee");
			}
			if (gPVOldAPI->isMole(last_name))
			{
				flags_list.push_back("Linden Lab Employee");
			}
			if (gPVOldAPI->isProductEngine(last_name))
			{
				flags_list.push_back("Linden Lab Contractor");
			}
			if (gPVOldAPI->isScout(last_name))
			{
				flags_list.push_back("Linden Lab Scout");
			}
			if (gPVOldAPI->isLLTester(last_name))
			{
				flags_list.push_back("Linden Lab Scout");
			}
		}
		else if (pv_flags != 0 && flags_list.empty())
		{
			// here are the bad flags
			if (pv_agent->isUserAutoMuted())
			{
				flags_list.push_back("Nuisance");
			}
			if (pv_agent->isUserBanned())
			{
				flags_list.push_back("Exiled");
			}
			if (pv_agent->isUserUnsupported())
			{
				flags_list.push_back("Unsupported");
			}
			// And here are the good flags
			if (pv_agent->isUserDevStaff())
			{
				flags_list.push_back("Developer");
			}
			if (pv_agent->isUserQAStaff())
			{
				flags_list.push_back("QA");
			}
			if (pv_agent->isUserSupportStaff())
			{
				flags_list.push_back("Support");
			}
			if (pv_agent->isUserTester())
			{
				flags_list.push_back("Tester");
			}
		}
		std::ostringstream agent_title;
		vector_to_string(agent_title, flags_list.begin(), flags_list.end());
		return agent_title.str();
	}

	bool PVAgent::isUserDevStaff() const
	{
		return (flags & STAFF_DEVELOPER);
	}

	bool PVAgent::isUserSupportStaff() const
	{
		return (flags & STAFF_SUPPORT);
	}

	bool PVAgent::isUserQAStaff() const
	{
		return (flags & STAFF_QA);
	}

	bool PVAgent::isUserTester() const
	{
		return (flags & USER_TESTER);
	}

	bool PVAgent::isUserUnsupported() const
	{
		return (flags & BAD_USER_UNSUPPORTED);
	}

	bool PVAgent::isUserAutoMuted() const
	{
		return (flags & BAD_USER_AUTOMUTED);
	}

	bool PVAgent::isUserBanned() const
	{
		return (flags & BAD_USER_BANNED);
	}

	bool PVDataOldAPI::isSupportGroup(const LLUUID& group_id) const
	{
		return support_group_.count(group_id);
	}

	bool PVAgent::isUserPolarized() const
	{
		//@todo: Re-order flags by hierarchy again and make this nicer
		//auto flags = getAgentFlags(avatar_id);
		//return (flags > BAD_USER_UNSUPPORTED && flags != DEPRECATED_TITLE_OVERRIDE);
		return (getFlags() != 0 &&
			!isUserAutoMuted() &&
			!isUserBanned() &&
			!isUserUnsupported());
	}
//}
