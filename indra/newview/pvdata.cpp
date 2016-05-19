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
* The Polarity Viewer Project
* http://www.polarityviewer.org
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"
#include "pvdata.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "pvconstants.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
//#include <boost/filesystem.hpp>
#include <boost/spirit/include/karma.hpp>

#include "llagent.h"
#include "llhttpclient.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llfloaterabout.h"
#include "pvcommon.h"
#include "llstartup.h"

// ##     ## ######## ######## ########     ##        #######   ######   ####  ######
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##         ##  ##
// #########    ##       ##    ########     ##       ##     ## ##   ####  ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##           ########  #######   ######   ####  ######

// Local timeout override to ensure we don't abort too soon
const F32 HTTP_TIMEOUT = 30.f;

// We make exception of the coding style guide here because this class is only used internally, and is not
// interacted directly with when hooking up to PVData from other files

// Think of this one as a private class.
class PVDataDownloader: public LLHTTPClient::Responder
{
	// TODO: Can't we cache the PVData instance per request?
	LOG_CLASS(PVDataDownloader);
public:
	PVDataDownloader(const std::string& http_request_url, const std::string& file_name) :
		mHTTPSourceURL(http_request_url),
		mSourceFileName(file_name)
	{
		// No-op
	}

	void httpSuccess() override
	{
		LL_DEBUGS("PVDataDownloader") << "http success with HTTP[" << LLCurl::Responder::getStatus() << "] for " << mHTTPSourceURL << LL_ENDL;
		// Do not rely on curl's error status and query the parser instead since a malformed file would return [200] OK but not parse properly.
		if (mDeserializeError)
		{
			LL_DEBUGS("PVDataDownloader") << "parse failure for " << mHTTPSourceURL << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("PVDataDownloader") << "parse success for " << mHTTPSourceURL << LL_ENDL;
		}
		PVData::instance().handleServerResponse(getContent(),				// LLSD& http_content
												mHTTPSourceURL,			// const std::string& http_source_url
												mSourceFileName,		// const std::string& data_file_name
												mDeserializeError,	// const bool& parse_failure
												false);							// const bool& http_failure
	}

	void httpFailure() override
	{
		LL_DEBUGS("PVDataDownloader") << "http failure with HTTP[" << LLCurl::Responder::getStatus() << "] for " << mHTTPSourceURL << LL_ENDL;
		PVData::instance().handleServerResponse(getContent(),				// LLSD& http_content
												mHTTPSourceURL,			// const std::string& http_source_url
												mSourceFileName,		// const std::string& data_file_name
												mDeserializeError,	// const bool& parse_failure
												true);							// const bool& http_failure
	}

private:
	std::string mHTTPSourceURL;
	std::string mSourceFileName;
	//LLDate mLastModified;
};

// ########   #######  ##      ## ##    ## ##        #######     ###    ########  ######## ########
// ##     ## ##     ## ##  ##  ## ###   ## ##       ##     ##   ## ##   ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ####  ## ##       ##     ##  ##   ##  ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ## ## ## ##       ##     ## ##     ## ##     ## ######   ########
// ##     ## ##     ## ##  ##  ## ##  #### ##       ##     ## ######### ##     ## ##       ##   ##
// ##     ## ##     ## ##  ##  ## ##   ### ##       ##     ## ##     ## ##     ## ##       ##    ##
// ########   #######   ###  ###  ##    ## ########  #######  ##     ## ########  ######## ##     ##

PVData::PVData()
	//eDataParseStatus(INIT),
	//eAgentsParseStatus(INIT)
{
}

void PVData::modularDownloader(const std::string& pfile_name_in)
{
	// Sets up the variables we need for each object. Avoids call bloat in the class constructor.
	mPVDataUserAgent = LLVersionInfo::getChannelAndVersionStatic();
	mPVDataViewerVersion = LLViewerMedia::getCurrentUserAgent();
	static LLCachedControl<bool> use_testing_source(gSavedSettings, "PVData_UseTestingDataSource", false);
	mPVDataRemoteURLBase = (use_testing_source) ? "https://data.polarityviewer.org/test/5/" : "https://data.polarityviewer.org/live/5/";

	// construct download url from file name
	mHeaders.insert("User-Agent", mPVDataUserAgent);
	mHeaders.insert("viewer-version", mPVDataViewerVersion);
	mPVDataModularRemoteURLFull = mPVDataRemoteURLBase + pfile_name_in;
	LL_DEBUGS("PVData") << "Downloading " << pfile_name_in << " from " << mPVDataModularRemoteURLFull << LL_ENDL;
	// TODO: HTTP eTag support
	LLHTTPClient::get(mPVDataModularRemoteURLFull, new PVDataDownloader(mPVDataModularRemoteURLFull, pfile_name_in), mHeaders, HTTP_TIMEOUT);
}

void PVData::downloadData()
{
	if (PVData::canDownload(eDataDownloadStatus))
	{
		eDataParseStatus = INIT;
		modularDownloader("data.xml");
	}
}

void PVData::downloadAgents()
{
	if (PVData::canDownload(eAgentsDownloadStatus))
	{
		eAgentsParseStatus = INIT;
		modularDownloader("agents.xml");
	}
}

void PVData::handleServerResponse(const LLSD& http_content, const std::string& http_source_url, const std::string& data_file_name, const bool& parse_failure, const bool& http_failure)
{
	LL_DEBUGS("PVData") << "Examining HTTP response for " << http_source_url << LL_ENDL;
	LL_DEBUGS("PVData") << "http_content=" << http_content << LL_ENDL;
	LL_DEBUGS("PVData") << "http_source_url=" << http_source_url << LL_ENDL;
	LL_DEBUGS("PVData") << "data_file_name=" << data_file_name << LL_ENDL;
	LL_DEBUGS("PVData") << "parse_failure=" << parse_failure << LL_ENDL;
	LL_DEBUGS("PVData") << "http_failure=" << http_failure << LL_ENDL;

	std::string expected_url = mPVDataRemoteURLBase + data_file_name;
	if (http_source_url != expected_url)
	{
		// something isn't quite right
		LL_ERRS("PVData") << "Received " << http_source_url << " which was not expected (expecting " << expected_url << "). Aborting!" << LL_ENDL;
	}
	else
	{
		if (data_file_name == "data.xml")
		{
			LL_DEBUGS("PVData") << "Received a DATA file" << LL_ENDL;
			if (http_failure)
			{
				handleDataFailure();
			}
			else
			{
				eDataDownloadStatus = OK;
			}
			if (!http_failure && parse_failure)
			{
				LL_WARNS("PVData") << "Parse failure, aborting." << LL_ENDL;
				eDataParseStatus = PARSE_FAILURE;
				handleDataFailure();
			}
			else if (!http_failure && !parse_failure)
			{
				eDataParseStatus = INIT;
				LL_DEBUGS("PVData") << "Loading " << http_source_url << LL_ENDL;
				LL_DEBUGS("PVData") << "~~~~~~~~ PVDATA (web) ~~~~~~~~" << LL_ENDL;
				LL_DEBUGS("PVData") << http_content << LL_ENDL;
				LL_DEBUGS("PVData") << "~~~~~~~~ END OF PVDATA (web) ~~~~~~~~" << LL_ENDL;
				//eDataParseStatus = INIT; // Don't reset here, that would defeat the purpose.
				parsePVData(http_content);
			}
			else
			{
				LL_WARNS("PVData") << "Something unexpected happened!" << LL_ENDL;
				handleDataFailure();
			}
		}
		else if (data_file_name == "agents.xml")
		{
			LL_DEBUGS("PVData") << "Received an AGENTS file" << LL_ENDL;
			if (http_failure)
			{
				handleAgentsFailure();
			}
			else
			{
				eAgentsDownloadStatus = OK;
			}
			if (!http_failure && parse_failure)
			{
				LL_WARNS("PVData") << "Parse failure, aborting." << LL_ENDL;
				eAgentsParseStatus = PARSE_FAILURE;
				handleAgentsFailure();
			}
			else if (!http_failure && !parse_failure)
			{
				eAgentsParseStatus = INIT;
				LL_DEBUGS("PVData") << "Loading " << http_source_url << LL_ENDL;
				LL_DEBUGS("PVData") << "~~~~~~~~ PVDATA AGENTS (web) ~~~~~~~~" << LL_ENDL;
				LL_DEBUGS("PVData") << http_content << LL_ENDL;
				LL_DEBUGS("PVData") << "~~~~~~~~ END OF PVDATA AGENTS (web) ~~~~~~~~" << LL_ENDL;
				//eAgentsParseStatus = INIT; // Don't reset here, that would defeat the purpose.
				parsePVAgents(http_content);
			}
			else
			{
				LL_WARNS("PVData") << "Something unexpected happened!" << LL_ENDL;
				handleAgentsFailure();
			}
		}
		else
		{
			LL_WARNS("PVData") << "Received file didn't match any expected patterns, aborting." << LL_ENDL;
			handleAgentsFailure();
		}
	}
}

// ########     ###    ########   ######  ######## ########   ######
// ##     ##   ## ##   ##     ## ##    ## ##       ##     ## ##    ##
// ##     ##  ##   ##  ##     ## ##       ##       ##     ## ##
// ########  ##     ## ########   ######  ######   ########   ######
// ##        ######### ##   ##         ## ##       ##   ##         ##
// ##        ##     ## ##    ##  ##    ## ##       ##    ##  ##    ##
// ##        ##     ## ##     ##  ######  ######## ##     ##  ######

bool PVData::canParse(size_t& status_container) const
{
	LL_DEBUGS("PVData") << "Checking parse status" << LL_ENDL;
	bool safe_to_parse = false;
	switch (status_container)
	{
		case INIT:
		safe_to_parse = true;
		break;
		case PARSING:
		LL_WARNS("PVData") << "Parser is already running, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		//safe_to_parse = false;
		break;
		case OK:
		LL_WARNS("PVData") << "Parser already completed, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		//safe_to_parse = false;
		break;
		// TODO: Handle the other possible errors here once the checks for those have been implemented.
		default:
		LL_WARNS("PVData") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << eAgentsParseStatus << "')" << LL_ENDL;
		// TODO: Make sure this actually sets the variable...
		status_container = UNDEFINED;
		//safe_to_parse = false;
		break;
	}

	return safe_to_parse;
}

bool PVData::canDownload(size_t& status_container) const
{
	LL_DEBUGS("PVData") << "Checking parse status" << LL_ENDL;
	bool safe = false;
	switch (status_container)
	{
	case INIT:
		safe = true;
		break;
	case PARSING:
		LL_WARNS("PVData") << "Download already in progress, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		//safe = false;
		break;
	case OK:
		//LL_WARNS("PVData") << "Already downloaded, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		safe = true;
		break;
	case DOWNLOAD_FAILURE:
		LL_WARNS("PVData") << "Failed to download and will retry later (STATUS='" << status_container << "')" << LL_ENDL;
		safe = true;
		break;
		// TODO: Handle the other possible errors here once the checks for those have been implemented.
	default:
		LL_WARNS("PVData") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << eAgentsParseStatus << "')" << LL_ENDL;
		// TODO: Make sure this actually sets the variable...
		status_container = UNDEFINED;
		//safe = false;
		break;
	}

	if (!safe)
	{
		LL_WARNS("PVData") << "Download not safe, skipping!" << LL_ENDL;
	}
	return safe;
}

void PVData::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS("PVData") << "Something went wrong downloading data file" << LL_ENDL;

	gAgent.mMOTD.assign("COULD NOT CONTACT MOTD SERVER");
	eDataDownloadStatus = DOWNLOAD_FAILURE;
}
void PVData::handleAgentsFailure()
{
	LL_WARNS("PVData") << "Something went wrong downloading agents file" << LL_ENDL;

	// we might want to remove this before release...
	mAgentAccess[LLUUID("f56e83a9-da38-4230-bac8-b146e7035dfc")] = 1;
	mAgentAccess[LLUUID("6b7c1d1b-fc8a-4b11-9202-707e99b4a89a")] = 1;
	mAgentAccess[LLUUID("584d796a-bb85-4fe9-8f7c-1f2fbf2ff164")] = 24; // Darl
	mAgentAccess[LLUUID("f1a73716-4ad2-4548-9f0e-634c7a98fe86")] = 24; // Xenhat
	mAgentAccess[LLUUID("a43d30fe-e2f6-4ef5-8502-2335879ec6b1")] = 32;
	mAgentAccess[LLUUID("573129df-bf1b-46c2-9bcc-5dca94e328b2")] = 64;
	mAgentAccess[LLUUID("238afefc-74ec-4afe-a59a-9fe1400acd92")] = 64;
	eAgentsDownloadStatus = DOWNLOAD_FAILURE;
}

void PVData::parsePVData(const LLSD& data_input)
{
	LL_DEBUGS("PVData") << "Entering Parser function" << LL_ENDL;
	LL_DEBUGS("PVData") << "~~~~~~~~ PARSER ~~~~~~~~" << LL_ENDL;
	LL_DEBUGS("PVData") << data_input << LL_ENDL;
	LL_DEBUGS("PVData") << "~~~~~~~~~~~~~~~~~~~~~~~~" << LL_ENDL;
	// Make sure we don't accidentally parse multiple times. Remember to reset eDataParseStatus when parsing is needed again.
	if (!canParse(eDataParseStatus))
	{
		// FIXME: why do we get 'eDataParseStatus==PARSING' BEFORE it's actually being set? (see below)
		return;
	}
	LL_DEBUGS("PVData") << "Beginning to parse Data" << LL_ENDL;
	eDataParseStatus = PARSING;
	LL_DEBUGS("PVData") << "Attempting to find Blocked Releases" << LL_ENDL;
	if (data_input.has("BlockedReleases"))
	{
		LL_DEBUGS("PVData") << "Populating Blocked Releases list..." << LL_ENDL;
		const LLSD& blocked = data_input["BlockedReleases"];
		for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			mBlockedVersions[version] = reason;
			LL_DEBUGS("PVData") << "Added " << version << " to mBlockedVersions with reason '" << reason << "'" << LL_ENDL;

			LL_DEBUGS("PVData") << "Dumping map contents" << LL_ENDL;
			LL_DEBUGS("PVData") << "~~~~~~~~ mBlockedVersions ~~~~~~~~" << LL_ENDL;
			for (const auto &p : mBlockedVersions) {
				LL_DEBUGS("PVData") << "mBlockedVersions[" << p.first << "] = " << p.second << LL_ENDL;
			}
			LL_DEBUGS("PVData") << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << LL_ENDL;
		}
	}
	if (data_input.has("MinimumVersion"))
	{
		LL_DEBUGS("PVData") << "Getting minimum version..." << LL_ENDL;
		const LLSD& min_version = data_input["MinimumVersion"];
		for (LLSD::map_const_iterator iter = min_version.beginMap(); iter != min_version.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			mMinimumVersion[version] = reason;
			LL_INFOS("PVData") << "Minimum Version is " << version << LL_ENDL;
		}
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	LL_DEBUGS("PVData") << "Attempting to find MOTD data" << LL_ENDL;
	if (data_input.has("MOTD"))
	{
		LL_DEBUGS("PVData") << "Found a MOTD!" << LL_ENDL;
		gAgent.mMOTD.assign(data_input["MOTD"]);
	}
#if PVDATA_MOTD_CHAT
	else if (data_input.has("ChatMOTD")) // only used if MOTD is not presence in the xml file.
	{
		LL_DEBUGS("PVData") << "Found Chat MOTDs!" << LL_ENDL;
		const LLSD& motd = data_input["ChatMOTD"];
		LLSD::array_const_iterator iter = motd.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(motd.size()))))->asString());
	}
#endif // PVDATA_MOTD_CHAT

	// If the event falls within the current date, use that for MOTD instead.
	LL_DEBUGS("PVData") << "Attempting to find Events data" << LL_ENDL;
	if (data_input.has("EventsMOTD"))
	{
		const LLSD& events = data_input["EventsMOTD"];
		for (LLSD::map_const_iterator iter = events.beginMap(); iter != events.endMap(); ++iter)
		{
			std::string name = iter->first;
			const LLSD& content = iter->second;
			LL_DEBUGS("PVData") << "Found event MOTD: " << name << LL_ENDL;

			if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
			{
				LL_DEBUGS("PVData") << "Setting MOTD to " << name << LL_ENDL;
				// TODO: Shove into notification well.
				gAgent.mMOTD.assign(content["EventMOTD"]); // note singular instead of plural above
				break; // Only use the first one found.
			}
		}
	}
#endif // PVDATA_MOTD
#if PVDATA_PROGRESS_TIPS

	// TODO: Split tips files
	// <polarity> Load the progress screen tips
	LL_DEBUGS("PVData") << "Attempting to find Progress Tip data" << LL_ENDL;
	if (data_input.has("ProgressTip"))
	{
		LL_DEBUGS("PVData") << "Found Progress Tips!" << LL_ENDL;
		// Store list for later use
		memoryResidentProgressTips = data_input["ProgressTip"];
		//gAgent.mMOTD.assign(getNewProgressTipForced());
	}
#endif // PVDATA_PROGRESS_TIPS

	eDataParseStatus = OK;
	LL_INFOS("PVData") << "Done parsing data" << LL_ENDL;
}

std::string PVData::getNewProgressTipForced()
{
	// This assigns a random entry as the MOTD / Progress Tip message.
	LLSD::array_const_iterator tip_iter = memoryResidentProgressTips.beginArray();
	if (tip_iter == memoryResidentProgressTips.endArray())
		return "";
	std::string random_tip = (tip_iter + (ll_rand(static_cast<S32>(memoryResidentProgressTips.size()))))->asString();
	LL_INFOS("PVData") << "Setting Progress tip to '" << random_tip << "'" << LL_ENDL;
	return random_tip;
}

void PVData::parsePVAgents(const LLSD& data_input)
{
	LL_INFOS("PVData") << "Beginning to parse Agents" << LL_ENDL;
	LL_DEBUGS("PVData") << "~~~~~~~~ PARSER ~~~~~~~~" << LL_ENDL;
	LL_DEBUGS("PVData") << data_input << LL_ENDL;
	LL_DEBUGS("PVData") << "~~~~~~~~~~~~~~~~~~~~~~~~" << LL_ENDL;
	// Make sure we don't accidentally parse multiple times. Remember to reset eDataParseStatus when parsing is needed again.
	if (!canParse(eAgentsParseStatus))
	{
		return;
	}

	eAgentsParseStatus = PARSING;
	LL_DEBUGS("PVData") << "Attempting to find Agents Access" << LL_ENDL;
	if (data_input.has("AgentAccess"))
	{
		LL_DEBUGS("PVData") << "Found Agents data" << LL_ENDL;
		// Populate the SpecialAgents array with the key-flag associations
		const LLSD& agents_list = data_input["AgentAccess"];
		for (LLSD::map_const_iterator iter = agents_list.beginMap(); iter != agents_list.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVData") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			mAgentAccess[key] = iter->second.asInteger();
			LL_DEBUGS("PVData") << "Added " << key << " with flag \'" << mAgentAccess[key] << "\'" << LL_ENDL;
		}
	}
	LL_DEBUGS("PVData") << "Attempting to find Agents Titles" << LL_ENDL;
	if (data_input.has("AgentTitles"))
	{
		LL_DEBUGS("PVData") << "Found Agents Titles" << LL_ENDL;
		const LLSD& titles_list = data_input["AgentTitles"];
		for (LLSD::map_const_iterator iter = titles_list.beginMap(); iter != titles_list.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVData") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			mAgentTitles[key] = iter->second.asString();
			LL_DEBUGS("PVData") << "Added " << key << " with title \'" << mAgentTitles[key] << "\'" << LL_ENDL;
		}
	}
	LL_DEBUGS("PVData") << "Attempting to find Agents Colors" << LL_ENDL;
	if (data_input.has("AgentColors"))
	{
		LL_DEBUGS("PVData") << "Found Agents Colors" << LL_ENDL;
		const LLSD& data = data_input["AgentColors"];
		LL_DEBUGS("PVData") << "~~~~~~~~~~~~~~~~ AgentColors ~~~~~~~~~~~~~~~~" << "\n"
			<< data << "\n"
			<< "~~~~~~~~~~~~  END OF AgentColors ~~~~~~~~~~~~" << "\n"
			<< LL_ENDL;
		for (LLSD::map_const_iterator iter = data.beginMap(); iter != data.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVData") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			LLColor4 color;
			LLColor4::parseColor4(iter->second, &color);
			// TODO: Write unit tests to make sure this is a LLColor4.
			mAgentColors[key] = static_cast<LLColor4>(color);
			LL_DEBUGS("PVData") << "Added " << key << " with color \'" << color << "\'" << LL_ENDL;
		}
	}

	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			mSupportGroup.insert(LLUUID(itr->first));
			LL_DEBUGS("PVData") << "Added " << itr->first << " to mSupportGroup" << LL_ENDL;
		}
	}

	eAgentsParseStatus = OK;
	LL_INFOS("PVData") << "Done parsing agents" << LL_ENDL;
	//autoMuteFlaggedAgents();
}

bool PVData::getDataDone() const
{
	if (eDataParseStatus == OK)
	{
		return true;
	}
	return false;
}

bool PVData::getAgentsDone() const
{
	if (eAgentsParseStatus == OK)
	{
		return true;
	}
	return false;
}
std::string PVData::getPreferredName(const LLAvatarName& av_name)
{
	std::string name;
	static LLCachedControl<bool> show_username(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> use_display_names(gSavedSettings, "UseDisplayNames");
	if ((show_username) && (use_display_names))
	{
		name = av_name.getCompleteName(); // Show everything
	}
	else if (use_display_names)
	{
		name = av_name.getDisplayName();
	}
	else
	{
		name = av_name.getUserName();
	}
	return name;
}

LLUUID PVData::getLockDownUUID()
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
bool PVData::isAllowedToLogin(const LLUUID& avatar_id)
{
	PVDataErrorMessage = "Generic Error Message";
	LLUUID lockdown_uuid = getLockDownUUID();
	if (lockdown_uuid != LLUUID::null)
	{
		LL_INFOS("PVData") << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			LL_INFOS("PVData") << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
			return true;
		}
		else
		{
			PVDataErrorMessage = "This build is locked down to another account.";
			return false;
		}
	}
	else
	{
		if (lockdown_uuid != LLUUID::null)
		{
			PVDataErrorMessage = "Something went wrong, and the authentication checks have failed.";
			return false;
		}
		signed int av_flags = getAgentFlags(avatar_id);
		//LL_WARNS() << "AGENT_FLAGS = " << av_flag << LL_ENDL;
		auto compiled_channel = LLVersionInfo::getCompiledChannel();
		if (av_flags & FLAG_USER_BANNED)
		{
			PVDataErrorMessage = "Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be an error, restart the viewer. Otherwise, Please download another Viewer.";
			return false;
		}
#if RELEASE_BUILD
		// prevent non-release builds to fall in the wrong hands
		else if (compiled_channel == APP_NAME + " Release"
			// Allow beta builds as well.
			|| compiled_channel == APP_NAME + " Beta")
		{
			return true;
		}
		else
		{
			LL_WARNS("PVData") << "Not a Release build; evaluating access level..." << LL_ENDL;
			LL_WARNS("PVData") << "RAW Access level for '" << avatar_id << "' : '" << av_flags << "'" << LL_ENDL;
			if (av_flags & FLAG_USER_BETA_TESTER)
			{
				LL_WARNS() << "Access level: TESTER" << LL_ENDL;
				return true;
			}
			if (av_flags & FLAG_STAFF_SUPPORT)
			{
				LL_WARNS("PVData") << "Access level: SUPPORT" << LL_ENDL;
				return true;
			}
			if (av_flags & FLAG_STAFF_QA)
			{
				LL_WARNS("PVData") << "Access level: QA" << LL_ENDL;
				return true;
			}
			if (av_flags & FLAG_STAFF_DEV)
			{
				LL_WARNS("PVData") << "Access level: DEVELOPER" << LL_ENDL;
				return true;
			}
			else
			{
				LL_WARNS("PVData") << "Access level: NONE" << LL_ENDL;
				PVDataErrorMessage = "You do not have permission to use this build of [APP_NAME]. Please wait for the public release.";
				return false;
			}
		}
#else
		else
		{
			return true;
		}
#endif // RELEASE_BUILD
	}
	//LL_WARNS("PVData") << "[PVDataErrorMessage] " << PVDataErrorMessage << LL_ENDL;
	//return false;
}

bool PVData::isBlockedRelease()
{
	// This little bit of code here does a few things. First it grabs the viewer's current version. Then it attempts to find that specific version
	// in the list of blocked versions (mBlockedVersions).
	// If the version is found, it assigns the version's index to the iterator 'iter', otherwise assigns map::find's retun value which is 'map::end'
	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	const std::string& sCurrentVersionShort = LLVersionInfo::getShortVersion();
	// Blocked Versions
	str_llsd_pairs::iterator blockedver_iterator = mBlockedVersions.find(sCurrentVersion);
	// Minimum Version
	str_llsd_pairs::iterator minver_iterator = mMinimumVersion.begin();
	PVDataErrorMessage = "Quit living in the past!";

	// Check if version is lower than the minimum version
	if (minver_iterator != mMinimumVersion.end() // Otherwise crashes if data is missing due to network failures
		&& sCurrentVersionShort < minver_iterator->first)
	{
		const LLSD& reason_llsd = minver_iterator->second;
		PVDataErrorMessage.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << PVDataErrorMessage << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	// Check if version is explicitely blocked
	if (blockedver_iterator != mBlockedVersions.end()) // if the iterator's value is map::end, it is not in the array.
	{
		// assign the iterator's associaded value (the reason message) to the LLSD that will be returned to the calling function
		const LLSD& reason_llsd = blockedver_iterator->second;
		PVDataErrorMessage.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << PVDataErrorMessage << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	else
	{
		LL_DEBUGS("PVData") << sCurrentVersion << " not found in the blocked releases list" << LL_ENDL;
	}

	// default
	return false;
}

int PVData::getAgentFlags(const LLUUID& avatar_id)
{
	int flags = mAgentAccess[avatar_id];

	// Will crash if name resolution is not available yet
	if (LLStartUp::getStartupState() >= STATE_STARTED)
	{
		if (PVCommon::isLinden(avatar_id))
		{
			// set bit for LL employee
			flags = flags |= FLAG_LINDEN_EMPLOYEE;
			mAgentAccess[avatar_id] = flags;
		}
	}
	LL_DEBUGS("PVData") << "Returning '" << flags << "'" << LL_ENDL;
	return flags;
}

bool PVData::is(const LLUUID& avatar_id, const U32& av_flag)
{
	return (mAgentAccess[avatar_id] & av_flag);
}

bool PVData::isSpecial(const LLUUID& avatar_id) const
{
	return (mAgentAccess.count(avatar_id));
}
bool PVData::isDeveloper(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_STAFF_DEV);
}

bool PVData::isSupport(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_STAFF_SUPPORT);
}

bool PVData::isQA(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_STAFF_QA);
}

bool PVData::isTester(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_BETA_TESTER);
}

bool PVData::isDeniedSupport(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_NO_SUPPORT);
}

bool PVData::isMuted(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_AUTOMUTED);
}

bool PVData::isBanned(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_BANNED);
}

bool PVData::hasColor(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_HAS_COLOR);
}

bool PVData::hasTitle(const LLUUID& avatar_id)
{
	return (mAgentAccess[avatar_id] & FLAG_USER_HAS_TITLE);
}

LLColor4 PVData::getAgentColor(const LLUUID& avatar_id)
{
	// Prevent ulterior typecasts
	LLColor4 agent_color; // Will be black here.
	agent_color = static_cast<LLColor4>(mAgentColors[avatar_id]);
	LL_DEBUGS("PVData") << "agent_color == " << agent_color << LL_ENDL;
	// TODO: Check to make sure it returns black if empty
	return agent_color;
}

// ReSharper disable CppAssignedValueIsNeverUsed
// ReSharper disable once CppParameterValueIsReassigned
// ReSharper disable CppEntityAssignedButNoRead
bool PVData::replaceWithAgentColor(const LLUUID& avatar_id, LLColor4 out_color4)
// ReSharper restore CppEntityAssignedButNoRead
{
	if (!this->hasColor(avatar_id))
		return false;
	LLColor4 agent_color; // Will be black here.
	agent_color = static_cast<LLColor4>(mAgentColors[avatar_id]);
	
	if (agent_color != LLColor4::black)
	{
		LL_DEBUGS("PVData") << "agent_color == " << agent_color << LL_ENDL;
		// calling function uses this
		out_color4 = agent_color;
		return true;
	}
	return false;
}
// ReSharper restore CppAssignedValueIsNeverUsed
// Resharper restore CppEntityAssignedButNoRead

std::string PVData::getAgentTitle(const LLUUID& avatar_id)
{
	return (mAgentTitles[avatar_id]);
}
// Checks on the agent using the viewer

std::string PVData::getAgentFlagsAsString(const LLUUID& avatar_id)
{
	// Check for agents flagged through PVData
	std::string flags_string = "";
	signed int av_flags = this->instance().getAgentFlags(avatar_id);
	if (av_flags > 0)
	{
		std::vector<std::string> flags_list;
		// TODO: Debate the need for HAS_TITLEand TITLE_OVERRIDE at the same time. We can do better.
		if (av_flags & FLAG_USER_HAS_TITLE)
		{
			flags_list.push_back(mAgentTitles[avatar_id]);
		}
		if (!(av_flags & FLAG_TITLE_OVERRIDE))
		{
			if (av_flags & FLAG_LINDEN_EMPLOYEE)
			{
				flags_list.push_back("Linden Lab Employee");
			}
			// here are the bad flags
			if (av_flags & FLAG_USER_AUTOMUTED)
			{
				flags_list.push_back("Nuisance");
			}
			if (av_flags & FLAG_USER_BANNED)
			{
				flags_list.push_back("Exiled");
			}
			if (av_flags & FLAG_USER_NO_SUPPORT)
			{
				flags_list.push_back("Unsupported");
			}
			// And here are the good flags
			if (av_flags & FLAG_STAFF_DEV)
			{
				flags_list.push_back("Developer");
			}
			if (av_flags & FLAG_STAFF_QA)
			{
				flags_list.push_back("QA");
			}
			if (av_flags & FLAG_STAFF_SUPPORT)
			{
				flags_list.push_back("Support");
			}
			if (av_flags & FLAG_USER_BETA_TESTER)
			{
				flags_list.push_back("Tester");
			}
		}
		std::ostringstream string_stream;
		using namespace boost::spirit::karma;
		string_stream << format(string % ',', flags_list);
		flags_string = string_stream.str();
		LL_DEBUGS() << "User-friendly flags for " << avatar_id << ": '" << flags_string << "'" << LL_ENDL;
	}
	return flags_string;
}

void PVData::startRefreshTimer()
{
	//LL_INFOS("PVData") << "No forced refresh" << LL_ENDL;
	if (!mPVDataRefreshTimer.getStarted())
	{
		LL_INFOS("PVData") << "Starting PVData refresh timer" << LL_ENDL;
		mPVDataRefreshTimer.start();
	}
	else
	{
		LL_WARNS("PVData") << "Timer already started!" << LL_ENDL;
	}
}

bool PVData::refreshDataFromServer(bool force_refresh_now)
{
	static LLCachedControl<U32> refresh_minutes(gSavedSettings, "PVData_RefreshTimeout", 60); // Minutes
	if (force_refresh_now || mPVDataRefreshTimer.getElapsedTimeF32() >= refresh_minutes * 60)
	{
		LL_DEBUGS("PVData") << "Attempting to live-refresh PVData" << LL_ENDL;
		PVData::instance().downloadData();

		LL_DEBUGS("PVData") << "Attempting to live-refresh Agents data" << LL_ENDL;
		PVData::instance().downloadAgents();
		if (!force_refresh_now)
		{
			LL_DEBUGS("PVData") << "Resetting timer" << LL_ENDL;
			mPVDataRefreshTimer.reset();
		}
		return true;
	}
	return false;
}