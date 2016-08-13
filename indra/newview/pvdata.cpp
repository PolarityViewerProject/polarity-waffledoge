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
#include "llsdserialize.h"
#include "llstartup.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "pvcommon.h"
#include "rlvhandler.h"
#include "llprogressview.h"

static const std::string LL_LINDEN = "Linden";
static const std::string LL_MOLE = "Mole";
static const std::string LL_PRODUCTENGINE = "ProductEngine";
static const std::string LL_SCOUT = "Scout";
static const std::string LL_TESTER = "Tester";

// ##     ## ######## ######## ########     ##        #######   ######   ####  ######
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##         ##  ##
// #########    ##       ##    ########     ##       ##     ## ##   ####  ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##           ########  #######   ######   ####  ######

// Local timeout override to ensure we don't abort too soon
const F32 HTTP_TIMEOUT = 30.f;

#if LL_DARWIN
size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return(p ? p-s : n);
}
#endif // LL_DARWIN

// We make exception of the coding style guide here because this class is only used internally, and is not
// interacted directly with when hooking up to PVData from other files

// Crappy port

void downloadComplete( LLSD const &aData, std::string const &aURL )
{
	//LL_DEBUGS() << aData << LL_ENDL;
	
	LLSD header = aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ][ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

	LLDate lastModified;
	if (header.has("last-modified"))
	{
		lastModified.secondsSinceEpoch( PVCommon::secondsSinceEpochFromString( "%a, %d %b %Y %H:%M:%S %ZP", header["last-modified"].asString() ) );
	}

	LLSD data = aData;
	data.erase( LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS );
	
	// TODO: re-implement last-modified support
	//PVData::getInstance()->handleResponseFromServer( data, aURL,true, lastModified);
	PVData::getInstance()->handleResponseFromServer(data, aURL, true /*,lastModified*/);
}

void downloadCompleteScript( LLSD const &aData, std::string const &aURL, std::string const &aFilename  )
{
	//LL_DEBUGS() << aData << LL_ENDL;
	LLSD header = aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ][ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
	LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD( aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ] );

	LLDate lastModified;
	if (header.has("last-modified"))
	{
		lastModified.secondsSinceEpoch( PVCommon::secondsSinceEpochFromString( "%a, %d %b %Y %H:%M:%S %ZP", header["last-modified"].asString() ) );
	}
	const LLSD::Binary &rawData = aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();

	if ( status.getType() == HTTP_NOT_MODIFIED )
	{
		LL_INFOS("PVData") << "Got [304] not modified for " << aURL << LL_ENDL;
		return;
	}

	if (rawData.size() <= 0)
	{
		LL_WARNS("PVData") << "Received zero data for " << aURL << LL_ENDL;
		return;
	}

	// <polarity> We don't use this.
#if SCRIPT_LIBRARY
	// basic check for valid data received
	LLXMLNodePtr xml_root;
	std::string stringData;
	stringData.assign( rawData.begin(), rawData.end() ); // LLXMLNode::parseBuffer wants a U8*, not a const U8*, so need to copy here just to be safe
	if ( (!LLXMLNode::parseBuffer( reinterpret_cast< U8*> ( &stringData[0] ), stringData.size(), xml_root, NULL)) || (xml_root.isNull()) || (!xml_root->hasName("script_library")) )
	{
		LL_WARNS("PVData") << "Could not read the script library data from "<< aURL << LL_ENDL;
		return;
	}
		
	LLAPRFile outfile ;
	outfile.open(aFilename, LL_APR_WB);
	if (!outfile.getFileHandle())
	{
		LL_WARNS("PVData") << "Unable to open file for writing: " << aFilename << LL_ENDL;
	}
	else
	{
		LL_INFOS("PVData") << "Saving " << aFilename << LL_ENDL;
		outfile.write(  &rawData[0], rawData.size() );
		outfile.close() ;
	}
#endif // SCRIPT_LIBRARY
}

void downloadError( LLSD const &aData, std::string const &aURL )
{
	LL_WARNS() << "Failed to download " << aURL << ": " << aData << LL_ENDL;
	PVData::getInstance()->handleResponseFromServer(aData, aURL, false);
}

#ifdef COROUTINE

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
#endif // COROUTINE

// ########   #######  ##      ## ##    ## ##        #######     ###    ########  ######## ########
// ##     ## ##     ## ##  ##  ## ###   ## ##       ##     ##   ## ##   ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ####  ## ##       ##     ##  ##   ##  ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ## ## ## ##       ##     ## ##     ## ##     ## ######   ########
// ##     ## ##     ## ##  ##  ## ##  #### ##       ##     ## ######### ##     ## ##       ##   ##
// ##     ## ##     ## ##  ##  ## ##   ### ##       ##     ## ##     ## ##     ## ##       ##    ##
// ########   #######   ###  ###  ##    ## ########  #######  ##     ## ########  ######## ##     ##

void PVData::modularDownloader(const std::string& pfile_name_in)
{
	// Sets up the variables we need for each object. Avoids call bloat in the class constructor.
	pvdata_user_agent_ = LLVersionInfo::getChannelAndVersionStatic();
	pvdata_viewer_version_ = LLViewerMedia::getCurrentUserAgent();
	static LLCachedControl<bool> use_testing_source(gSavedSettings, "PVData_UseTestingDataSource", false);
	pvdata_remote_url_base_ = (use_testing_source) ? "https://data.polarityviewer.org/test/5/" : "https://data.polarityviewer.org/live/5/";

	// construct download url from file name
	headers_.insert("User-Agent", pvdata_user_agent_);
	headers_.insert("viewer-version", pvdata_viewer_version_);
	// FIXME: This is ugly
	pvdata_modular_remote_url_full_ = pvdata_remote_url_base_ + pfile_name_in;
	if (pfile_name_in == "data.xml")
	{
		pvdata_url_full_ = pvdata_modular_remote_url_full_;
	}
	else if (pfile_name_in == "agents.xml")
	{
		pvdata_agents_url_full_ = pvdata_modular_remote_url_full_;
	}

	//PV_DEBUG("Downloading " + pfile_name_in + " from " + pvdata_modular_remote_url_full_, LLError::LEVEL_INFO);
	// TODO: HTTP eTag support
	//LLHTTPClient::get(pvdata_modular_remote_url_full_, new PVDataDownloader(pvdata_modular_remote_url_full_, pfile_name_in), headers_, HTTP_TIMEOUT);
	LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet( pvdata_modular_remote_url_full_, boost::bind( downloadComplete, _1, pvdata_modular_remote_url_full_ ),
		boost::bind( downloadError, _1, pvdata_modular_remote_url_full_ ) );
}

void PVData::downloadData()
{
	if (PVData::canDownload(data_download_status_))
	{
		data_parse_status_ = INIT;
		modularDownloader("data.xml");
	}
}

void PVData::downloadAgents()
{
	if (PVData::canDownload(agents_download_status_))
	{
		agents_parse_status_ = INIT;
		modularDownloader("agents.xml");
	}
}

void PVData::handleResponseFromServer(const LLSD& http_content,
	const std::string& http_source_url,
	//const std::string& data_file_name,
	const bool& parse_success
	// TODO: re-implement last-modified support
	//const bool& http_failure
	//const LLDate& last_modified
	)
{
	static LLCachedControl<bool> dump_web_data(gSavedSettings, "PVDebug_DumpWebData", true);
	//PV_DEBUG("Examining HTTP response for " + http_source_url, LLError::LEVEL_INFO);
	//PV_DEBUG("http_content=" + http_content.asString(), LLError::LEVEL_DEBUG);
	//PV_DEBUG("http_source_url=" + http_source_url, LLError::LEVEL_DEBUG);
	//PV_DEBUG("data_file_name=" + data_file_name);
	//PV_DEBUG("parse_success=" + parse_success, LLError::LEVEL_DEBUG);
	//PV_DEBUG("http_failure=" + http_failure);

	// Set status to OK here for now.
	data_parse_status_ = agents_parse_status_ = OK;
	if (http_source_url == pvdata_url_full_)
	{
		//PV_DEBUG("Received a DATA file", LLError::LEVEL_DEBUG);
		if (!parse_success)
		{
			LL_WARNS("PVData") << "DATA Parse failure, aborting." << LL_ENDL;
			data_parse_status_ = PARSE_FAILURE;
			handleDataFailure();
		}
		else
		{
			data_parse_status_ = INIT;
			if (dump_web_data)
			{
				Dump(http_source_url, http_content);
			}
			//data_parse_status_ = INIT; // Don't reset here, that would defeat the purpose.
			parsePVData(http_content);
		}
	}
	if (http_source_url == pvdata_agents_url_full_)
	{
		//PV_DEBUG("Received an AGENTS file", LLError::LEVEL_DEBUG);
		if (!parse_success)
		{
			LL_WARNS("PVData") << " AGENTS Parse failure, aborting." << LL_ENDL;
			agents_parse_status_ = PARSE_FAILURE;
			handleAgentsFailure();
		}
		else
		{
			agents_parse_status_ = INIT;
			if (dump_web_data)
			{
				Dump(http_source_url, http_content);
			}
			//agents_parse_status_ = INIT; // Don't reset here, that would defeat the purpose.
			parsePVAgents(http_content);
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
	//PV_DEBUG("Checking parse status", LLError::LEVEL_DEBUG);
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
		LL_WARNS("PVData") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << agents_parse_status_ << "')" << LL_ENDL;
		// TODO: Make sure this actually sets the variable...
		status_container = UNDEFINED;
		//safe_to_parse = false;
		break;
	}

	return safe_to_parse;
}

bool PVData::canDownload(size_t& status_container) const
{
	//PV_DEBUG("Checking parse status", LLError::LEVEL_DEBUG);
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
		LL_WARNS("PVData") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << agents_parse_status_ << "')" << LL_ENDL;
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
	data_download_status_ = DOWNLOAD_FAILURE;
}
void PVData::handleAgentsFailure()
{
	LL_WARNS("PVData") << "Something went wrong downloading agents file" << LL_ENDL;

	// we might want to remove this before release...
	agents_access_["f56e83a9-da38-4230-bac8-b146e7035dfc"] = 1;
	agents_access_["6b7c1d1b-fc8a-4b11-9202-707e99b4a89a"] = 1;
	agents_access_["584d796a-bb85-4fe9-8f7c-1f2fbf2ff164"] = 24; // Darl
	agents_access_["f1a73716-4ad2-4548-9f0e-634c7a98fe86"] = 24; // Xenhat
	agents_access_["a43d30fe-e2f6-4ef5-8502-2335879ec6b1"] = 32;
	agents_access_["573129df-bf1b-46c2-9bcc-5dca94e328b2"] = 64;
	agents_access_["238afefc-74ec-4afe-a59a-9fe1400acd92"] = 64;
	agents_download_status_ = DOWNLOAD_FAILURE;
}

void PVData::parsePVData(const LLSD& data_input)
{
	/*
		TODO 1:	Keep a copy of data_input in memory until we segment everything
				into dedicated memory blobs
		TODO 2:	store section LLSD blobs as class member variables
				i.e:
				'const LLSD& events = data_input["EventsMOTD"];'
				becomes
				'PVData::EventsMOTD = data_input["EventsMOTD"];'
		TODO 3:	Revert custom maps into LLSD blobs if performance allows it.
	*/

	// Make sure we don't accidentally parse multiple times. Remember to reset data_parse_status_ when parsing is needed again.
	if (!canParse(data_parse_status_))
	{
		// FIXME: why do we get 'data_parse_status_==PARSING' BEFORE it's actually being set? (see below)
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	//PV_DEBUG("Beginning to parse Data", LLError::LEVEL_DEBUG);
	data_parse_status_ = PARSING;
	//PV_DEBUG("Attempting to find Blocked Releases", LLError::LEVEL_DEBUG);
	if (data_input.has("BlockedReleases"))
	{
		//PV_DEBUG("Populating Blocked Releases list...", LLError::LEVEL_DEBUG);
		const LLSD& blocked = data_input["BlockedReleases"];
		for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			blocked_versions_[version] = reason;
			//PV_DEBUG("Added " + version + " to blocked_versions_ with reason '" + reason.asString() + "'", LLError::LEVEL_DEBUG);
		}
	}
	if (data_input.has("MinimumVersion"))
	{
		//PV_DEBUG("Getting minimum version...", LLError::LEVEL_DEBUG);
		const LLSD& min_version = data_input["MinimumVersion"];
		for (LLSD::map_const_iterator iter = min_version.beginMap(); iter != min_version.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			minimum_version_[version] = reason;
			LL_DEBUGS("PVData") << "Minimum Version is " << version << LL_ENDL;
		}
	}

#ifdef PVDATA_MOTD
	// Set Message Of The Day if present
	//PV_DEBUG("Attempting to find MOTD data", LLError::LEVEL_DEBUG);
	if (data_input.has("MOTD"))
	{
		//PV_DEBUG("Found a MOTD!", LLError::LEVEL_DEBUG);
		gAgent.mMOTD.assign(data_input["MOTD"]);
	}
#ifdef PVDATA_MOTD_CHAT
	else if (data_input.has("ChatMOTD")) // only used if MOTD is not presence in the xml file.
	{
		//PV_DEBUG("Found Chat MOTDs!", LLError::LEVEL_DEBUG);
		const LLSD& motd = data_input["ChatMOTD"];
		LLSD::array_const_iterator iter = motd.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(motd.size()))))->asString());
	}
#endif // PVDATA_MOTD_CHAT

	// If the event falls within the current date, use that for MOTD instead.
	//PV_DEBUG("Attempting to find Events data", LLError::LEVEL_DEBUG);
	if (data_input.has("EventsMOTD"))
	{
		PVData::motd_events_list_ = data_input["EventsMOTD"];
		for (LLSD::map_const_iterator iter = motd_events_list_.beginMap(); iter != motd_events_list_.endMap(); ++iter)
		{
			std::string name = iter->first;
			const LLSD& content = iter->second;
			//PV_DEBUG("Found event MOTD: " + name, LLError::LEVEL_DEBUG);

			if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
			{
				//PV_DEBUG("Setting MOTD to " + name, LLError::LEVEL_DEBUG);
				// TODO: Shove into notification well.
				gAgent.mMOTD.assign(content["EventMOTD"]); // note singular instead of plural above
				break; // Only use the first one found.
			}
		}
	}
#endif // PVDATA_MOTD
#ifdef PVDATA_PROGRESS_TIPS

	// TODO: Split tips files
	// <polarity> Load the progress screen tips
	//PV_DEBUG("Attempting to find Progress Tip data", LLError::LEVEL_DEBUG);
	if (data_input.has("ProgressTip"))
	{
		//PV_DEBUG("Found Progress Tips!", LLError::LEVEL_DEBUG);
		// Store list for later use
		progress_tips_list_ = data_input["ProgressTip"];
	}
#endif // PVDATA_PROGRESS_TIPS

	if (data_input.has("WindowTitles"))
	{
		// Store list for later use
		window_titles_list_ = data_input["WindowTitles"];
	}
	data_parse_status_ = OK;
	LL_INFOS("PVData") << "Done parsing data" << LL_ENDL;

	//static LLCachedControl<bool> dump_ram_data(gSavedSettings, "PVDebug_DumpMemoryResidentData", true);
	//if (dump_ram_data)
	//{
	//	Dump("PVData (Memory)", data_input);
	//}
}

std::string PVData::getNewProgressTipForced()
{
	// This assigns a random entry as the MOTD / Progress Tip message.
	LLSD::array_const_iterator tip_iter = progress_tips_list_.beginArray();
	if (tip_iter == progress_tips_list_.endArray())
		return "";
	std::string random_tip = (tip_iter + (ll_rand(static_cast<S32>(progress_tips_list_.size()))))->asString();
	LL_INFOS("PVData") << "Setting Progress tip to '" << random_tip << "'" << LL_ENDL;
	return random_tip;
}

std::string PVData::getNewProgressTip(const std::string msg_in)
{
	LL_DEBUGS("PVData") << "Entering function" << LL_ENDL;
	// Pass the existing message right through
	if (!msg_in.empty())
	{
		LL_DEBUGS("PVData") << "returning '" << msg_in << "' in passthrough mode" << LL_ENDL;
		return msg_in;
	}
	// Use the last tip if available
	std::string return_tip = last_login_tip;
	if (mTipCycleTimer.getStarted())
	{
		static LLCachedControl<F32> progress_tip_timout(gSavedSettings, "PVUI_ProgressTipTimer", 2.f);
		if (mTipCycleTimer.getElapsedTimeF32() >= progress_tip_timout)
		{
			LL_DEBUGS("PVData") << "mTipCycleTimer elapsed; getting a new random tip" << LL_ENDL;
			LL_DEBUGS("PVData") << "Last tip was '" << last_login_tip << "'" << LL_ENDL;

			// Most likely a teleport screen; let's add something.

			return_tip = PVData::instance().progress_tips_list_.getRandom();
			LL_DEBUGS("PVData") << "New tip from function is '" << return_tip << "'" << LL_ENDL;

			if (!return_tip.empty() && return_tip != last_login_tip)
			{
				LL_INFOS("PVData") << "Setting new progress tip to '" << return_tip << "'" << LL_ENDL;
				last_login_tip = return_tip;
				//gAgent.mMOTD.assign(return_tip);
				//setMessage(return_tip);
			}
			mTipCycleTimer.reset();
		}
	}
	else
	{
		LL_WARNS("PVData") << "mTipCycleTimer not started!" << LL_ENDL;
	}

	return return_tip;
}

void PVData::parsePVAgents(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset data_parse_status_ when parsing is needed again.
	if (!canParse(agents_parse_status_))
	{
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	agents_parse_status_ = PARSING;
	LL_INFOS("PVData") << "Beginning to parse Agents" << LL_ENDL;

	// Empty data to make sure refresh works right.
	//PV_DEBUG("Attempting to find Agents Access", LLError::LEVEL_DEBUG);
	if (data_input.has("AgentAccess"))
	{
		//PV_DEBUG("Found Agents data", LLError::LEVEL_DEBUG);
		// Populate the SpecialAgents array with the key-flag associations
		PVData::agents_access_ = data_input["AgentAccess"];
		Dump("PVAgents (AgentAccess)", agents_access_);
	}
	//PV_DEBUG("Attempting to find Agents Titles", LLError::LEVEL_DEBUG);
	if (data_input.has("AgentTitles"))
	{
		//PV_DEBUG("Found Agents Titles", LLError::LEVEL_DEBUG);
		PVData::agents_titles_ = data_input["AgentTitles"];
		Dump("PVAgents (agents_titles_)", agents_titles_);
	}
	//PV_DEBUG("Attempting to find Agents Colors", LLError::LEVEL_DEBUG);
	if (data_input.has("AgentColors"))
	{
		//PV_DEBUG("Found Agents Colors", LLError::LEVEL_DEBUG);
		PVData::agents_colors_ = data_input["AgentColors"];
		Dump("PVAgents (AgentColors)", agents_colors_);
	}

	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			support_group_.insert(LLUUID(itr->first));
			//PV_DEBUG("Added " + itr->first + " to support_group_", LLError::LEVEL_DEBUG);
		}
	}

	agents_parse_status_ = OK;
	LL_INFOS("PVData") << "Done parsing agents" << LL_ENDL;

	//static LLCachedControl<bool> dump_ram_data(gSavedSettings, "PVDebug_DumpMemoryResidentData", true);
	//if (dump_ram_data)
	//{
	//	Dump("PVAgents (Memory)", data_input);
	//}

	//autoMuteFlaggedAgents();
}


bool PVData::getDataDone() const
{
	if (data_parse_status_ == OK)
	{
		return true;
	}
	return false;
}

bool PVData::getAgentsDone() const
{
	if (agents_parse_status_ == OK)
	{
		return true;
	}
	return false;
}
// <polarity> The Linden Lab viewer's logic is somewhat spaghetti and confusing to me, so I wrote my own.
std::string PVData::getPreferredName(const LLAvatarName& av_name)
{
	static LLCachedControl<bool> show_username(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> use_display_names(gSavedSettings, "UseDisplayNames");
	if (use_display_names && show_username)
	{
		return av_name.getCompleteNameForced(); // Show everything
	}
	else if (use_display_names && !show_username)
	{
		return av_name.getDisplayNameForced();
	}
	else if (!use_display_names && !show_username)
	{
		return av_name.getUserName();
	}
	else
	{
		// we shouldn't hit this, but a sane fallback can't hurt.
		return av_name.getUserName();
	}
}

// <polarity> Overload to work with UUIDs
std::string PVData::getPreferredName(const LLUUID& avatar_lluuid)
{
	static LLCachedControl<bool> show_username(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> use_display_names(gSavedSettings, "UseDisplayNames");
	// Get name via name cache
	LLAvatarName av_name;
	LLAvatarNameCache::get(avatar_lluuid, &av_name);

	if (!av_name.isValidName())
	{
		LL_WARNS("PVData") << "Name lookup failed, aborting!" << LL_ENDL;
		return "LOOKUPFAILED TRYAGAIN";
	}

	if (use_display_names && show_username)
	{
		return av_name.getCompleteNameForced(); // Show everything
	}
	else if (use_display_names && !show_username)
	{
		return av_name.getDisplayNameForced();
	}
	else if (!use_display_names && !show_username)
	{
		return av_name.getUserName();
	}
	else
	{
		// we shouldn't hit this, but a sane fallback can't hurt.
		return av_name.getUserName();
	}
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
	pvdata_error_message_ = "Generic Error Message";
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
			pvdata_error_message_ = "This build is locked down to another account.";
			return false;
		}
	}
	else
	{
		if (lockdown_uuid != LLUUID::null)
		{
			pvdata_error_message_ = "Something went wrong, and the authentication checks have failed.";
			return false;
		}
		S32 av_flags = getAgentFlags(avatar_id);
		//LL_WARNS() << "AGENT_FLAGS = " << av_flag << LL_ENDL;
		auto compiled_channel = LLVersionInfo::getCompiledChannel();
		if (av_flags & FLAG_USER_BANNED)
		{
			pvdata_error_message_ = "Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be an error, restart the viewer. Otherwise, Please download another Viewer.";
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
				pvdata_error_message_ = "You do not have permission to use this build of [APP_NAME]. Please wait for the public release.";
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
	//LL_WARNS("PVData") << "[pvdata_error_message_] " << pvdata_error_message_ << LL_ENDL;
	//return false;
}

bool PVData::isBlockedRelease()
{
	// This little bit of code here does a few things. First it grabs the viewer's current version. Then it attempts to find that specific version
	// in the list of blocked versions (blocked_versions_).
	// If the version is found, it assigns the version's index to the iterator 'iter', otherwise assigns map::find's retun value which is 'map::end'
	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	const std::string& sCurrentVersionShort = LLVersionInfo::getShortVersion();
	// Blocked Versions
	str_llsd_pairs::iterator blockedver_iterator = blocked_versions_.find(sCurrentVersion);
	// Minimum Version
	str_llsd_pairs::iterator minver_iterator = minimum_version_.begin();
	pvdata_error_message_ = "Quit living in the past!";

	// Check if version is lower than the minimum version
	if (minver_iterator != minimum_version_.end() // Otherwise crashes if data is missing due to network failures
		&& sCurrentVersionShort < minver_iterator->first)
	{
		const LLSD& reason_llsd = minver_iterator->second;
		pvdata_error_message_.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << pvdata_error_message_ << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	// Check if version is explicitely blocked
	if (blockedver_iterator != blocked_versions_.end()) // if the iterator's value is map::end, it is not in the array.
	{
		// assign the iterator's associaded value (the reason message) to the LLSD that will be returned to the calling function
		const LLSD& reason_llsd = blockedver_iterator->second;
		pvdata_error_message_.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << pvdata_error_message_ << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	else
	{
		//PV_DEBUG(sCurrentVersion + " not found in the blocked releases list", LLError::LEVEL_DEBUG);
	}

	// default
	return false;
}

S32 PVData::getAgentFlags(const LLUUID& avatar_id)
{
	S32 flags = agents_access_[avatar_id.asString()].asInteger();
	return flags;
}

bool PVData::isSpecial(const LLUUID& avatar_id) const
{
	bool special = (agents_access_[avatar_id.asString()].asInteger() > 0);
	return special;
}

bool PVData::isDeveloper(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_DEV);
}

bool PVData::isSupport(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_SUPPORT);
}

bool PVData::isQA(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_QA);
}

bool PVData::isTester(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_BETA_TESTER);
}

bool PVData::isDeniedSupport(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_NO_SUPPORT);
}

bool PVData::isMuted(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_AUTOMUTED);
}

bool PVData::isBanned(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_BANNED);
}

bool PVData::hasTitle(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_HAS_TITLE);
}

bool PVData::isSupportGroup(const LLUUID& avatar_id) const
{
	return (support_group_.count(avatar_id));
}

LLColor4 PVData::getAgentColor(const LLUUID& avatar_id)
{
	// Prevent ulterior typecasts
	LLColor4 agent_color = LLColor4::black;
	if (isSpecial(avatar_id))
	{
		LLColor4::parseColor4(agents_colors_[avatar_id.asString()], &agent_color);
	}
	//PV_DEBUG("agent_color == " + agent_color);
	return agent_color;
}

bool PVData::isLinden(const LLUUID& avatar_id, LLAvatarName &av_name)
{
	// <Polarity> Speed up: Check if we already establed that association
	if (getAgentFlags(avatar_id) & FLAG_LINDEN_EMPLOYEE)
	{
		return true;
	}

	std::string first_name, last_name;
	if (LLAvatarNameCache::get(avatar_id, &av_name))
	{
		std::istringstream full_name(av_name.getUserName());
		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(avatar_id, first_name, last_name);
	}
	if (first_name.empty())
	{
		// prevent returning 'true' when name is missing.
		return false;
	}
	bool is_linden = (last_name == LL_LINDEN ||
		last_name == LL_MOLE ||
		last_name == LL_PRODUCTENGINE ||
		last_name == LL_SCOUT ||
		last_name == LL_TESTER);

	if (is_linden)
	{
		// set bit for LL employee
		// Can't we make this more efficient?
		S32 av_flags = this->instance().getAgentFlags(avatar_id);
		av_flags = av_flags |= FLAG_LINDEN_EMPLOYEE;
		agents_access_[avatar_id.asString()] = av_flags;
	}
	return is_linden;
}

// ReSharper disable CppAssignedValueIsNeverUsed
// ReSharper disable once CppParameterValueIsReassigned
// ReSharper disable CppEntityAssignedButNoRead
bool PVData::replaceWithAgentColor(const LLUUID& avatar_id, LLColor4 out_color4)
// ReSharper restore CppEntityAssignedButNoRead
{
	LLColor4 temporary_color = getAgentColor(avatar_id);
	if (temporary_color != LLColor4::black)
	{
		out_color4 = temporary_color;
		return true;
	}
	return false;
}
// ReSharper restore CppAssignedValueIsNeverUsed
// Resharper restore CppEntityAssignedButNoRead

std::string PVData::getAgentTitle(const LLUUID& avatar_id)
{
	std::string title = agents_titles_[avatar_id.asString()].asString();
	return title;
}
// Checks on the agent using the viewer

std::string PVData::getAgentFlagsAsString(const LLUUID& avatar_id)
{
	// Check for agents flagged through PVData
	std::string flags_string = "";
	S32 av_flags = PVData::instance().getAgentFlags(avatar_id);
	if (av_flags > 0)
	{
		// LL_WARNS() << "Agent Flags for " << avatar_id << " = " << av_flags << LL_ENDL;
		std::vector<std::string> flags_list;
		// TODO: Debate the need for HAS_TITLEand TITLE_OVERRIDE at the same time. We can do better.
		if (av_flags & FLAG_USER_HAS_TITLE)
		{
			flags_list.push_back(getAgentTitle(avatar_id));
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

		using namespace boost::spirit::karma;
		std::ostringstream string_stream;
		string_stream << format(string % ',', flags_list);
		flags_string = string_stream.str();
		// LL_WARNS() << "User-friendly flags for " << avatar_id << ": '" << flags_string << "'" << LL_ENDL;
	}
	return flags_string;
}

void PVData::startRefreshTimer()
{
	//LL_INFOS("PVData") << "No forced refresh" << LL_ENDL;
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

bool PVData::refreshDataFromServer(bool force_refresh_now)
{
	static LLCachedControl<U32> refresh_minutes(gSavedSettings, "PVData_RefreshTimeout", 60); // Minutes
	if (force_refresh_now || pvdata_refresh_timer_.getElapsedTimeF32() >= refresh_minutes * 60)
	{
		LL_INFOS("PVData") << "Attempting to live-refresh PVData" << LL_ENDL;
		PVData::instance().downloadData();

		//PV_DEBUG("Attempting to live-refresh Agents data", LLError::LEVEL_DEBUG);
		PVData::instance().downloadAgents();
		if (!force_refresh_now)
		{
			//PV_DEBUG("Resetting timer", LLError::LEVEL_DEBUG);
			pvdata_refresh_timer_.reset();
		}
		return true;
	}
	return false;
}

// static
// TODO: Fix non-null terminated strings issue.
#if !RELEASE_BUILD && FIXED_STRINGS_NULL_TERM
void PVData::PV_DEBUG(const std::string log_in_s, const LLError::ELevel& level)
{
	// Skip debug entirely if the user isn't authenticated yet
	if ((LLStartUp::getStartupState() <= STATE_LOGIN_PROCESS_RESPONSE)
		|| !PVData::instance().isDeveloper(gAgentID)) // or if not a developer
	{
		return;
	}

	static LLCachedControl<bool> pvdebug_printtolog(gSavedSettings, "PVDebug_PrintToLog", true);
	if (!pvdebug_printtolog)
	{
		return;
	}

	std::string out_s = log_in_s;

	if (LLError::LEVEL_WARN == level && LLError::getDefaultLevel() == level)
	{
		LL_WARNS("PVData") << out_s << LL_ENDL;
	}
	if (LLError::LEVEL_ERROR == level && LLError::getDefaultLevel() == level)
	{
		LL_ERRS("PVData") << out_s << LL_ENDL;
	}
		if (LLError::LEVEL_WARN == level && LLError::getDefaultLevel() == level)
	LL_INFOS("PVData") << out_s << LL_ENDL;

	// plug potential leak
	out_s.clear();
	return;
}
#endif // !RELEASE_BUILD

void PVData::Dump(const std::string name, const LLSD& map)
{
	static LLCachedControl<bool> pvdebug_printtolog(gSavedSettings, "PVDebug_PrintToLog", true);
	
	if ((LLStartUp::getStartupState() <= STATE_LOGIN_PROCESS_RESPONSE) // Skip debug entirely if the user isn't authenticated yet
		|| !PVData::instance().isDeveloper(gAgentID) // or if not a developer
		|| !pvdebug_printtolog) // or if chosing not to.
	{
		return;
	}
	
	std::stringstream str;
	LLSDSerialize::toPrettyXML(map, str);
	LL_INFOS("PVData")
		<< "\n==========================="
		<< "\n<" << name << ">"
		<< "\n"
		<< str.str()
		<< "\n"
		<< "</" << name << ">"
		<< "\n==========================="
	<< LL_ENDL;
}

LLColor4 PVData::getColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& is_buddy_and_show_it)
{
	// Coloring will break immersion and identify agents even if their name is replaced, so return default color in that case.
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
	{
		return default_color;
	}
	LLColor4 return_color = default_color; // color we end up with at the end of the logic
	LLColor4 pvdata_color; // User color from PVData if user has one, equals return_color otherwise.

	static bool pvdata_color_is_valid = false;

	static const LLUIColor linden_color = LLUIColorTable::instance().getColor("PlvrLindenChatColor", LLColor4::cyan);
	static const LLUIColor muted_color = LLUIColorTable::instance().getColor("PlvrMutedChatColor", LLColor4::grey);

	// we'll need this later. Defined here to avoid multiple calls in the same code path.
	//LLAvatarName av_name;

	// Called first to seed av_name
	/*
	if (PVData::instance().isLinden(avatar_id, av_name))
	{
	// TODO: Make sure we only hit this code path once per Linden (make sure they get added properly)
	// This means we need to save the linden list somewhere probably when refreshing pvdata, or just use
	// an entirely different list. Another solution (probably the most lightweight one) would be to check
	// if a custom title has been attributed to them here instead of down there.
	return_color = linden_color.get();
	}
	*/
	// Some PVData-flagged users CAN be muted.
	if (LLMuteList::instance().isMuted(avatar_id))
	{
		return_color = muted_color.get();
		return return_color;
	}

	// Check if agent is flagged through PVData
	S32 av_flags = PVData::instance().getAgentFlags(avatar_id);
	bool has_flags = (av_flags > 0);
	if (has_flags)
	{
		pvdata_color_is_valid = true;
		if (av_flags & PVData::FLAG_USER_HAS_TITLE && !(av_flags & PVData::FLAG_TITLE_OVERRIDE))
		{
			// Do not warn when the user only has a title and no special color since it is acceptable
		}
		else if (av_flags & PVData::FLAG_LINDEN_EMPLOYEE)
		{
			// was previously flagged as employee, so will end up in this code path
			pvdata_color = linden_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_DEV)
		{
			static const LLUIColor dev_color = LLUIColorTable::instance().getColor("PlvrDevChatColor", LLColor4::orange);
			pvdata_color = dev_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_QA)
		{
			static const LLUIColor qa_color = LLUIColorTable::instance().getColor("PlvrQAChatColor", LLColor4::red);
			pvdata_color = qa_color.get();
		}
		else if (av_flags & PVData::FLAG_STAFF_SUPPORT)
		{
			static const LLUIColor support_color = LLUIColorTable::instance().getColor("PlvrSupportChatColor", LLColor4::magenta);
			pvdata_color = support_color.get();
		}
		else if (av_flags & PVData::FLAG_USER_BETA_TESTER)
		{
			static const LLUIColor tester_color = LLUIColorTable::instance().getColor("PlvrTesterChatColor", LLColor4::yellow);
			pvdata_color = tester_color.get();
		}
		else if (av_flags & PVData::FLAG_USER_BANNED)
		{
			static const LLUIColor banned_color = LLUIColorTable::instance().getColor("PlvrBannedChatColor", LLColor4::grey2);
			pvdata_color = banned_color.get();
		}
		else
		{
			LL_WARNS("PVData") << "Color Manager caught a bug! Agent is supposed to be special but no code path exists for this case!\n" << "(This is most likely caused by a missing agent flag)" << LL_ENDL;
			LL_WARNS("PVData") << "~~~~~~~ COLOR DUMP ~~~~~~~" << LL_ENDL;
			LL_WARNS("PVData") << "avatar_id = " << avatar_id << LL_ENDL;
			LL_WARNS("PVData") << "av_flags = " << av_flags << LL_ENDL;
			LL_WARNS("PVData") << "would-be pvdata_color = " << pvdata_color << LL_ENDL;
			LL_WARNS("PVData") << "~~~ END OF COLOR DUMP ~~~" << LL_ENDL;
			LL_WARNS("PVData") << "Report this occurence and send the lines above to the Polarity Developers" << LL_ENDL;
			pvdata_color_is_valid = false; // to be sure
		}
		// Special color, when defined, overrides all colors
		LLColor4 agent_color = PVData::instance().getAgentColor(avatar_id);
		if (agent_color != LLColor4::black && agent_color != LLColor4::magenta)
		{
			// No custom color defined, set as fallback
			// TODO: Use a color defined in colors.xml
			pvdata_color = agent_color;
			pvdata_color_is_valid = true;
		}
		if (!pvdata_color_is_valid)
		{
			pvdata_color = default_color;
		}
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
	bool show_f = (show_friends && is_buddy_and_show_it);

	// Lengthy but fool-proof.
	if (show_f && has_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (show_f && has_flags && !low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !has_flags && low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !has_flags && !low_priority_friend_status)
	{
		return_color = LLUIColorTable::instance().getColor("NameTagFriend", LLColor4::yellow);
	}
	if (!show_f && has_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && has_flags && !low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && !has_flags && low_priority_friend_status)
	{
		return_color = default_color;
	}
	if (!show_f && !has_flags && !low_priority_friend_status)
	{
		return_color = default_color;
	}

	return return_color;
}