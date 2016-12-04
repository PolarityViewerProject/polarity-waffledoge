/**
 * @file pvdata.cpp
 * @brief Downloadable metadata for viewer features.
 * Inspired by FSData by Techwolf Lupindo
 * Re-implented by Xenhat Liamano
 *
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "pvdata.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
#include <boost/spirit/include/karma.hpp>

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloaterabout.h"
#include "llmutelist.h"
#include "llprogressview.h"
#include "llsdserialize.h"
#include "llstartup.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "noise.h"

#include <stdlib.h> // for setenv
#include "llfloaterpreference.h"
#include "lltrans.h" // for getString
#include "llnotificationsutil.h"
#include "pvcommon.h"

// This one needs to stay in the global scope, I think
PVData*				gPVData = NULL;
PVDataAuth*			gPVDataAuth = NULL;
PVDataDownloader*	gPVDataDownloader = NULL;
PVDataUtil*			gPVDataUtil = NULL;
PVDataViewerInfo*	gPVDataViewerInfo = NULL;
PVSearchUtil*		gPVSearchUtil = NULL;

std::string PVDataDownloader::pvdata_url_full_ = "";
std::string PVDataDownloader::pvdata_agents_url_full_ = "";

#if LL_DARWIN
size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return(p ? p-s : n);
}
#endif // LL_DARWIN

void PVData::PV_DEBUG(const std::string& log_in_s, const LLError::ELevel& level, const bool& developer_only)
{
	static LLCachedControl<bool> pvdebug_printtolog(gSavedSettings, "PVDebug_PrintToLog", true);
	if (!pvdebug_printtolog || (developer_only && (LLStartUp::getStartupState() >= STATE_LOGIN_CONTINUE && !gPVDataAuth->isUserDevStaff(gAgentID))))
	{
		return;
	}
	// Signed int because there is no operator for LLError::LEVEL_WARN => S32
	S32 log_level = (S32)level;
	S32 pvdebug_printtolog_forcedlevel = gSavedSettings.getS32("PVDebug_PrintToLogForcedLevel");
	if (pvdebug_printtolog_forcedlevel >= 0)
	{
		// Don't let the user set log as error: this will crash them.
		if (LLError::LEVEL_ERROR == pvdebug_printtolog_forcedlevel)
		{
			log_level = LLError::LEVEL_WARN;
		}
		else
		{
			log_level = pvdebug_printtolog_forcedlevel;
		}
	}
	// Ensure our string is null-terminated.
	const std::string nullterm_string = log_in_s.c_str();

	if (LLError::LEVEL_DEBUG == log_level)
	{
		LL_DEBUGS() << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_INFO == log_level)
	{
		LL_INFOS() << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_WARN == log_level)
	{
		LL_WARNS() << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_ERROR == log_level)
	{
		LL_ERRS() << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_NONE == log_level)
	{
		// Nope.
	}
}

void PVData::Dump(const std::string name, const LLSD& map)
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

bool PVDataDownloader::can_proceed(U8& status_container) const
{
	gPVData->PV_DEBUG("Checking status", LLError::LEVEL_DEBUG);
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

void PVDataDownloader::downloadData()
{
	if (can_proceed(pv_data_status_))
	{
		pv_data_status_ = DOWNLOAD_IN_PROGRESS;
		modularDownloader(data_file);
	}
}

void PVDataDownloader::downloadAgents()
{
	if (can_proceed(pv_agents_status_))
	{
		pv_agents_status_ = DOWNLOAD_IN_PROGRESS;
		modularDownloader(agents_file);
	}
}

void PVDataDownloader::modularDownloader(const S8& pfile_name_in)
{
	// TODO: Move some place that won't get run twice

	pv_file_name_data_string_ = pv_file_names.find(data_file)->second;
	pv_file_name_agents_string_ = pv_file_names.find(agents_file)->second;

	// Sets up the variables we need for each object. Avoids call bloat in the class constructor.
	pvdata_user_agent_ = LLViewerMedia::getCurrentUserAgent();
	pvdata_viewer_version_ = LLVersionInfo::getChannelAndVersionStatic();
	LL_WARNS() << "Checking for testing data source" << LL_ENDL;
	static LLCachedControl<bool> pvdata_testing_branch(gSavedSettings, "PVData_UseTestingDataSource", false);
	LL_WARNS() << "Checking for testing data source part 2" << LL_ENDL;
	if (pv_url_remote_base_string_ == "" || pvdata_testing_branch != pv_downloader_testing_branch)
	{
		pv_url_remote_base_string_ = "https://data.polarityviewer.org/" + (pvdata_testing_branch ? std::string("test/") : std::string("live/")) + std::to_string(pvdata_file_version) + "/";
		pv_downloader_testing_branch = pvdata_testing_branch;
	}

	// construct download url from file name
	headers_.insert("User-Agent", pvdata_user_agent_);
	headers_.insert("viewer-version", pvdata_viewer_version_);
	// FIXME: This is ugly
	//pvdata_modular_remote_url_full_ = pv_url_remote_base_string_ + pfile_name_in;
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

	gPVData->PV_DEBUG("Downloading " + requested_file + " from " + pv_url_remote_base_string_ + requested_file, LLError::LEVEL_INFO);
	// TODO: HTTP eTag support
	//LLHTTPClient::get(pvdata_modular_remote_url_full_, new PVDataDownloader(pvdata_modular_remote_url_full_, pfile_name_in), headers_, HTTP_TIMEOUT);
	LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(pv_url_remote_base_string_ + requested_file,
		boost::bind(downloadComplete, _1, pv_url_remote_base_string_ + requested_file),
		boost::bind(downloadError, _1, pv_url_remote_base_string_ + requested_file));
}

void PVDataDownloader::downloadComplete(const LLSD& aData, std::string& aURL)
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

void PVDataDownloader::downloadError(const LLSD& aData, std::string& aURL)
{
	LL_WARNS() << "Failed to download " << aURL << ": " << aData << LL_ENDL;
	handleResponseFromServer(aData, aURL, true);
}

void PVDataDownloader::handleResponseFromServer(const LLSD& http_content,
	const std::string& http_source_url,
	const bool& download_failed
)
{
	static LLCachedControl<bool> dump_web_data(gSavedSettings, "PVDebug_DumpWebData", false);
	if (dump_web_data)
	{
		gPVData->Dump(http_source_url, http_content);
	}
	gPVData->PV_DEBUG("http_content=" + http_content.asString(), LLError::LEVEL_DEBUG);

	if (http_source_url == pvdata_url_full_)
	{
		gPVData->PV_DEBUG("Got DATA file", LLError::LEVEL_DEBUG);
		gPVDataDownloader->setDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS() << "DATA Download failure, aborting." << LL_ENDL;
			gPVDataDownloader->setDataStatus(DOWNLOAD_FAILURE);
			gPVDataDownloader->handleDataFailure();
		}
		else
		{
			gPVDataDownloader->setDataStatus(DOWNLOAD_OK);
			gPVDataDownloader->parsePVData(http_content);
		}
	}
	else if (http_source_url == pvdata_agents_url_full_)
	{
		gPVData->PV_DEBUG("Got AGENTS file", LLError::LEVEL_DEBUG);
		gPVDataDownloader->setAgentsDataStatus(NEW_DATA);
		if (download_failed)
		{
			LL_WARNS() << " AGENTS Download failure, aborting." << LL_ENDL;
			gPVDataDownloader->setAgentsDataStatus(DOWNLOAD_FAILURE);
			gPVDataDownloader->handleAgentsFailure();
		}
		else
		{
			gPVDataDownloader->setAgentsDataStatus(DOWNLOAD_OK);
			//pv_agents_status_ = INIT; // Don't reset here, that would defeat the purpose.
			gPVDataDownloader->parsePVAgents(http_content);
		}
	}
	else
	{
		gPVData->PV_DEBUG("Got SOMETHING we weren't expecting. what do?", LLError::LEVEL_WARN);
	}
}

// ########     ###    ########   ######  ######## ########   ######
// ##     ##   ## ##   ##     ## ##    ## ##       ##     ## ##    ##
// ##     ##  ##   ##  ##     ## ##       ##       ##     ## ##
// ########  ##     ## ########   ######  ######   ########   ######
// ##        ######### ##   ##         ## ##       ##   ##         ##
// ##        ##     ## ##    ##  ##    ## ##       ##    ##  ##    ##
// ##        ##     ## ##     ##  ######  ######## ##     ##  ######

void PVDataDownloader::parsePVData(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset pv_data_status_ when parsing is needed again.
	if (!can_proceed(pv_data_status_))
	{
		// FIXME: why do we get 'pv_data_status_==PARSING' BEFORE it's actually being set? (see below)
		LL_WARNS() << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}
	gPVDataViewerInfo = PVDataViewerInfo::getInstance();
	gPVData->PV_DEBUG("Beginning to parse Data", LLError::LEVEL_DEBUG);
	pv_data_status_ = PARSING_IN_PROGRESS;

	std::string section = pv_data_sections_.at(MinimumVersion);

	// TODO: Loop through sections and check section type to determine validity?
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		gPVDataViewerInfo->setMinimumVersion(data_input[section]);
		//gPVData->PV_DEBUG("Minimum Version is " + gPVDataViewerInfo->getMinimumVersion(), LLError::LEVEL_INFO);
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG);
	}
	
	section = pv_data_sections_.at(BlockedReleases);
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		gPVData->PV_DEBUG("Populating Blocked Releases list...", LLError::LEVEL_DEBUG);
		gPVDataViewerInfo->setBlockedVersionsList(data_input[section]);
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG);
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	section = pv_data_sections_.at(MOTD);
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		gAgent.mMOTD.assign(data_input[section]);
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG); // Don't warn on this one
	}

#if PVDATA_MOTD_CHAT
	section = pv_data_sections_.at(ChatMOTD);
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto motd = data_input[section];
		LLSD::array_const_iterator iter = motd.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(motd.size()))))->asString());
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}
#endif // PVDATA_MOTD_CHAT

	section = pv_data_sections_.at(EventsMOTD);
	// If the event falls within the current date, use that for MOTD instead.
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		gPVDataViewerInfo->setMotdEventsList(data_input[section]);
		auto event_motd = gPVDataViewerInfo->getEventMotdIfAny();
		if (event_motd != "")
		{
			gAgent.mMOTD.assign(event_motd);
		}
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_DEBUG); // don't warn on this one
	}
#endif // PVDATA_MOTD

#if PVDATA_PROGRESS_TIPS
	section = pv_data_sections_.at(ProgressTip);
	// TODO: Split tips files
	// <polarity> Load the progress screen tips
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		// Store list for later use
		gPVDataViewerInfo->setProgressTipsList(data_input[section]);
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}
#endif // PVDATA_PROGRESS_TIPS

	section = pv_data_sections_.at(WindowTitles);
	gPVData->PV_DEBUG("Attempting to find " + section, LLError::LEVEL_DEBUG);
	if (data_input.has(section))
	{
		gPVData->PV_DEBUG("Found " + section + "!", LLError::LEVEL_DEBUG);
		auto blob = data_input[section];
		gPVData->Dump("Window Titles raw", blob);
		// Store list for later use
		// FIXME: This sets an empty map?!
		gPVDataViewerInfo->setWindowTitlesList(blob);
	}
	else
	{
		gPVData->PV_DEBUG("No " + section + " found!", LLError::LEVEL_WARN);
	}
	pv_data_status_ = READY;
	mPVData_llsd = data_input;
	LL_INFOS() << "Done parsing data" << LL_ENDL;
}

void PVDataDownloader::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS() << "Something went wrong downloading data file" << LL_ENDL;
	gAgent.mMOTD.assign("COULD NOT CONTACT MOTD SERVER");
	pv_data_status_ = DOWNLOAD_FAILURE;
}

void PVDataAuth::setFallbackAgentsData()
{
	pv_special_agent_flags_[LLUUID("f56e83a9-da38-4230-bac8-b146e7035dfc")] = BAD_USER_BANNED;
	pv_special_agent_flags_[LLUUID("6b7c1d1b-fc8a-4b11-9202-707e99b4a89a")] = BAD_USER_BANNED;
	pv_special_agent_flags_[LLUUID("584d796a-bb85-4fe9-8f7c-1f2fbf2ff164")] = STAFF_DEVELOPER | STAFF_QA; // Darl
	pv_special_agent_flags_[LLUUID("f1a73716-4ad2-4548-9f0e-634c7a98fe86")] = STAFF_DEVELOPER; // Xenhat
	pv_special_agent_flags_[LLUUID("238afefc-74ec-4afe-a59a-9fe1400acd92")] = STAFF_DEVELOPER;
	pv_special_agent_flags_[LLUUID("a43d30fe-e2f6-4ef5-8502-2335879ec6b1")] = STAFF_SUPPORT;
}

void PVDataDownloader::handleAgentsFailure()
{
	LL_WARNS() << "Something went wrong downloading agents file" << LL_ENDL;
	gPVDataAuth->setFallbackAgentsData();
	pv_agents_status_ = DOWNLOAD_FAILURE;
}

#if 0 // v7
std::string PVDataInfo::getMinimumVersion()
{
	auto version = minimum_version_.beginMap()->second;
	// TODO: QA THIS
	PV_DEBUG("Minimum version is: " + version.asString(), LLError::LEVEL_INFO);
	return version.asString();
}
#endif

bool PVDataViewerInfo::isVersionAtOrAboveMinimum()
{
	/* TODO: Implement in v7.
	* What we need:
	<key>MinimumVersion</key>
	<map>
	<key>VERSION</key>
	<string>5.12.3</string>
	<key>REASON</key>
	<string>This version of Polarity Viewer is too old. Please wait for an updated build to arrive your way or download a new one.</string>
	</map>
	*/
#if VERSION_7
	std::string min_version_str = getMinimumVersion();
	std::istringstream iss(min_version_str);
	std::vector<std::string> tokens;
	std::string token;
	while (std::getline(iss, token, '.')) {
		if (!token.empty())
			tokens.push_back(token);
	}
	// yay implicit conversions!
	if (token[0] >= LLVersionInfo::getMajor() &&
		token[1] >= LLVersionInfo::getMinor() &&
		token[2] >= LLVersionInfo::getPatch())
	{
		return true;
	}
	else
	{
		// TODO: Write tests. omg.
		// TODO: Finish this once we have v7 deployed.
		auto version_str = minimum_version_[version_str];
		if (minimum_version_.has("REASON") && minimum_version_["REASON"].type() == LLSD::TypeString)
		{

			version_str.beginMap()->first;

		}

		auto version_data = version_map["REASON"];
		setErrorMessage(version_data->second)
#else
	return true;
#endif
	}

bool PVDataDownloader::getDataDone()
{
	if (pv_data_status_ == READY)
	{
		return true;
	}
	return false;
}

bool PVDataDownloader::getAgentsDone()
{
	if (pv_agents_status_ == READY)
	{
		return true;
	}
	return false;
}

std::string PVDataViewerInfo::getNewProgressTipForced()
{
	// This assigns a random entry as the MOTD / Progress Tip message.
	LLSD::array_const_iterator tip_iter = progress_tips_list_.beginArray();
	if (tip_iter == progress_tips_list_.endArray())
		return "";
	std::string random_tip = (tip_iter + (ll_rand(static_cast<S32>(progress_tips_list_.size()))))->asString();
	LL_INFOS() << "Setting Progress tip to '" << random_tip << "'" << LL_ENDL;
	return random_tip;
}

std::string PVDataViewerInfo
	::getNewProgressTip(const std::string msg_in)
{
	LL_DEBUGS() << "Entering function" << LL_ENDL;
	// Pass the existing message right through
	if (!msg_in.empty())
	{
		LL_DEBUGS() << "returning '" << msg_in << "' in passthrough mode" << LL_ENDL;
		return msg_in;
	}
	// Use the last tip if available
	std::string return_tip = "";
	if (last_login_tip != "")
	{
		return_tip = last_login_tip;
	}
	if (mTipCycleTimer.getStarted())
	{
		static LLCachedControl<F32> progress_tip_timout(gSavedSettings, "PVUI_ProgressTipTimer", 2.f);
		if (mTipCycleTimer.getElapsedTimeF32() >= progress_tip_timout)
		{
			LL_DEBUGS() << "mTipCycleTimer elapsed; getting a new random tip" << LL_ENDL;
			LL_DEBUGS() << "Last tip was '" << last_login_tip << "'" << LL_ENDL;

			// Most likely a teleport screen; let's add something.

			return_tip = progress_tips_list_.getRandom();
			LL_DEBUGS() << "New tip from function is '" << return_tip << "'" << LL_ENDL;

			if (!return_tip.empty() && return_tip != last_login_tip)
			{
				LL_INFOS() << "Setting new progress tip to '" << return_tip << "'" << LL_ENDL;
				last_login_tip = return_tip;
			}
			mTipCycleTimer.reset();
		}
	}
	else
	{
		LL_WARNS() << "mTipCycleTimer not started!" << LL_ENDL;
	}

	return return_tip;
}

std::string PVDataViewerInfo::getRandomWindowTitle()
{
	gPVData->PV_DEBUG("Getting random window title from this list:", LLError::LEVEL_INFO);
	gPVData->Dump("window_titles_list_", gPVDataViewerInfo->window_titles_list_);
	std::string title = gPVDataViewerInfo->window_titles_list_.getRandom();
	gPVData->PV_DEBUG("Returning  '" + title + "'", LLError::LEVEL_INFO);
	return title;
}

void PVDataAuth::setSpecialAgentFlags(const LLUUID& uuid, const S32& flags)
{
	pv_special_agent_flags_[uuid] = flags;
}

	void PVDataAuth::setSpecialAgentTitle(const LLUUID& uuid, const std::string& title)
{
	pv_special_agent_title_[uuid] = title;
}

	void PVDataAuth::setSpecialAgentColor(const LLUUID& uuid, const LLColor4& color4)
{
	pv_special_agent_color_[uuid] = color4;
}

void PVDataAuth::setSpecialAgentBanReason(const LLUUID& uuid, const std::string& reason)
{
	pv_special_agent_ban_reason_[uuid] = reason;
}

void PVDataAuth::setVendorSupportGroup(const LLUUID& uuid)
{
	support_group_.insert(uuid);
}

void PVDataDownloader::parsePVAgents(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset pv_data_status_ when parsing is needed again.
	if (!can_proceed(pv_agents_status_))
	{
		LL_WARNS() << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	pv_agents_status_ = PARSING_IN_PROGRESS;
	LL_INFOS() << "Beginning to parse Agents" << LL_ENDL;

	gPVData->PV_DEBUG("Attempting to find Agents root nodes", LLError::LEVEL_DEBUG);
	if (data_input.has("SpecialAgentsList"))
	{
		const LLSD& special_agents_llsd = data_input["SpecialAgentsList"];

		for (LLSD::map_const_iterator uuid_iterator = special_agents_llsd.beginMap();
			 uuid_iterator != special_agents_llsd.endMap(); ++uuid_iterator)
		{
			LLUUID uuid;
			LLUUID::parseUUID(uuid_iterator->first, &uuid);
			const LLSD& data_map = uuid_iterator->second;
			if (data_map.has("Access") && data_map["Access"].type() == LLSD::TypeInteger)
			{
				gPVDataAuth->setSpecialAgentFlags(uuid,data_map["Access"].asInteger());
			}
			if (data_map.has("HexColor") && data_map["HexColor"].type() == LLSD::TypeString)
			{
				gPVDataAuth->setSpecialAgentColor(uuid,Hex2Color4(data_map["HexColor"].asString()));
			}
			if (data_map.has("Title") && data_map["Title"].type() == LLSD::TypeString)
			{
				gPVDataAuth->setSpecialAgentTitle(uuid,data_map["Title"].asString());
			}
			if (data_map.has("BanReason") && data_map["BanReason"].type() == LLSD::TypeString)
			{
				gPVDataAuth->setSpecialAgentBanReason(uuid,data_map["BanReason"].asString());
			}
		}
	}
	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			gPVDataAuth->setVendorSupportGroup(LLUUID(itr->first));
			gPVData->PV_DEBUG("Added " + itr->first + " to support_group_", LLError::LEVEL_DEBUG);
		}
	}

	mPVAgents_llsd = data_input;
	pv_agents_status_ = PVDataDownloader::READY;
	LL_INFOS() << "Done parsing agents" << LL_ENDL;

	gPVDataAuth->autoMuteFlaggedAgents();
}

void PVDataAuth::autoMuteFlaggedAgents()
{
	if (!gCacheName)
	{
		return;
	}
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
}

LLUUID PVDataAuth::getLockDownUUID()
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
bool PVDataAuth::isAllowedToLogin(const LLUUID& avatar_id)
{
	LL_INFOS() << "Evaluating access for " << avatar_id << "..." << LL_ENDL;
	gPVData->setErrorMessage("Generic clearance failure.");
#if PVDATA_UUID_LOCKDOWN
	LLUUID lockdown_uuid = getLockDownUUID();
	if (lockdown_uuid != LLUUID::null)
	{
		LL_INFOS() << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			LL_INFOS() << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
			return true;
		}
		gPVData->setErrorMessage("This build is locked down to another account.");
	}
	if (lockdown_uuid != LLUUID::null)
	{
		gPVData->setErrorMessage("Something went wrong, and the authentication checks have failed.");
	}
#elif DEVEL_BUILD
	if (gPVDataAuth->getSpecialAgentFlags(avatar_id) & STAFF_DEVELOPER)
	{
		return true;
	}
	gPVData->setErrorMessage("Sorry, this build is reserved for [APP_NAME] developers. Please download a public build at " + LLTrans::getString("ViewerDownloadURL") + ".");
#endif
	// NOTE: We make an exception here and cache the agent flags due to the amount of find() calls that would be generated otherwise.
	// We should probably make a new class for each agent or something... - Xenhat
	auto av_flags = gPVDataAuth->getSpecialAgentFlags(avatar_id);
	if (av_flags & BAD_USER_BANNED)
	{
		gPVData->setErrorMessage("Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be a mistake, restart the viewer. Otherwise, Please download [https://get.secondlife.com another Viewer].");
		return false;
	}
	auto compiled_channel = LLVersionInfo::getCompiledChannel();
	if (compiled_channel == APP_NAME + " Release"
		// Allow beta builds as well.
		|| compiled_channel == APP_NAME + " Beta")
	{
		return true;
	}
	//else
	{
		// prevent non-release builds to fall in the wrong hands
		LL_WARNS() << "Not a Release build; evaluating access level..." << LL_ENDL;
		LL_WARNS() << "RAW Access level for '" << avatar_id << "' : '" << av_flags << "'" << LL_ENDL;
		if (av_flags & STAFF_DEVELOPER)
		{
			LL_WARNS() << "Access level: DEVELOPER" << LL_ENDL;
			return true;
		}
		if (av_flags & STAFF_SUPPORT)
		{
			LL_WARNS() << "Access level: SUPPORT" << LL_ENDL;
			return true;
		}
		if (av_flags & STAFF_QA)
		{
			LL_WARNS() << "Access level: QA" << LL_ENDL;
			return true;
		}
		if (av_flags & USER_TESTER)
		{
			LL_WARNS() << "Access level: TESTER" << LL_ENDL;
			return true;
		}
		LL_WARNS() << "Access level: NONE" << LL_ENDL;
		gPVData->setErrorMessage("You do not have clearance to use this build of [APP_NAME].\nIf you believe this to be a mistake, contact the [APP_NAME] Viewer support. Otherwise, please download a public build at\n" + LLTrans::getString("ViewerDownloadURL") + ".");
	}
	return false;
}

/**
 * \brief Determines if the current binary is a known, and blocked release.
 * \return true if the release is blocked, false if allowed.
 */
bool PVDataViewerInfo::isBlockedRelease()
{
	// This little bit of code here does a few things. First it grabs the viewer's current version. Then it attempts to find that specific version
	// in the list of blocked versions (blocked_versions_).
	// If the version is found, it assigns the version's index to the iterator 'iter', otherwise assigns map::find's retun value which is 'map::end'
	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	const std::string& sCurrentVersionShort = LLVersionInfo::getShortVersion();
	// Blocked Versions
	auto blockedver_iterator = blocked_versions_.find(sCurrentVersion);
	// Minimum Version
	auto minver_iterator = minimum_version_.begin();
	gPVData->setErrorMessage("Quit living in the past!");

	// TODO v7: Check if isVersionAtOrAboveMinimum is more suitable
	
	// Check if version is lower than the minimum version
	if (minver_iterator != minimum_version_.end() // Otherwise crashes if data is missing due to network failures
		&& sCurrentVersionShort < minver_iterator->first)
	{
		const LLSD& reason_llsd = minver_iterator->second;
		gPVData->setErrorMessage(reason_llsd["REASON"]);
		LL_WARNS() << sCurrentVersion << " is not allowed to be used anymore (" << gPVData->getErrorMessage() << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	// Check if version is explicitly blocked
	if (blockedver_iterator != blocked_versions_.end()) // if the iterator's value is map::end, it is not in the array.
	{
		// assign the iterator's associated value (the reason message) to the LLSD that will be returned to the calling function
		const LLSD& reason_llsd = blockedver_iterator->second;
		gPVData->setErrorMessage(reason_llsd["REASON"]);
		LL_WARNS() << sCurrentVersion << " is not allowed to be used anymore (" << gPVData->getErrorMessage() << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	gPVData->PV_DEBUG(sCurrentVersion + " not found in the blocked releases list", LLError::LEVEL_DEBUG);

	// default
	return false;
}

inline S32 PVDataAuth::getSpecialAgentFlags(const LLUUID& avatar_id)
{
	if (gPVDataAuth->pv_special_agent_flags_.count(avatar_id) != 0)
	{
		return gPVDataAuth->pv_special_agent_flags_.at(avatar_id);
	}
	return 0;
}

bool PVDataAuth::isUserDevStaff(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & STAFF_DEVELOPER);
}

bool PVDataAuth::isUserSupportStaff(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & STAFF_SUPPORT);
}

bool PVDataAuth::isUserQAStaff(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & STAFF_QA);
}

bool PVDataAuth::isUserTester(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & USER_TESTER);
}

bool PVDataAuth::isUserUnsupported(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & BAD_USER_UNSUPPORTED);
}

bool PVDataAuth::isUserAutoMuted(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & BAD_USER_AUTOMUTED);
}

bool PVDataAuth::isUserBanned(const LLUUID& avatar_id)
{
	return (getSpecialAgentFlags(avatar_id) & BAD_USER_BANNED);
}

bool PVDataAuth::isSupportGroup(const LLUUID& avatar_id) const
{
	return (support_group_.count(avatar_id));
}

bool PVDataAuth::isUserPolarized(const LLUUID& avatar_id)
{
	// TODO: Re-order flags by hierarchy again and make this nicer
	//auto flags = getAgentFlags(avatar_id);
	//return (flags > BAD_USER_UNSUPPORTED && flags != DEPRECATED_TITLE_OVERRIDE);
	return (gPVDataAuth->getSpecialAgentFlags(avatar_id) != 0 &&
		!gPVDataAuth->isUserAutoMuted(avatar_id) &&
		!gPVDataAuth->isUserBanned(avatar_id) &&
		!gPVDataAuth->isUserUnsupported(avatar_id));
}

bool PVDataAuth::isLinden(const LLUUID& avatar_id, S32& av_flags) const
{
#if !FIXED_LINDEN_CHECK
	// Our shit is broken, let's fall back to the basic code.
	std::string first_name, last_name;
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(avatar_id, &av_name))
	{
		std::istringstream full_name(av_name.getUserName());
		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(avatar_id, first_name, last_name);
	}

	return (last_name == LL_LINDEN ||
		last_name == LL_MOLE ||
		last_name == LL_PRODUCTENGINE ||
		last_name == LL_SCOUT ||
		last_name == LL_TESTER);
#else
	// <Polarity> Speed up: Check if we already establed that association
	if (agents_linden_[avatar_id.asString()].asBoolean())
	{
		return true;
	}


		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(avatar_id, first_name, last_name);
	}
	if (first_name == "(waiting)" || last_name.empty()) // name cache not ready or Resident, which will never be a linden/god. abort.
	{
		return false;
	}
	if (last_name == LL_LINDEN
		|| last_name == LL_MOLE
		|| last_name == LL_PRODUCTENGINE
		|| last_name == LL_SCOUT
		|| last_name == LL_TESTER)
	{
		// set bit for LL employee
		PV_DEBUG(first_name + " " + last_name + " is a linden!", LLError::LEVEL_INFO);
		//pv_special_agent_flags_[avatar_id.asString()] = (av_flags |= LINDEN_EMPLOYEE);
		agents_linden_[avatar_id.asString()] = true;
		return true;
	}

	PV_DEBUG(first_name + (last_name.empty() ? "" : " " + last_name) + " is NOT a linden!", LLError::LEVEL_DEBUG);
	return false;
#endif
}

bool PVDataAuth::getSpecialAgentCustomTitle(const LLUUID& avatar_id, std::string& new_title)
{
	if(pv_special_agent_title_.find(avatar_id) != pv_special_agent_title_.end())
	{
		new_title = pv_special_agent_title_[avatar_id];
	}
	return (!new_title.empty());
}
// Checks on the agent using the viewer

std::string PVDataAuth::getAgentFlagsAsString(const LLUUID& avatar_id)
{
	// Check for agents flagged through PVData
	std::string agent_title = "";
	std::vector<std::string> flags_list;
	S32 av_flags = getSpecialAgentFlags(avatar_id);
	if (isLinden(avatar_id, av_flags))
	{
		flags_list.push_back("Linden Lab Employee");
	}
	if (av_flags != 0 || !flags_list.empty())
	{
		// LL_WARNS() << "Agent Flags for " << avatar_id << " = " << av_flags << LL_ENDL;
		std::string custom_title;
		//auto title_ptr *
		if (getSpecialAgentCustomTitle(avatar_id, custom_title))
		{
			// Custom tag present, drop previous title to use that one instead.
			flags_list.clear();
			flags_list.push_back(custom_title);
		}
		else
		{
			// here are the bad flags
			if (isUserAutoMuted(avatar_id))
			{
				flags_list.push_back("Nuisance");
			}
			if (isUserBanned(avatar_id))
			{
				flags_list.push_back("Exiled");
			}
			if (isUserUnsupported(avatar_id))
			{
				flags_list.push_back("Unsupported");
			}
			// And here are the good flags
			if (isUserDevStaff(avatar_id))
			{
				flags_list.push_back("Developer");
			}
			if (isUserQAStaff(avatar_id))
			{
				flags_list.push_back("QA");
			}
			if (isUserSupportStaff(avatar_id))
			{
				flags_list.push_back("Support");
			}
			if (isUserTester(avatar_id))
			{
				flags_list.push_back("Tester");
			}
		}

		using namespace boost::spirit::karma;
		std::ostringstream string_stream;
		string_stream << format(string % ',', flags_list);
		agent_title = string_stream.str();
	}
	return agent_title;
}

bool PVDataAuth::isSpecialAgentColored(const LLUUID& avatar_id)
{
	return pv_special_agent_color_.find(avatar_id) != pv_special_agent_color_.end();
}

LLColor4 PVDataAuth::getSpecialAgentColorDirectly(const LLUUID& avatar_id)
{
	return pv_special_agent_color_[avatar_id];
}

void PVDataDownloader::startRefreshTimer()
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

void PVDataDownloader::refreshDataFromServer(bool force_refresh_now)
{
	// paranoia check
	if(!pvdata_refresh_timer_.getStarted())
	{
		return;
	}
	static LLCachedControl<U32> refresh_minutes(gSavedSettings, "PVData_RefreshTimeout", 60); // Minutes
	if (force_refresh_now || pvdata_refresh_timer_.getElapsedTimeF32() >= refresh_minutes * 60)
	{
		LL_INFOS() << "Attempting to live-refresh PVData" << LL_ENDL;
		gPVDataDownloader->downloadData();

		gPVData->PV_DEBUG("Attempting to live-refresh Agents data", LLError::LEVEL_DEBUG);
		gPVDataDownloader->downloadAgents();
		if (!force_refresh_now)
		{
			gPVData->PV_DEBUG("Resetting timer", LLError::LEVEL_DEBUG);
			pvdata_refresh_timer_.reset();
		}
	}
}

LLColor4 PVDataAuth::getSpecialAgentColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& show_buddy_status)
{
	// Try to operate in the same instance, reduce call overhead
	auto uiCT = LLUIColorTable::getInstance();

	LLColor4 return_color = default_color; // color we end up with at the end of the logic
	LLColor4 pvdata_color = default_color; // User color from PVData if user has one, equals return_color otherwise.

	static LLUIColor linden_color = uiCT->getColor("PlvrLindenChatColor", LLColor4::cyan);
	auto av_flags = gPVDataAuth->getSpecialAgentFlags(avatar_id);
	if (gPVDataAuth->isLinden(avatar_id, av_flags))
	{
		return linden_color;
	}
	static LLUIColor muted_color = uiCT->getColor("PlvrMutedChatColor", LLColor4::grey);

	// Some PVData-flagged users CAN be muted.
	// TODO PLVR: Do we still need this?
	if (LLMuteList::instance().isMuted(avatar_id))
	{
		return_color = muted_color.get();
		return return_color;
	}
	// Check if agent is flagged through PVData
	if (gPVDataAuth->isSpecialAgentColored(avatar_id))
	{
		pvdata_color = gPVDataAuth->getSpecialAgentColorDirectly(avatar_id);
	}
	else if (av_flags != 0) // v7 ready
	{
		// TODO: QA this
		//if (gPVDataAuth->isLinden(avatar_id, av_flags))
		//{
		//	gPVDataAuth->setSpecialAgentColor(avatar_id, linden_color.get());
		//	return linden_color.get();
		//}
		if (gPVDataAuth->isUserDevStaff(avatar_id))
		{
			static LLUIColor dev_color = uiCT->getColor("PlvrDevChatColor", LLColor4::orange);
			pvdata_color = dev_color.get();
		}
		else if (gPVDataAuth->isUserQAStaff(avatar_id))
		{
			static LLUIColor qa_color = uiCT->getColor("PlvrQAChatColor", LLColor4::red);
			pvdata_color = qa_color.get();
		}
		else if (gPVDataAuth->isUserSupportStaff(avatar_id))
		{
			static LLUIColor support_color = uiCT->getColor("PlvrSupportChatColor", LLColor4::magenta);
			pvdata_color = support_color.get();
		}
		else if (gPVDataAuth->isUserTester(avatar_id))
		{
			static LLUIColor tester_color = uiCT->getColor("PlvrTesterChatColor", LLColor4::yellow);
			pvdata_color = tester_color.get();
		}
		else if (gPVDataAuth->isUserBanned(avatar_id))
		{
			static LLUIColor banned_color = uiCT->getColor("PlvrBannedChatColor", LLColor4::grey2);
			pvdata_color = banned_color.get();
		}
		else if (gPVDataAuth->isUserAutoMuted(avatar_id))
		{
			static LLUIColor banned_color = uiCT->getColor("PlvrMutedChatColor", LLColor4::grey2);
			pvdata_color = banned_color.get();
		}
		// Unsupported users have no color.
		else
		{
			// TODO: Use localizable strings
			LLSD args;
			args["AVATAR_ID"] = avatar_id;
			args["PV_FLAGS"] = std::to_string(av_flags); // NOTE: Maybe use user friendly string instead?
			args["PV_COLOR"] = llformat("{%.5f , %.5f ,%.5f}", pvdata_color);
			args["MESSAGE"] = "Agent has deprecated or unhandled flags associated to it!";
			LLNotificationsUtil::add("PVData_ColorBug", args);
		}
		// Speedup: Put fetched agent color into cached list to speed up subsequent function calls
		gPVDataAuth->setSpecialAgentColor(avatar_id, pvdata_color);
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
	static LLCachedControl<bool> show_friends(gSavedSettings, "NameTagShowFriends");
	static LLCachedControl<bool> low_priority_friend_status(gSavedSettings, "PVColorManager_LowPriorityFriendStatus", true);
	bool show_f = (show_friends && show_buddy_status && LLAvatarTracker::instance().isBuddy(avatar_id));

	// Lengthy but fool-proof.
	if (show_f && av_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (show_f && av_flags && !low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !av_flags && low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !av_flags && !low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
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
	return return_color;
}

LLColor4 PVDataDownloader::Hex2Color4(const std::string color) const
{
	return gPVDataDownloader->Hex2Color4(stoul(color, nullptr, 16));
}
LLColor4 PVDataDownloader::Hex2Color4(int hexValue)
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
	LL_DEBUGS("PVData") << "Search separator index from settings: '" << settings_separator << "'" << LL_ENDL;
	return settings_separator;
}

void PVSearchUtil::setSearchSeparator(const U32 separator_in_u32)
{
	PVSearchSeparatorSelected = separator_in_u32;;
	LL_DEBUGS("PVData") << "Setting search separator to '" << separator_in_u32 << "'" << "('" << getSearchSeparator() << "')" << LL_ENDL;
	gSavedSettings.setU32("PVUI_SubstringSearchSeparator", separator_in_u32);

}

std::string PVSearchUtil::getSearchSeparator()
{
	auto separator = gPVSearchUtil->PVSearchSeparatorAssociation[PVSearchSeparatorSelected];
	LL_DEBUGS("PVData") << "Search separator from runtime: '" << separator << "'" << LL_ENDL;
	return separator;
}

std::string PVSearchUtil::getSearchSeparator(const U32 separator_to_get_u32) const
{
	PVSearchSeparatorSelected = separator_to_get_u32;
	return getSearchSeparator();
}

// <polarity> The Linden Lab viewer's logic is somewhat spaghetti and confusing to me, so I wrote my own.
std::string PVDataUtil::getPreferredName(const LLAvatarName& av_name)
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
void PVDataUtil::setChatLogsDirOverride()
{
#ifdef FINISHED_CHAT_LOG_WRITE
	/*
	// ReSharper disable CppDeprecatedEntity // we are cross-platform.
	auto override_location = getenv("PV_CHATLOGS_LOCATION_OVERRIDE");
	// ReSharper restore CppDeprecatedEntity
	if (override_location && override_location != gSavedPerAccountSettings.getString("InstantMessageLogPath").c_str())
	{

	LL_WARNS() << "Would set logs location to: " << override_location << LL_ENDL;
	//gSavedPerAccountSettings.setString("InstantMessageLogPath", override_location);
	//LLFloaterPreference::moveTranscriptsAndLog();
	}

	LPCWSTR name_ = L"PV_CHATLOGS_LOCATION_OVERRIDE";
	std::string value_ = log_location_from_settings;

	HKEY        key;
	HKEY        subKey;
	char const *subKeyName;

	if (es_invalid == scope_) {
	return;
	}

	switch (scope_) {
	case es_system:
	key = HKEY_LOCAL_MACHINE;
	subKeyName = systemEnvSubKey;
	break;

	case es_user:
	key = HKEY_CURRENT_USER;
	subKeyName = userEnvSubKey;
	break;
	}

	// Assign the new value.
	value_ = text;

	// Write the new value to the registry.
	RegOpenKeyEx(key, subKeyName, 0, KEY_SET_VALUE, &subKey);
	RegSetValueEx(subKey,
	name_.c_str(),
	0,
	REG_EXPAND_SZ,
	reinterpret_cast<const BYTE *>(value_.c_str()),
	value_.length() + 1);
	RegCloseKey(key);
	*/
#endif
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
	//LL_WARNS() << "Would set logs location to: " << log_location_from_settings << LL_ENDL;
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
	//auto nya = RegCreateKeyEx(HKEY_CURRENT_USER, ,0,NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,);
	// TODO: Linux and OSX support
	return value;
}

void PVDataUtil::getChatLogsDirOverride()
{
	std::string log_location_from_settings = gSavedPerAccountSettings.getString("InstantMessageLogPath");
	std::string registry_key = "PV_CHATLOGS_LOCATION_OVERRIDE"; // TODO: Move to global
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
	gPVData->PV_DEBUG("Would set logs location to: " + new_chat_logs_dir, LLError::LEVEL_WARN);
	gPVData->PV_DEBUG("gDirUtilp->getChatLogsDir() = " + gDirUtilp->getChatLogsDir(), LLError::LEVEL_WARN);

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
		gPVData->PV_DEBUG("Hmmm strange, location mismatch: " + new_chat_logs_dir + " != " + gDirUtilp->getChatLogsDir(), LLError::LEVEL_WARN);
	}

	gSavedPerAccountSettings.setString("InstantMessageLogPath", new_chat_logs_dir);
}

// Copied from LLFloaterPreferences because we need to run this without a floater instance existing.
bool PVDataUtil::moveTranscriptsAndLog(std::string userid) const
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

std::string PVDataAuth::getToken()
{
#if INTERNAL_BUILD
	std::string token = gSavedSettings.getString("PVAuth_TesterToken");
	if (token.length() != 32 || token == "00000000000000000000000000000000")
	{
		gSavedSettings.setString("PVAuth_TesterToken", ""); // stupid LLSD defaults empty strings to '0'
	}
	return gSavedSettings.getString("PVAuth_TesterToken");
#else
	return "";
#endif
}
