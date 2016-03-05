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

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
//#include <boost/filesystem.hpp>
#include <boost/spirit/include/karma.hpp>

#include "llagent.h"
#include "llhttpclient.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"

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

PVData::PVData() :
	eDataParseStatus(INIT),
	eAgentsParseStatus(INIT)
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
	modularDownloader("data.xml");
}

void PVData::downloadAgents()
{
	modularDownloader("agents.xml");
}

void PVData::handleServerResponse(const LLSD& http_content, const std::string& http_source_url, const std::string& data_file_name, const bool& parse_failure, const bool& http_failure)
{
	LL_DEBUGS("PVDataHTTP") << "Examining HTTP response for " << http_source_url << LL_ENDL;
	LL_DEBUGS("PVDataHTTP") << "http_content=" << http_content << LL_ENDL;
	LL_DEBUGS("PVDataHTTP") << "http_source_url=" << http_source_url << LL_ENDL;
	LL_DEBUGS("PVDataHTTP") << "data_file_name=" << data_file_name << LL_ENDL;
	LL_DEBUGS("PVDataHTTP") << "parse_failure=" << parse_failure << LL_ENDL;
	LL_DEBUGS("PVDataHTTP") << "http_failure=" << http_failure << LL_ENDL;

	std::string expected_url = mPVDataRemoteURLBase + data_file_name;
	if (http_source_url != expected_url)
	{
		// something isn't quite right
		LL_ERRS("PVDataHTTP") << "Received " << http_source_url << " which was not expected (expecting " << expected_url << "). Aborting!" << LL_ENDL;
	}
	else
	{
		if (data_file_name == "data.xml")
		{
			LL_DEBUGS("PVDataHTTP") << "Received a DATA file" << LL_ENDL;
			if (http_failure)
			{
				handleDataFailure();
			}
			else if (!http_failure && parse_failure)
			{
				LL_WARNS("PVDataHTTP") << "Parse failure, aborting." << LL_ENDL;
			}
			else if (!http_failure && !parse_failure)
			{
				LL_DEBUGS("PVDataHTTP") << "Loading " << http_source_url << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << "~~~~~~~~ PVDATA (web) ~~~~~~~~" << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << http_content << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << "~~~~~~~~ END OF PVDATA (web) ~~~~~~~~" << LL_ENDL;
				//eDataParseStatus = INIT; // Don't reset here, that would defeat the purpose.
				parsePVData(http_content);
			}
			else
			{
				LL_ERRS("PVDataHTTP") << "Something unexpected happened!" << LL_ENDL;
			}
		}
		else if (data_file_name == "agents.xml")
		{
			LL_DEBUGS("PVDataHTTP") << "Received an AGENTS file" << LL_ENDL;
			if (http_failure || parse_failure)
			{
				handleAgentsFailure();
			}
			else if (!http_failure && parse_failure)
			{
				LL_WARNS("PVDataHTTP") << "Parse failure, aborting." << LL_ENDL;
			}
			else if (!http_failure && !parse_failure)
			{
				LL_DEBUGS("PVDataHTTP") << "Loading " << http_source_url << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << "~~~~~~~~ PVDATA AGENTS (web) ~~~~~~~~" << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << http_content << LL_ENDL;
				LL_DEBUGS("PVDataHTTP") << "~~~~~~~~ END OF PVDATA AGENTS (web) ~~~~~~~~" << LL_ENDL;
				//eAgentsParseStatus = INIT; // Don't reset here, that would defeat the purpose.
				parsePVAgents(http_content);
			}
			else
			{
				LL_ERRS("PVDataHTTP") << "Something unexpected happened!" << LL_ENDL;
			}
		}
		else
		{
			LL_ERRS("PVDataHTTP") << "Received file didn't match any expected patterns, aborting." << LL_ENDL;
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

bool PVData::canParse(const size_t& status_container) const
{
	LL_DEBUGS("PVDataParser") << "Checking parse status" << LL_ENDL;
	bool safe_to_parse = false;
	switch (status_container)
	{
		case INIT:
		safe_to_parse = true;
		break;
		case PARSING:
		LL_WARNS("PVDataParser") << "Parser is already running, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		//safe_to_parse = false;
		break;
		case OK:
		LL_WARNS("PVDataParser") << "Parser already completed, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		//safe_to_parse = false;
		break;
		// TODO: Handle the other possible errors here once the checks for those have been implemented.
		default:
		LL_WARNS("PVDataParser") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << eAgentsParseStatus << "')" << LL_ENDL;
		break;
	}

	return safe_to_parse;
}

void PVData::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS("PVDataHTTP") << "Something went wrong downloading data file" << LL_ENDL;

	gAgent.mMOTD.assign("Nyaaaaaaa~");
	eDataParseStatus = OK;
}
void PVData::handleAgentsFailure()
{
	LL_WARNS("PVDataHTTP") << "Something went wrong downloading agents file" << LL_ENDL;

	// we might want to remove this before release...
	mAgentAccess[LLUUID("f56e83a9-da38-4230-bac8-b146e7035dfc")] = 1;
	mAgentAccess[LLUUID("6b7c1d1b-fc8a-4b11-9202-707e99b4a89a")] = 1;
	mAgentAccess[LLUUID("584d796a-bb85-4fe9-8f7c-1f2fbf2ff164")] = 24; // Darl
	mAgentAccess[LLUUID("f1a73716-4ad2-4548-9f0e-634c7a98fe86")] = 24; // Xenhat
	mAgentAccess[LLUUID("a43d30fe-e2f6-4ef5-8502-2335879ec6b1")] = 32;
	mAgentAccess[LLUUID("573129df-bf1b-46c2-9bcc-5dca94e328b2")] = 64;
	mAgentAccess[LLUUID("238afefc-74ec-4afe-a59a-9fe1400acd92")] = 64;
	eAgentsParseStatus = OK;
}

void PVData::parsePVData(const LLSD& data_input)
{
	LL_DEBUGS("PVDataParser") << "Entering Parser function" << LL_ENDL;
	LL_DEBUGS("PVDataParser") << "~~~~~~~~ PARSER ~~~~~~~~" << LL_ENDL;
	LL_DEBUGS("PVDataParser") << data_input << LL_ENDL;
	LL_DEBUGS("PVDataParser") << "~~~~~~~~~~~~~~~~~~~~~~~~" << LL_ENDL;
	// Make sure we don't accidentally parse multiple times. Remember to reset eDataParseStatus when parsing is needed again.
	if (!canParse(eDataParseStatus))
	{
		// FIXME: why do we get 'eDataParseStatus==PARSING' BEFORE it's actually being set? (see below)
		return;
	}
	LL_DEBUGS("PVDataParser") << "Beginning to parse Data" << LL_ENDL;
	eDataParseStatus = PARSING;
	LL_DEBUGS("PVDataParser") << "Attempting to find Blocked Releases" << LL_ENDL;
	if (data_input.has("BlockedReleases"))
	{
		LL_DEBUGS("PVDataParser") << "Populating Blocked Releases list..." << LL_ENDL;
		const LLSD& blocked = data_input["BlockedReleases"];
		for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			mBlockedVersions[version] = reason;
			LL_DEBUGS("PVDataParser") << "Added " << version << " to mBlockedVersions" << LL_ENDL;
		}
	}
	if (data_input.has("MinimumVersion"))
	{
		LL_DEBUGS("PVDataParser") << "Getting minimum version..." << LL_ENDL;
		const LLSD& min_version = data_input["MinimumVersion"];
		for (LLSD::map_const_iterator iter = min_version.beginMap(); iter != min_version.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			mMinimumVersion[version] = reason;
			LL_INFOS("PVDataParser") << "Minimum Version is " << version << LL_ENDL;
		}
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	LL_DEBUGS("PVDataParser") << "Attempting to find MOTD data" << LL_ENDL;
	if (data_input.has("MOTD"))
	{
		LL_DEBUGS("PVDataParser") << "Found a MOTD!" << LL_ENDL;
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
	LL_DEBUGS("PVDataParser") << "Attempting to find Events data" << LL_ENDL;
	if (data_input.has("EventsMOTD"))
	{
		const LLSD& events = data_input["EventsMOTD"];
		for (LLSD::map_const_iterator iter = events.beginMap(); iter != events.endMap(); ++iter)
		{
			std::string name = iter->first;
			const LLSD& content = iter->second;
			LL_DEBUGS("PVDataParser") << "Found event MOTD: " << name << LL_ENDL;

			if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
			{
				LL_DEBUGS("PVDataParser") << "Setting MOTD to " << name << LL_ENDL;
				// TODO: Shove into notification well.
				gAgent.mMOTD.assign(content["EventMOTD"]); // note singular instead of plural above
				break; // Only use the first one found.
			}
		}
	}
#endif // PVDATA_MOTD
#if PVDATA_PROGRESS_TIPS

	// TODO: Split tips files
	// <Polarity:Xenhat> Load the progress screen tips
	LL_DEBUGS("PVDataParser") << "Attempting to find Progress Tip data" << LL_ENDL;
	if (data_input.has("ProgressTip"))
	{
		LL_DEBUGS("PVDataParser") << "Found Progress Tips!" << LL_ENDL;
		// Store list for later use
		memoryResidentProgressTips = data_input["ProgressTip"];
		gAgent.mMOTD.assign(getNewProgressTipForced());
	}
#endif // PVDATA_PROGRESS_TIPS

	eDataParseStatus = OK;
	LL_INFOS("PVDataParser") << "Done parsing data" << LL_ENDL;
}

std::string PVData::getNewProgressTipForced()
{
	// This assigns a random entry as the MOTD / Progress Tip message.
	LLSD::array_const_iterator tip_iter = memoryResidentProgressTips.beginArray();
	if (tip_iter == memoryResidentProgressTips.endArray())
		return "";
	std::string random_tip = (tip_iter + (ll_rand(static_cast<S32>(memoryResidentProgressTips.size()))))->asString();
	LL_INFOS("PVDataParser") << "Setting Progress tip to '" << random_tip << "'" << LL_ENDL;
	return random_tip;
}

void PVData::parsePVAgents(const LLSD& data_input)
{
	LL_INFOS("PVDataParser") << "Beginning to parse Agents" << LL_ENDL;
	LL_DEBUGS("PVDataParser") << "~~~~~~~~ PARSER ~~~~~~~~" << LL_ENDL;
	LL_DEBUGS("PVDataParser") << data_input << LL_ENDL;
	LL_DEBUGS("PVDataParser") << "~~~~~~~~~~~~~~~~~~~~~~~~" << LL_ENDL;
	// Make sure we don't accidentally parse multiple times. Remember to reset eDataParseStatus when parsing is needed again.
	if (!canParse(eAgentsParseStatus))
	{
		return;
	}

	eAgentsParseStatus = PARSING;
	LL_DEBUGS("PVDataParser") << "Attempting to find Agents Access" << LL_ENDL;
	if (data_input.has("AgentAccess"))
	{
		LL_DEBUGS("PVDataParser") << "Found Agents data" << LL_ENDL;
		// Populate the SpecialAgents array with the key-flag associations
		const LLSD& agents_list = data_input["AgentAccess"];
		for (LLSD::map_const_iterator iter = agents_list.beginMap(); iter != agents_list.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVDataParser") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			mAgentAccess[key] = iter->second.asInteger();
			LL_DEBUGS("PVDataParser") << "Added " << key << " with flag \'" << mAgentAccess[key] << "\'" << LL_ENDL;
		}
	}
	LL_DEBUGS("PVDataParser") << "Attempting to find Agents Titles" << LL_ENDL;
	if (data_input.has("AgentTitles"))
	{
		LL_DEBUGS("PVDataParser") << "Found Agents Titles" << LL_ENDL;
		const LLSD& titles_list = data_input["AgentTitles"];
		for (LLSD::map_const_iterator iter = titles_list.beginMap(); iter != titles_list.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVDataParser") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			mAgentTitles[key] = iter->second.asString();
			LL_DEBUGS("PVDataParser") << "Added " << key << " with title \'" << mAgentTitles[key] << "\'" << LL_ENDL;
		}
	}
	LL_DEBUGS("PVDataParser") << "Attempting to find Agents Colors" << LL_ENDL;
	if (data_input.has("AgentColors"))
	{
		LL_DEBUGS("PVDataParser") << "Found Agents Colors" << LL_ENDL;
		const LLSD& data = data_input["AgentColors"];
		LL_DEBUGS("PVDataParser") << "~~~~~~~~~~~~~~~~ AgentColors ~~~~~~~~~~~~~~~~" << "\n"
			<< data << "\n"
			<< "~~~~~~~~~~~~  END OF AgentColors ~~~~~~~~~~~~" << "\n"
			<< LL_ENDL;
		for (LLSD::map_const_iterator iter = data.beginMap(); iter != data.endMap(); ++iter)
		{
			LLUUID key = LLUUID(iter->first);
			LL_DEBUGS("PVDataParser") << "Feeding '" << key << "' to the parser" << LL_ENDL;
			LLColor4 color;
			LLColor4::parseColor4(iter->second, &color);
			// TODO: Write unit tests to make sure this is a LLColor4.
			mAgentColors[key] = static_cast<LLColor4>(color);
			LL_DEBUGS("PVDataParser") << "Added " << key << " with color \'" << color << "\'" << LL_ENDL;
		}
	}

	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			mSupportGroup.insert(LLUUID(itr->first));
			LL_DEBUGS("PVDataParser") << "Added " << itr->first << " to mSupportGroup" << LL_ENDL;
		}
	}

	eAgentsParseStatus = OK;
	LL_INFOS("PVDataParser") << "Done parsing agents" << LL_ENDL;
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
	if (temp.length() != 36)
	{
		return LLUUID::null;
	}
	return static_cast<LLUUID>(temp);
}
bool PVData::isAllowedToLogin(const LLUUID& avatar_id)
{
	PVDataErrorMessage = "Generic Error Message";
	LLUUID lockdown_uuid = getLockDownUUID();
	// UUIDs are 36 characters. Faster than doing ((std::string)avatar_id).length all over
	if (lockdown_uuid != LLUUID::null)
	{
		// convert the string back into a uuid to compare it against another UUID.
		LL_INFOS("PVData") << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			LL_INFOS("PVData") << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
			return true;
		}
		else
		{
			PVDataErrorMessage = "This build is locked down to another account.";
		}
	}
	else
	{
		if (lockdown_uuid != LLUUID::null)
		{
			PVDataErrorMessage = "Something went wrong, and the authentication checks have failed.";
		}
		signed int av_flags = getAgentFlags(avatar_id);
		if (av_flags != 0)
		{
			//LL_WARNS() << "AGENT_FLAGS = " << av_flag << LL_ENDL;
			if (av_flags & FLAG_USER_BANNED)
			{
				PVDataErrorMessage = "Unfortunately, your have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. Please download another Viewer.";
			}
			// prevent non-release builds to fall in the wrong hands
			else if (LLVersionInfo::getCompiledChannel() == "Polarity Release")
			{
				// Don't filter Release builds
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
				if (av_flags & FLAG_STAFF_QA)
				{
					LL_WARNS("PVData") << "Access level: QA" << LL_ENDL;
					return true;
				}
				if (av_flags & FLAG_STAFF_SUPPORT)
				{
					LL_WARNS("PVData") << "Access level: SUPPORT" << LL_ENDL;
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
				}
			}
		}
	}
	LL_WARNS("PVData") << "[PVDataErrorMessage] " << PVDataErrorMessage << LL_ENDL;
	return false;
}

bool PVData::isBlockedRelease()
{
	// This little bit of code here does a few things. First it grabs the viewer's current version. Then it attempts to find that specific version
	// in the list of blocked versions (mBlockedVersions).
	// If the version is found, it assigns the version's index to the iterator 'iter', otherwise assigns map::find's retun value which is 'map::end'
	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	const std::string& sCurrentVersionShort = LLVersionInfo::getShortVersion();
	std::map<std::string, LLSD>::iterator b_iter = mBlockedVersions.find(sCurrentVersion);
	// Minimum Version
	std::map<std::string, LLSD>::iterator v_iter = mMinimumVersion.begin();
	PVDataErrorMessage = "";
	if (sCurrentVersionShort < v_iter->first)
	{
		PVDataErrorMessage = v_iter->second.asString();
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << PVDataErrorMessage << ")" << LL_ENDL;
		//TODO: fire up updater
		return true;
	}
	else if (b_iter != mBlockedVersions.end()) // if the iterator's value is map::end, it is not in the array.
	{
		// assign the iterator's associaded value (the reason message) to the LLSD that will be returned to the calling function
		PVDataErrorMessage = b_iter->second.asString();
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << PVDataErrorMessage << ")" << LL_ENDL;
		//TODO: fire up updater
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
	//std::map<LLUUID, S32>::iterator iter = mAgentAccess.find(avatar_id);
	int flags = mAgentAccess[avatar_id];
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
	LL_INFOS("") << "agent_color == " << agent_color << LL_ENDL;
	// TODO: Check to make sure it returns black if empty
	return (agent_color);
}

bool PVData::replaceWithAgentColor(const LLUUID& avatar_id, LLColor4 out_color4)
{
	if (!this->hasColor(avatar_id))
		return false;
	LLColor4 agent_color; // Will be black here.
	agent_color = static_cast<LLColor4>(mAgentColors[avatar_id]);
	
	if (agent_color != LLColor4::black)
	{
		LL_INFOS("") << "agent_color == " << agent_color << LL_ENDL;
		out_color4 = agent_color;
		return true;
	}
	return false;
}

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
		if (av_flags & FLAG_USER_HAS_TITLE)
		{
			flags_list.push_back(mAgentTitles[avatar_id]);
		}
		if (!(av_flags &FLAG_TITLE_OVERRIDE))
		{
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
