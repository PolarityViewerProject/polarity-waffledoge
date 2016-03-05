/**
* @file pvdata.h
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

#pragma once

// Tell Compiler to leave us alone.
// boost::spirit::terminal<boost::spirit::tag::oct>::result_helper': redefinition of default parameter: parameter 3
#pragma warning (disable : 4348)

#include "llsingleton.h"
#include "llavatarname.h"   // for convenience
#include "llerror.h"
#include "boost/container/flat_set.hpp"

class PVData: public LLSingleton<PVData>
{
	friend class LLSingleton<PVData>;
	LOG_CLASS(PVData); // Required to enable the log messages prefix
public:

	// Constructor. Is automatically called every time a new object of this class is created.
	// This is conceptually equivalent to an include or an init script in the way that
	// every object of this class will contain the code defined here.
	// This also means that every logic within this block will run as the object is created.
	// USE SPARINGLY AND OPTIMIZE YOUR CODE.
	PVData();

	// This contains the raw LLSD data blob returned by the server.
	//LLSD memoryResidentDataBlob = LLSD::emptyMap();

	// This contains the re-usable LLSD data for login tips.
	// It is easier (at least for me) to parse the LLSD over and over and get a new value,
	// than storing them into a list and playing musical format conversions.
	LLSD memoryResidentProgressTips = LLSD::emptyMap();

	enum flags_t: signed int
	{
		//
		// Dear maintainer:
		//
		// Once you are done trying to 'optimize' this routine,
		// and have realized what a terrible mistake that was,
		// please increment the following counter as a warning
		// to the next guy:
		//
		// total_hours_wasted_here = 68
		//

		// Those aren't numbers. They are bits and here we use them as an array of booleans.
		// Every avatar flag has its own bit and you can combine them should such need arise.
		// REMINDER: Check against 0 for avatars not in the list, NOT -1
		FLAG_USER_BANNED = (1 << 0), /* [0000 0000 0001] We don't want them using our stuff.        */
		FLAG_USER_AUTOMUTED = (1 << 1), /* [0000 0000 0010] Automatically muted on login.              */
		FLAG_USER_NO_SUPPORT = (1 << 2), /* [0000 0000 0100] User voided their warranty.                */
		FLAG_STAFF_DEV = (1 << 3), /* [0000 0000 1000] They wrote the code you're looking at.     */
		FLAG_STAFF_QA = (1 << 4), /* [0000 0001 0000] They approved the code you're looking at.  */
		FLAG_STAFF_SUPPORT = (1 << 5), /* [0000 0010 0000] They help users.                           */
		FLAG_USER_BETA_TESTER = (1 << 6), /* [0000 0100 0000] They kill kittens in the name of science.  */
		FLAG_USER_HAS_TITLE = (1 << 7), /* [0000 1000 0000] User that deserves recognition             */
		FLAG_TITLE_OVERRIDE = (1 << 8), /* [0001 0000 0000] Title overrides general flags list         */
		FLAG_USER_HAS_COLOR = (1 << 9), /* [0010 0000 0000] User has a custom color                    */
	};

	// This returns true when the main data has been processed without errors.
	// returns current status otherwise.
	bool getDataDone() const;

	// This returns true when the agents data has been processed without errors.
	// returns current status otherwise.
	bool getAgentsDone() const;

	// This downloads the data from the server.
	// Call this just before the login screen and after the LLProxy has been setup.
	void modularDownloader(const std::string& pfile_path);

	// This downloads the data file
	void downloadData();

	// This downloads the agents file
	void downloadAgents();

	// This handles the data received from the server after downloading the data
	void PVData::handleServerResponse(const LLSD& http_content, const std::string& http_source_url, const std::string& data_file_name, const bool& parse_failure, const bool& http_failure);

	// Returns the agent flags as a LLColor4
	LLColor4 getAgentColor(const LLUUID& avatar_id);

	// Attempt to replace specified color with agent color. Returns true if replacement was made.
	bool PVData::replaceWithAgentColor(const LLUUID& avatar_id, LLColor4 out_color4);

	// Returns the agent flags as a decimal number
	int getAgentFlags(const LLUUID& avatar_id);

	// Returns ALL the agent's flags as a comma-separated string.
	std::string getAgentFlagsAsString(const LLUUID& avatar_id);

	// Returns the agent title as a string
	std::string getAgentTitle(const LLUUID& avatar_id);

	//
	// functions to quickly find if somebody has the proper flag
	//

	// Returns a boolean indicating if specified flag is attributed to the speficied agent.
	// Also returns 'false' if the agent cannot be found in the special agents list.
	bool is(const LLUUID& avatar_id, const U32& av_flag);
	// Does the avatar have any flag?
	// Use getAgentFlag() to get which flag.
	bool isSpecial(const LLUUID& avatar_id) const;
	// Is the avatar a Viewer Developer?
	bool isDeveloper(const LLUUID& avatar_id);
	// Is the avatar a Support Team Member?
	bool isSupport(const LLUUID& avatar_id);
	// Is the avatar a QA Team Member?
	bool isQA(const LLUUID& avatar_id);
	// Is the avatar a Beta Tester?
	bool isTester(const LLUUID& avatar_id);
	// Is the avatar prevented from getting support?
	bool isDeniedSupport(const LLUUID& avatar_id);
	// Is the avatar automatically muted on login?
	bool isMuted(const LLUUID& avatar_id);
	// Is the avatar denied access to the viewer?
	bool isBanned(const LLUUID& avatar_id);
	// Has the avatar earned a special color?
	bool PVData::hasColor(const LLUUID& avatar_id);
	// Has the avatar earned a special title?
	bool hasTitle(const LLUUID& avatar_id);

	// Returns the lockdown UUID constant as a string
	static LLUUID PVData::getLockDownUUID();

	// This returns the avatar's name in the format defined by the viewer settings.
	static std::string getPreferredName(const LLAvatarName& av_name);

	// Returns whether or not the user can use our viewer
	bool isAllowedToLogin(const LLUUID& avatar_id);

	// Returns a string containing the reason why the current release is blocked from logging in.
	// In case the release is allowed to be used, an empty string is returned, which gets interpreted as an "OK"
	// during the login process.
	bool isBlockedRelease();

	// Force getting a new progress tip, regardless of the timer
	std::string getNewProgressTipForced();

	// Contains the error message to display to the user if something goes wrong with PVData.
	std::string PVDataErrorMessage = "";

private:

	// This processes the main data
	void parsePVData(const LLSD& data_input);

	// This processes the agents data
	void parsePVAgents(const LLSD& data_input);

	// Temporary blob to store the hand-crafted HTTP header
	LLSD mHeaders;

	// Cache the variables that get inserted in the HTTP headers to avoid calling the functions every time an object is created
	std::string mPVDataUserAgent;
	std::string mPVDataViewerVersion;

	// Data parsing status
	enum eParseStatusList
	{
		// States
		INIT = 0,
		PARSING = 1,
		// OK
		OK = 2,
		// Errors
		PARSE_FAILURE = 3,
		LOCAL_MISSING = 4,
		UNDEFINED = 999
	};

	// Data parse status
	size_t eDataParseStatus = INIT;

	// Agents parse status
	size_t eAgentsParseStatus = INIT;

	// Check if it's safe to parse data
	bool canParse(const size_t& status_container) const;

	// [URL COMPONENT]
	// This is the URL where the PVData data is downloaded from, minus filename
	std::string mPVDataRemoteURLBase;

	// [URL COMPONENT]
	// This is the complete URL where the PVData data is downloaded from, with file name.
	std::string mPVDataModularRemoteURLFull;

	// This contains the LLSD blob received from the server.
	// Kept in memory to avoid Filesystem I/O delays and dependencies
	LLSD mPVDataMemoryData;

	// This contains the viewer versions that aren't allowed to be used anymore
	std::map<std::string, LLSD> mBlockedVersions;

	// Minimum viewer version allowed to be used
	std::map<std::string, LLSD> mMinimumVersion;

	// agents <-> role associations
	typedef std::map<LLUUID, S32> flag_db_t;
	flag_db_t mAgentAccess;

	// agents <-> title associations
	typedef std::map<LLUUID, std::string> agent_data_t;
	agent_data_t mAgentTitles;

	// agents <-> color association	s
	typedef std::map<LLUUID, LLColor4> agent_color_map_t;
	agent_color_map_t mAgentColors;

	// This contains the progress view tips
	// Somehow Re-Sharper errors here.
	boost::container::flat_set<std::string> mProgressViewTipsList;

	// This contains the UUID of our support group
	std::set<LLUUID> mSupportGroup;

	void handleDataFailure();
	void handleAgentsFailure();
};
