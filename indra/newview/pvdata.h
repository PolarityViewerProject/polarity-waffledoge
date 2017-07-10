/**
 * @file pvdata.h
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
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#pragma once
#ifndef PV_DATA_H
#define PV_DATA_H

#include "llavatarname.h" // for convenience
#include "llerror.h" // for LOG_CLASS
#include "lluicolortable.h"
#include "pvtypes.h"

class LLColor4;
class LLColor3;
class LLUUID;

//
// Dear maintainer:
//
// Once you are done trying to 'optimize' this routine,
// and have realized what a terrible mistake that was,
// please increment the following counter as a warning
// to the next guy:
//
// total_hours_wasted_here = 145
// See also: https://xkcd.com/844/
//

/**
 * Notes about the code and the methodologies employed here
 * If you have a valid reason to disregard the information provided here, please update this documentation to reflect the new standard
 * and change the code, or leave a note for the maintainer/code grunt.
 *
 * Function parameter types:
 * When should I use const int& instead of int?
 *     The rule of thumb is, pass all data that is at most as large as the word size (32bit, 64bit) by value (generally simple data such as int), and everything else by reference to const.
 *     Also pass templated arguments by reference to const; since the compiler has access to the definition of the template, it can always optimize the reference away.
 *     see: http://stackoverflow.com/a/4705846/1570096
 *
 * However, if passing by reference requires you to make a copy of the data in the function, please refrain from doing so and pass by value instead (ie 'int' instead of 'const& int')
 * See: http://stackoverflow.com/a/7592741/1570096
 *
 */

 /*
	 Member ordering:
	 Please follow the following standard:
	 Class YourClass
	 {
	 public:
		 get stuff
		 set stuff

	 private:
		 const var
		 static var
		 private var

		 functions
		 in order
		 of execution or need
	 }

  * Concerning structure size optimization, the correct order according to viva64 is:

	 struct MyStructOpt
	 {
		 compound_type;
		 void *m_ptr;
		 size_t m_size_t;
		 int m_int; // equivalent for long, signed and unsigned
		 short m_short;
		 char m_char;
	 };
	 // Respectfully borrowed from http://www.viva64.com/en/w/V802/print/ and http://www.viva64.com/en/a/0030/ for documentation purposes

  */

// 32bit integer
// 0000 0000 0000 0000 0000 0000 0000 0000
// I tried to use bitset and couldn't work around the out_of_range exception
typedef U32 pvagent_flag;
/**
 * \brief They wrote the code you're looking at.
 *
 *	Detailed Bitset representation : [0000 0000 0000 0000 0000 0000 0000 001]
 */
const pvagent_flag STAFF_DEVELOPER = (1 << 2);
/**
 * \brief They approved the code you're looking at.
 *
 *	Detailed Bitset representation : [0000 0000 0000 0000 0000 0000 0000 0010]
 */
const pvagent_flag STAFF_QA = (1 << 3);
/**
 * \brief They help users.
 *
 *	Detailed Bitset representation : [0000 0000 0000 0000 0000 0000 0000 0100]
 */
const pvagent_flag STAFF_SUPPORT = (1 << 4);
/**
 * \brief They kill kittens in the name of science.
 *
 *	Detailed Bitset representation : [0000 0000 0000 0000 0000 0000 0000 1000]
 */
const pvagent_flag USER_TESTER = (1 << 5);
/**
 * \brief They kill kittens in the name of science.
 *
 *	Detailed Bitset representation : [0000 0000 0000 0000 0000 0000 0000 1000]
 */
const pvagent_flag USER_CONTRIBUTOR = (1 << 6);
/**
 * \brief User voided their warranty.
 *
 *	Detailed Bitset representation : [0010 0000 0000 0000 0000 0000 0000 0000]
 */
const pvagent_flag BAD_USER_UNSUPPORTED = (1 << 29);
/**
 * \brief Automatically muted on login.
 *
 *	Detailed Bitset representation : [0100 0000 0000 0000 0000 0000 0000 0000]
 */
const pvagent_flag BAD_USER_AUTOMUTED = (1 << 30);
/**
 * \brief We don't want them using our stuff.
 *
 *	Detailed Bitset representation : [1000 0000 0000 0000 0000 0000 0000 0000]
 */
const pvagent_flag BAD_USER_BANNED = (1 << 31);

// XML flags defines

const std::string WORD_STAFF_DEVELOPER		= "Developer";		/* They wrote the code you're looking at.     	*/
const std::string WORD_STAFF_QA				= "QA";		       	/* They approved the code you're looking at.  	*/
const std::string WORD_STAFF_SUPPORT		= "Support";		/* They help users.                           	*/
const std::string WORD_USER_TESTER			= "Tester";			/* They kill kittens in the name of science.  	*/
const std::string WORD_USER_CONTRIBUTOR		= "Contributor";			/* They kill kittens in the name of science.  	*/
const std::string WORD_BAD_USER_UNSUPPORTED	= "Unsupported";	/* User voided their warranty.                	*/
const std::string WORD_BAD_USER_AUTOMUTED	= "Automuted";		/* Automatically muted on login.              	*/
const std::string WORD_BAD_USER_BANNED		= "Banned";		   	/* We don't want them using our stuff.        	*/

class PVAgent
{
	// Let this class access the data as well
	friend class PVDataOldAPI;

	pvagent_flag flags;
	std::string title;
	std::string ban_reason;
	LLColor3 color;
	LLUUID uuid;
/*
	PVAgent()
	{
		uuid = LLUUID::null;
		flags = 0; // NOT SPECIAL
		title = std::string();
		color = LLColor4::black;
		ban_reason = std::string();
	};
	~PVAgent()
	{
		flags = 0; // NOT SPECIAL
		title = std::string();
		color = LLColor4::black;
		ban_reason = std::string();

		//auto it = pvAgents.find(uuid);
		//if (it != pvAgents.end())
		//{
		//	pvAgents.erase(it);
		//}
		uuid = LLUUID::null;
	};
*/
	// Functions. Remember: "Instantiating many instances of a class incurs run-time space only for its instance variables, not for any of its functions."

	/**
	* \brief Is this a Linden Employee?
	* \param last_name agent last name
	* \return bool
	*/
	static bool isLinden(const std::string& last_name);

	/**
	* \brief Is this a Mole?
	* \param last_name agent last name
	* \return bool
	*/
	static bool isMole(const std::string& last_name);

	/**
	* \brief Is this a ProductEngine employee?
	* \param last_name agent last name
	* \return bool
	*/
	static bool isProductEngine(const std::string& last_name);

	/**
	* \brief Is this a Linden Scout?
	* \param last_name agent last name
	* \return bool
	*/
	static bool isScout(const std::string& last_name);

	/**
	* \brief Is this a Linden Tester?
	* \param last_name agent last name
	* \return bool
	*/
	static bool isLLTester(const std::string& last_name);

	LLColor4 getColorInternal(const LLUIColorTable& cTablePtr);

	/**
	* \brief Returns ALL the agent's flags as a comma-separated string, or the custom title
	* \param get_custom_title get custom title instead of roles list
	* \return flags as string.
	*/
	std::string getTitle(bool get_custom_title = true);

	static PVAgent * create(const LLUUID & id, const LLColor3 & color = LLColor3::black, const S32 & flags = 0, const std::string & custom_title = std::string(), const std::string & ban_reason = std::string());

public:

	/**
	* \brief get pointer to specific agent extra data
	* \param id agent to get data from
	* \return  PVAgent*
	*/
	static PVAgent* find(const LLUUID& id);

	/**
	* \brief get color of agent.
	* \param id Agent to get the color of
	* \param default_color Color to fall back to if the agent has no color
	* \param show_buddy_status show buddy color if applicable
	* \return
	*/
	static LLColor4 getColor(const LLUUID& id, const LLColor4 &default_color, bool show_buddy_status = true);

	/**
	* \brief Get agent flags
	* \return U32 flag set
	*/
	pvagent_flag getFlags();

	/**
	* \brief get the agent's custom title, if any.
	* \param new_title variable to store the custom title into.
	* \return true if custom title exists, false otherwise.
	*/
	bool getTitleCustom(std::string& new_title);

	/**
	* \brief get human-readable list of flags
	* \param get_custom_title get custom title instead of flags list
	* \return string
	*/
	std::vector<std::string> getTitleHumanReadable(bool get_custom_title);

	/**
	* \brief Agent has a color (either custom or level default)
	* param color_out custom agent color, or LLColor4::black if none
	* \return bool
	*/
	bool hasSpecialColor(LLColor4& color_out);

	// Returns whether or not the user can use our viewer
	static bool isAllowedToLogin(const LLUUID& id, bool output_message = false);

	/**
	* \brief Determines if agent have more features/rights than regular users (for testing or security reasons)
	* \return bool
	*/
	bool isPolarized();

	/**
	* \brief Is the agent denied access to the viewer?
	* \return bool
	*/
	bool isProviderBanned();

	/**
	* \brief Is the agent a Viewer Developer?
	* \return bool
	*/
	bool isProviderDeveloper();

	/**
	* \brief Is the agent automatically muted on login?
	* \return bool
	*/
	bool isProviderMuted();

	/**
	* \briefIs the agent a QA Team Member?
	* \return bool
	*/
	bool isProviderQATeam();

	/**
	* \brief Is the agent a Support Team Member?
	* \return bool
	*/
	bool isProviderSupportTeam();

	/**
	* \brief Is the agent a Tester?
	* \return bool
	*/
	bool isProviderTester();

	/**
	* \brief Is the agent a contributor?
	* \return bool
	*/
	bool isProviderContributor();

	/**
	* \brief Is the agent prevented from getting support?
	* \return bool
	*/
	bool isProviderUnsupported();
};

// list of created agent blobs, by address and uuid
typedef std::map<LLUUID, PVAgent*> pvagent_list;
static pvagent_list pvAgents;
// OLD API
// @todo get rid of singleton. This is bad. Very bad.
class PVDataOldAPI : public LLSingleton<PVDataOldAPI> // required for instance()
{
	friend class PVAgentData;
	LLSINGLETON_EMPTY_CTOR(PVDataOldAPI);
	LOG_CLASS(PVDataOldAPI); // Required to enable the log messages prefix
public:

	std::string getErrorMessage()
	{
		return pvdata_error_message_;
	}

	void setErrorMessage(const std::string& new_error_message)
	{
		pvdata_error_message_ = new_error_message;
	}

	/**
	* \brief LLSD dumper. Does not check authentication by itself.
	* \param name Name of LLSD to display in log
	* \param map LLSD to dump
	*/
	static void Dump(const std::string &name, const LLSD &map);

private:
	/**
	 * \brief Contains the error message to display to the user if something goes wrong with PVDataOldAPI.
	 */
	std::string pvdata_error_message_ = "";

	// Public Interface
public:
	// old api below
	/**
	* \brief Create generic entries for agents who should always have access should data fails to be acquired
	*/
	void setFallbackAgentsData();

	/**
	* \brief Check if supplied group is one of/the vendor's support group
	* \param id group UUID
	* \return bool
	*/
	bool isSupportGroup(const LLUUID& id) const;

	// Returns the lockdown UUID constant as a string
	static LLUUID getLockDownUUID();

	/**
	* \brief This returns the agent's name in the format defined by the viewer settings.
	* \param av_name agent name
	* \return preferred name
	*/
	static std::string getPreferredName(const LLAvatarName& av_name);

	std::string getToken();
	std::string getEventMotdIfAny();
	void checkBeggar(const LLUUID & id, const std::string & message);
	// setters

	/**
	* \brief Set flag to agent
	* \param id agent UUID
	*/
	void setVendorSupportGroup(const LLUUID& id);

	// NOTE: Maybe return success?
	void autoMuteFlaggedAgents();

private:

	/**
	* \brief Special agents and their level for quick lookup
	*/
	pv_pair_uuid_sint pv_special_agent_flags_;

	/**
	* \brief Special agent and their titles for quick lookup
	*/
	pv_pair_uuid_llcolor4 pv_special_agent_color_;

	// Agents Colors
	pv_pair_uuid_string pv_special_agent_title_;

	// Linden Lab employees and other God-like agents
	std::vector<LLUUID> agents_linden_;

	// Ban reason, if present
	pv_pair_uuid_string pv_special_agent_ban_reason_;

public:
	// This contains the UUID of our support group
	std::set<LLUUID> support_group_;

	void setDataStatus(const S32& status) { pv_data_status_ = status; }
	void setAgentsDataStatus(const S32& status) { pv_agents_status_ = status; }

	/**
	 * \brief This downloads the data file
	 */
	void downloadData();

	// Cache the variables that get inserted in the HTTP headers to avoid calling the functions every time an object is created
	std::string pvdata_user_agent_;
	std::string pvdata_viewer_version_;

	// Temporary blob to store the hand-crafted HTTP header
	LLSD headers_;

	enum pv_files : U8
	{
		DATA,
		AGENTS
	};
	/**
	* \brief  This downloads the requested data file from the server.
	*/
	void modularDownloader(const U8 pfile_name_in);

	/**
	 * \brief Check if data downloader/parser is done with the data.
	 * \return true or false
	 */
	bool getDataDone();

	/**
	 * \brief Check if data downloader/parser is done with the agents data.
	 * \return true or false
	 */
	bool getAgentsDone();

	void startRefreshTimer();

	void refreshDataFromServer(bool force_refresh_now = false);
	void cleanup();

private:

	const enum pv_data_sections_index
	{
		MinimumVersion,
		BlockedReleases,
		MOTD,
		ChatMOTD,
		EventsMOTD,
		WindowTitles,
		ProgressTip,
	};
	const std::vector<std::string> pv_data_sections_ =
	{
		"MinimumVersion",
		"BlockedReleases",
		"MOTD",
		"ChatMOTD",
		"EventsMOTD",
		"WindowTitles",
		"ProgressTip",
	};

	// Local timeout override to ensure we don't abort too soon
	const F32 HTTP_TIMEOUT = 30.f;

	/**
	 * \brief Data parsing status
	 */
	enum eParseStatusList
	{
		UNDEFINED,
		DOWNLOAD_FAILURE,
		PARSE_FAILURE,
		DOWNLOAD_IN_PROGRESS,
		NEW_DATA,
		DOWNLOAD_OK,
		PARSING_IN_PROGRESS,
		READY,
	};

	/**
	* \brief Data download/parse status
	*/
	U8 pv_data_status_ = UNDEFINED;

	/**
	* \brief Agents download/parse status
	*/
	U8 pv_agents_status_ = UNDEFINED;

	/**
	 * \brief Check if it's safe to parse
	 * \param status_container which file to check for
	 * \return bool
	 */
	bool can_proceed(U8& status_container) const;

	/**
	 * \brief Fallback when data can't be obtained
	 */
	void handleDataFailure();
	/**
	 * \brief Fallback when data can't be obtained
	 */
	void handleAgentsFailure();

	/**
	 * \brief This handles the data received from the server after downloading the data
	 * \param http_content downloaded blob
	 * \param http_source_url where it came from
	 * \param download_failed did it fail downloading?
	 */
	static void handleResponseFromServer(const LLSD& http_content,
		const std::string& http_source_url,
		const bool& download_failed
	);

	static void downloadComplete(const LLSD& aData, std::string& aURL);

	static void downloadError(const LLSD& aData, std::string& aURL);

	/**
	* \brief This processes the data
	* \param data_input LLSD blob to parse
	*/
	void parsePVData(const LLSD& data_input);

	U32 translateFlagsToBitSet(const std::string flag);

	void addAgents(const LLSD & agent_list);

	/**
	* \brief This processes the agents data
	* \param data_input LLSD blob to parse
	*/
	void parsePVAgents(const LLSD& data_input);

	/**
	* \brief PVData main LLSD
	*/
	LLSD mPVData_llsd;

	/**
	* \brief PVData agents LLSD
	*/
	LLSD mPVAgents_llsd;

	// some color helpers
	LLColor4 Hex2Color4(const std::string &color) const;
	static LLColor4 Hex2Color4(int hexValue);

	static bool mBeggarCheckEnabled;
	/**
	 * \brief Check if current viewer is recent enough. Need to be called before showing login screen and disable login button + show error dialog if not the case.
	 * \return bool
	 */
	bool isVersionUnderMinimum();
public:
	static void setBeggarCheck(const bool enabled);
	// Get a new progress tip (throttled)
	std::string getNewProgressTip();
	static std::string getRandomWindowTitle();

	/**
	* \brief Determines if the current binary is a known, and blocked release.
	* \return true if the release is blocked, false if allowed.
	*/
	bool isBlockedRelease();

	// setters
	void setMotdEventsList(const LLSD& blob)
	{
		motd_events_list_ = blob;
	}
	void setProgressTipsList(const LLSD& blob)
	{
		progress_tips_list_ = blob;
	}
	void setWindowTitlesList(const LLSD& blob)
	{
		window_titles_list_ = blob;
	}
	void setBlockedVersionsList(const LLSD& blob);

	// This contains the re-usable LLSD data for login tips.
	// It is easier (at least for me) to parse the LLSD over and over and get a new value,
	// than storing them into a list and playing musical format conversions.
	LLSD progress_tips_list_ = LLSD::emptyMap();

	/**
	* \brief Humorous window titles during login
	*/
	LLSD window_titles_list_ = LLSD::emptyMap();

	/**
	* \brief MOTD list (Events)
	*/
	LLSD motd_events_list_ = LLSD::emptyMap();

	// This contains the viewer versions that aren't allowed to be used anymore
	pv_pair_string_llsd blocked_versions_;

public:
	// Minimum viewer version allowed to be used
	pv_pair_string_llsd minimum_version_;
};

extern PVDataOldAPI* gPVOldAPI;
//}
#endif // PV_DATA_H
