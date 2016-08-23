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

#include "boost/container/flat_set.hpp"
#include "llavatarname.h" // for convenience
#include "llerror.h" // for LOG_CLASS
#include "llsingleton.h" // for instance()

class LLColor4;
class LLUUID;

class PVData : public LLSingleton <PVData> // required for instance()
{
	friend class LLSingleton <PVData>;
	LOG_CLASS(PVData); // Required to enable the log messages prefix

	// PLVR TODO: Make this stuff private
public:

	// Constructor. Is automatically called every time a new object of this class is created.
	// This is conceptually equivalent to an include or an init script in the way that
	// every object of this class will contain the code defined here.
	// This also means that every logic within this block will run as the object is created.
	// USE SPARINGLY AND OPTIMIZE YOUR CODE.
	PVData()
	{}
	// Destructor
	~PVData()
	{}

	// This contains the re-usable LLSD data for login tips.
	// It is easier (at least for me) to parse the LLSD over and over and get a new value,
	// than storing them into a list and playing musical format conversions.
	LLSD progress_tips_list_ = LLSD::emptyMap();

	// Humorous window titles during login
	LLSD window_titles_list_ = LLSD::emptyMap();

	// Events MOTD
	LLSD motd_events_list_ = LLSD::emptyMap();
	
	// Agents access levels
	LLSD agents_access_ = LLSD::emptyMap();
	
	// Agents Titles
	LLSD agents_titles_ = LLSD::emptyMap();
	
	// Agents Colors
	LLSD agents_colors_ = LLSD::emptyMap();

	// Linden Lab employees and other God-like agents
	LLSD agents_linden_ = LLSD::emptyMap();

	enum flags_t : S32
	{
		//
		// Dear maintainer:
		//
		// Once you are done trying to 'optimize' this routine,
		// and have realized what a terrible mistake that was,
		// please increment the following counter as a warning
		// to the next guy:
		//
		// total_hours_wasted_here = 86
		//

		// Those aren't numbers. They are bits and here we use them as an array of booleans.
		// Every avatar flag has its own bit and you can combine them should such need arise.
		// REMINDER: Check against 0 for avatars not in the list, NOT -1
		FLAG_USER_BANNED = (1 << 0),      /* [0000 0000 0001] We don't want them using our stuff.        */
		FLAG_USER_AUTOMUTED = (1 << 1),   /* [0000 0000 0010] Automatically muted on login.              */
		FLAG_USER_NO_SUPPORT = (1 << 2),  /* [0000 0000 0100] User voided their warranty.                */
		FLAG_STAFF_DEV = (1 << 3),        /* [0000 0000 1000] They wrote the code you're looking at.     */
		FLAG_STAFF_QA = (1 << 4),         /* [0000 0001 0000] They approved the code you're looking at.  */
		FLAG_STAFF_SUPPORT = (1 << 5),    /* [0000 0010 0000] They help users.                           */
		FLAG_USER_BETA_TESTER = (1 << 6), /* [0000 0100 0000] They kill kittens in the name of science.  */
		//FLAG_USER_HAS_TITLE = (1 << 7),   /* [0000 1000 0000] User that deserves recognition             */
		FLAG_TITLE_OVERRIDE = (1 << 8),   /* [0001 0000 0000] Title overrides general flags list         */
		//FLAG_USER_HAS_COLOR = (1 << 9),   /* DEPRECATED [0010 0000 0000] User has a custom color         */

		// Last.
		FLAG_LINDEN_EMPLOYEE = (1 << 15), /* [1000 0000 0000 0000] Linden Lab Employee */
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
	void handleResponseFromServer(const LLSD& http_content,
		const std::string& http_source_url,
		//const std::string& data_file_name,
		const bool& parse_success
		// TODO: re-implement last-modified support
		//const bool& http_failure
		//const LLDate& last_modified
		);
	//void handleResponseFromServer(const LLSD& http_content, const std::string& http_source_url, const std::string& data_file_name, const bool& parse_failure, const bool& http_failure);

	bool isSupportGroup(const LLUUID& id) const;

	// Better version of isLinden that takes PVData into account
	bool isLinden(const LLUUID& avatar_id, S32& av_flags);

	// Returns the agent flags as a decimal number
	S32 getAgentFlags(const LLUUID& avatar_id);

	// Returns ALL the agent's flags as a comma-separated string.
	std::string getAgentFlagsAsString(const LLUUID& avatar_id);
	void startRefreshTimer();
	bool refreshDataFromServer(bool force_refresh_now);
	// Returns the agent title as a string
	bool getAgentTitle(const LLUUID& avatar_id, std::string& agent_title);

	//
	// functions to quickly find if somebody has the proper flag
	//

	// Does the avatar have any flag? Use getAgentFlag() to get which flag.
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
	// Has the avatar earned a special title?
	bool hasTitle(const LLUUID& avatar_id);

	// Returns the lockdown UUID constant as a string
	static LLUUID PVData::getLockDownUUID();

	// This returns the avatar's name in the format defined by the viewer settings.
	static std::string getPreferredName(const LLAvatarName& av_name);
	static std::string getPreferredName(const LLUUID& avatar_lluuid);
	// Returns whether or not the user can use our viewer
	bool isAllowedToLogin(const LLUUID& avatar_id);

	// Returns a string containing the reason why the current release is blocked from logging in.
	// In case the release is allowed to be used, an empty string is returned, which gets interpreted as an "OK"
	// during the login process.
	bool isBlockedRelease();

	// Force getting a new progress tip, regardless of the timer
	std::string getNewProgressTipForced();

	// Get a new progress tip (throttled)
	std::string getNewProgressTip(const std::string msg_in);

	// Contains the error message to display to the user if something goes wrong with PVData.
	std::string pvdata_error_message_ = "";

	/// <summary>
	/// Developer-only log output.
	/// </summary>
	static void PV_DEBUG(const std::string& log_in_s, const LLError::ELevel& level);
	static void Dump(const std::string name, const LLSD & map);

	// Get a color for the specified agent (UUID version)
	static LLColor4 getColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& is_buddy_and_show_it);
private:

	// This processes the main data
	void parsePVData(const LLSD& data_input);

	// This processes the agents data
	void parsePVAgents(const LLSD& data_input);

	// Temporary blob to store the hand-crafted HTTP header
	LLSD headers_;

	// Cache the variables that get inserted in the HTTP headers to avoid calling the functions every time an object is created
	std::string pvdata_user_agent_;
	std::string pvdata_viewer_version_;
	LLFrameTimer pvdata_refresh_timer_;

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
		//LOCAL_MISSING = 4,
		DOWNLOAD_FAILURE,
		UNDEFINED = 999
	};

	// Data parse status
	size_t data_parse_status_ = INIT;

	// Agents parse status
	size_t agents_parse_status_ = INIT;

	// Data parse status
	size_t data_download_status_ = INIT;

	// Agents parse status
	size_t agents_download_status_ = INIT;

	// Check if it's safe to parse data
	bool canParse(size_t& status_container) const;

	bool canDownload(size_t & status_container) const;

	// [URL COMPONENT]
	// This is the URL where the PVData data is downloaded from, minus filename
	std::string pvdata_remote_url_base_;

	// [URL COMPONENT]
	// This is the complete URL where the modular file is downloaded from, with file name.
	std::string pvdata_modular_remote_url_full_;

	// [URL COMPONENT]
	// This is the complete URL where the data file is downloaded from, with file name.
	std::string pvdata_url_full_;

	// [URL COMPONENT]
	// This is the complete URL where the agents file is downloaded from, with file name.
	std::string pvdata_agents_url_full_;

	typedef std::map<std::string, LLSD> str_llsd_pairs;
	// This contains the viewer versions that aren't allowed to be used anymore
	// TODO: Use native LLSD OR change it->second's type for performance concerns
	str_llsd_pairs blocked_versions_;

	// Minimum viewer version allowed to be used
	str_llsd_pairs minimum_version_;

	// agents <-> role associations
	typedef std::map<LLUUID, S32> flag_db_t;
	//flag_db_t mAgentAccess;

	// agents <-> title associations
	typedef std::map<LLUUID, std::string> agent_data_t;
	// agent_data_t mAgentTitles;

	// This contains the UUID of our support group
	std::set<LLUUID> support_group_;

	void handleDataFailure();
	void handleAgentsFailure();

	LLFrameTimer mTipCycleTimer; // <polarity/>	
								 // Get new progress tip if enough time elapsed since the last time this was called
	std::string last_login_tip;

public:
	// agents <-> color association	s
	typedef std::map<LLUUID, LLColor4> agent_color_map_t;
	//agent_color_map_t mAgentColors;
};
