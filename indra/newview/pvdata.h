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

#ifndef PV_DATA_H
#define PV_DATA_H

#pragma once

 //
 // Dear maintainer:
 //
 // Once you are done trying to 'optimize' this routine,
 // and have realized what a terrible mistake that was,
 // please increment the following counter as a warning
 // to the next guy:
 //
 // total_hours_wasted_here = 128
 //


// Tell Compiler to leave us alone.
// boost::spirit::terminal<boost::spirit::tag::oct>::result_helper': redefinition of default parameter: parameter 3
#pragma warning (disable : 4348)

#include "boost/container/flat_set.hpp"
#include "llavatarname.h" // for convenience
#include "llerror.h" // for LOG_CLASS
#include "llsingleton.h" // for instance()
#include "pvtypes.h"

class LLColor4;
class LLUUID;

// TODO 1: Make sure this is respected
/* Note about ordering:
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
*/

	class PVData : public LLSingleton <PVData> // required for instance()
	{
		LOG_CLASS(PVData); // Required to enable the log messages prefix
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
		* \brief Developer-only message logger
		* \param log_in_s message to display/log
		* \param level severity level, defaults to debug
		*/
		static void PVData::PV_DEBUG(const std::string& log_in_s, const LLError::ELevel& level = LLError::LEVEL_DEBUG, const bool& developer_only = false);

		/**
		* \brief LLSD dumper. Does not check authentication by itself.
		* \param name Name of LLSD to display in log
		* \param map LLSD to dump
		*/
		static void Dump(const std::string name, const LLSD & map);

	private:
		/**
		 * \brief Contains the error message to display to the user if something goes wrong with PVData.
		 */
		std::string pvdata_error_message_ = "";
	};
	extern PVData* gPVData;

	class PVDataUtil : public LLSingleton<PVDataUtil>
	{
	public:
		/**
		 * \brief This returns the agent's name in the format defined by the viewer settings.
		 * \param av_name agent name
		 * \return preferred name
		 */
		static std::string getPreferredName(const LLAvatarName& av_name);

		/// <summary></summary>

		/**
		 * \brief Attempt to set the chat logs location from environment if available
		 */
		static void getChatLogsDirOverride();
		static void setChatLogsDirOverride();
		bool moveTranscriptsAndLog(std::string userid) const;
	};
	extern PVDataUtil* gPVDataUtil;

	class PVDataDownloader : public LLSingleton<PVDataDownloader>
	{
		// Public interface
	public:

		void setDataStatus(const S32& status) { pv_data_status_ = status; }
		void setAgentsDataStatus(const S32& status) { pv_agents_status_ = status; }

		/**
		 * \brief This downloads the data file
		 */
		void downloadData();

		/**
		* \brief This downloads the agents file
		*/
		void downloadAgents();

		// Cache the variables that get inserted in the HTTP headers to avoid calling the functions every time an object is created
		std::string pvdata_user_agent_;
		std::string pvdata_viewer_version_;
		LLFrameTimer pvdata_refresh_timer_;

		// Temporary blob to store the hand-crafted HTTP header
		LLSD headers_;
		
		/**
		* \brief  This downloads the requested data file from the server.
		*/
		void modularDownloader(const S8& pfile_name_in);

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

		// Enables logging for this class
		typedef PVDataDownloader _LL_CLASS_TO_LOG;
	private:

		enum pv_data_sections_index
		{
			MinimumVersion,
			BlockedReleases,
			MOTD,
			ChatMOTD,
			EventsMOTD,
			WindowTitles,
			ProgressTip,
		};
		std::vector<std::string> pv_data_sections_ =
		{
			"MinimumVersion",
			"BlockedReleases",
			"MOTD",
			"ChatMOTD",
			"EventsMOTD",
			"WindowTitles",
			"ProgressTip",
		};

		/**
		 * \brief DEPRECATED Data file name
		 */
		std::string pv_file_name_data_string_;// = "data.xml";

		/**
		 * \brief DEPRECATED Agents file name
		 */
		std::string pv_file_name_agents_string_;// = "agents.xml";

		/**
		 * \brief cached value for efficient memory writes
		 */
		bool pv_downloader_testing_branch;

		// [URL COMPONENT]
		// This is the URL where the PVData data is downloaded from, minus filename
		std::string pv_url_remote_base_string_;

		// This is the complete URL where the data file is downloaded from, with file name.
		static std::string pvdata_url_full_;

		// [URL COMPONENT]
		// This is the complete URL where the agents file is downloaded from, with file name.
		static std::string pvdata_agents_url_full_;

		// Local timeout override to ensure we don't abort too soon
		const F32 HTTP_TIMEOUT = 30.f;

		// Gross. Anyone who can re-write this, please do.

		enum pvData_files_index : S8
		{
			data_file,
			agents_file,
		};
		pv_pair_int_string pv_file_names = { { data_file, "data.xml"}, { agents_file, "agents.xml" } };

		U8 pvdata_file_version = 6;

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
		LLColor4 Hex2Color4(const std::string color) const;
		static LLColor4 Hex2Color4(int hexValue);

	};
	extern PVDataDownloader* gPVDataDownloader;

	/**
	 * \brief Contains authentication and agents data
	 */
	class PVDataAuth : public LLSingleton<PVDataAuth>
	{
		typedef PVDataAuth _LL_CLASS_TO_LOG;

		// Public Interface
	public:

		/**
		 * \brief Create generic entries for agents who should always have access should data fails to be acquired
		 */
		void setFallbackAgentsData();

		/**
		 * \brief Is the agent automatically muted on login?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserAutoMuted(const LLUUID& avatar_id);

		/**
		 * \brief Is the agent denied access to the viewer?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserBanned(const LLUUID& avatar_id);

		/**
		 * \brief Is the agent prevented from getting support?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserUnsupported(const LLUUID& avatar_id);

		/**
		 * \brief Determines if agent have more features/rights than regular users (for testing or security reasons)
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserPolarized(const LLUUID& avatar_id);

		/**
		 * \brief Is the agent a Viewer Developer?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserDevStaff(const LLUUID& avatar_id);

		// 
		/**
		 * \briefIs the agent a QA Team Member?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserQAStaff(const LLUUID& avatar_id);

		/**
		 * \brief Is the agent a Support Team Member?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserSupportStaff(const LLUUID& avatar_id);

		/**
		 * \brief Check if supplied group is one of/the vendor's support group
		 * \param id group UUID
		 * \return bool
		 */
		bool isSupportGroup(const LLUUID& id) const;
		// 
		/**
		 * \brief Is the agent a Tester?
		 * \param avatar_id agent UUID
		 * \return bool
		 */
		static bool isUserTester(const LLUUID& avatar_id);

		/**
		 * \brief Better version of isLinden that takes PVData into account
		 * TODO: Make overload without flags to parse them if called in weird places
		 * \param avatar_id agent UUID
		 * \param av_flags Pass agents flag to avoid parsing again
		 * \return bool
		 */
		bool isLinden(const LLUUID& avatar_id, S32& av_flags) const;

		// Returns the agent flags as a decimal number
		// TODO: Create versions with std::binary and such
		static S32 getSpecialAgentFlags(const LLUUID& avatar_id);

		/**
		 * \brief Returns ALL the agent's flags as a comma-separated string.
		 * \param avatar_id agent UUID
		 * \return flags as string.
		 */
		std::string getAgentFlagsAsString(const LLUUID& avatar_id);

		
		bool getSpecialAgentCustomTitle(const LLUUID& avatar_id, std::string& new_title);

		/**
		 * \brief Same as getAgentFlagsAsString, without the custom title
		 * \param avatar_id agent UUID
		 * \return flags as string
		 */
		//std::string getAgentFlagsAsStringRaw(const LLUUID& avatar_id);


		// Returns the lockdown UUID constant as a string
		static LLUUID getLockDownUUID();

		// Returns whether or not the user can use our viewer
		bool isAllowedToLogin(const LLUUID& avatar_id);

		/**
		 * \brief Agent has a color (either custom or level default)
		 * \param uuid agent UUID
		 * \return bool
		 */
		bool isSpecialAgentColored(const LLUUID& uuid);

		LLColor4 getSpecialAgentColorDirectly(const LLUUID& avatar_id);

		
		/**
		 * \brief 
		 * \param avatar_id Agent to get the color of
		 * \param default_color Color to fall back to if the agent has no color
		 * \param show_buddy_status show buddy color if applicable
		 * \return 
		 */
		static LLColor4 getSpecialAgentColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& show_buddy_status = true);
		std::string getToken();
		// setters

		/**
		 * \brief Set flag to agent
		 * \param uuid agent uuid
		 * \param flags flags
		 */
		void setSpecialAgentFlags(const LLUUID& uuid, const S32& flags);
		void setSpecialAgentTitle(const LLUUID& uuid, const std::string& title);
		void setSpecialAgentColor(const LLUUID& uuid, const LLColor4& color4);
		void setSpecialAgentBanReason(const LLUUID& uuid, const std::string& reason);
		void setVendorSupportGroup(const LLUUID& uuid);

		// NOTE: Maybe return success?
		void autoMuteFlaggedAgents();

	private:
		/*static*/ const std::string LL_LINDEN = "Linden";
		/*static*/ const std::string LL_MOLE = "Mole";
		/*static*/ const std::string LL_PRODUCTENGINE = "ProductEngine";
		/*static*/ const std::string LL_SCOUT = "Scout";
		/*static*/ const std::string LL_TESTER = "Tester";

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

		// Last updated 2016-09-26 1:23:12 PM
		enum flags_t : S32
		{
			// Those aren't numbers. They are bits and here we use them as an array of booleans.
			// Every agent flag has its own bit and you can combine them should such need arise.
			// REMINDER: Check against 0 for avatars not in the list, NOT -1
			// TODO v7: MAKE -3
			BAD_USER_BANNED = (1 << 0),      /* [0000 0000 0001] We don't want them using our stuff.        */
			// TODO v7: MAKE -1
			BAD_USER_AUTOMUTED = (1 << 1),   /* [0000 0000 0010] Automatically muted on login.              */
			// TODO v7: Make -2
			BAD_USER_UNSUPPORTED = (1 << 2),  /* [0000 0000 0100] User voided their warranty.                */
			// TODO v7: move under STAFF_QA
			STAFF_DEVELOPER = (1 << 3),        /* [0000 0000 1000] They wrote the code you're looking at.     */
			// TODO v7: move under STAFF_SUPPORT
			STAFF_QA = (1 << 4),         /* [0000 0001 0000] They approved the code you're looking at.  */
			// TODO v7: move under USER_TESTER
			STAFF_SUPPORT = (1 << 5),    /* [0000 0010 0000] They help users.                           */
			// TODO v7: move under DEPRECATED_TITLE_OVERRIDE
			USER_TESTER = (1 << 6), /* [0000 0100 0000] They kill kittens in the name of science.  */
			//FLAG_USER_HAS_TITLE = (1 << 7),   /* [0000 1000 0000] User that deserves recognition             */
			// TODO v7: DEPRECATE, empty title falls back to level flag
			DEPRECATED_TITLE_OVERRIDE = (1 << 8),   /* [0001 0000 0000] Title overrides general flags list         */
			//FLAG_USER_HAS_COLOR = (1 << 9),   /* DEPRECATED [0010 0000 0000] User has a custom color         */

			// Last.
			LINDEN_EMPLOYEE = (1 << 15), /* [1000 0000 0000 0000] Linden Lab Employee */
		};

		// This contains the UUID of our support group
		std::set<LLUUID> support_group_;
	};
	extern PVDataAuth* gPVDataAuth;

	/**
	 * \brief Contains information about the viewer itself.
	 */
	class PVDataViewerInfo : public LLSingleton<PVDataViewerInfo>
	{
	public:
		// getters

		// ew, string.
#if 0 // v7
		bool isVersionBlocked(const std::string version) const
		{
			return blocked_versions_.has(version);
		}
#endif
		std::string getEventMotdIfAny()
		{
			for (LLSD::map_const_iterator iter = motd_events_list_.beginMap(); iter != motd_events_list_.endMap(); ++iter)
			{
				auto name = iter->first;
				auto content = iter->second;
				//PV_DEBUG("Found event MOTD: " + name, LLError::LEVEL_DEBUG);

				if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
				{
					PVData::PV_DEBUG("Setting MOTD to " + name, LLError::LEVEL_DEBUG);
					PVData::PV_DEBUG("Setting MOTD to " + name, LLError::LEVEL_DEBUG);
					// TODO: Shove into notification well.
					return name;
				}
			}
			return "";
		}
#if 0 // v7
		std::string getMinimumVersion();
#endif
		/**
		 * \brief Check if current viewer is recent enough. Need to be called before showing login screen and disable login button + show error dialog if not the case.
		 * \return bool
		 */
		bool isVersionAtOrAboveMinimum();

		// Force getting a new progress tip, regardless of the timer
		std::string getNewProgressTipForced();

		// Get a new progress tip (throttled)
		std::string getNewProgressTip(const std::string msg_in);

		static std::string getRandomWindowTitle();


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
		void setBlockedVersionsList(const LLSD& blob)
		{
			//blocked_versions_ = blob; // v7?
			auto blocked = blob["BlockedReleases"];
			for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
			{
				auto version = iter->first;
				auto reason = iter->second;
				blocked_versions_[version] = reason;
				PVData::PV_DEBUG("Added " + version + " to blocked_versions_ with reason '" + reason.asString() + "'", LLError::LEVEL_DEBUG);
			}
		}
		void setMinimumVersion(const LLSD& blob)
		{
			//minimum_version_ = blob; // v7?
			auto min_version = blob["MinimumVersion"];
			for (LLSD::map_const_iterator iter = min_version.beginMap(); iter != min_version.endMap(); ++iter)
			{
				auto version = iter->first;
				auto reason = iter->second;
				minimum_version_[version] = reason;
				LL_DEBUGS("PVData") << "Minimum Version is " << version << LL_ENDL;
			}
		}

	private:

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
		//LLSD blocked_versions_; // v7?
		pv_pair_string_llsd blocked_versions_;

		// Minimum viewer version allowed to be used
		//LLSD minimum_version_; // v7?
		pv_pair_string_llsd minimum_version_;

		LLFrameTimer mTipCycleTimer;

		std::string last_login_tip;
	};
	extern PVDataViewerInfo* gPVDataViewerInfo;

// TODO: Move to another file?
class PVSearchUtil : public LLSingleton <PVSearchUtil>
{
	LOG_CLASS(PVSearchUtil);

public:
	// refresh from settings
	static U32 getSearchSeparatorFromSettings();
	static void setSearchSeparator(const U32 separator_in_u32);
	// get separator
	static std::string getSearchSeparator();
	std::string getSearchSeparator(const U32 separator_to_get_u32) const;

private:
	enum PVSearchSeparators : U32
	{
		separator_space,
		separator_plus,
		separator_comma,
		separator_pipe,
		separator_semicolon,
		separator_period,
		separator_colon,
	};

	/**
	 * \brief Contains the possible search separators
	 * TODO: Re-write as a map.
	 */
	const std::vector<std::string> PVSearchSeparatorAssociation
	{
		" ",
		"+",
		",",
		"|",
		";",
		".",
		":",
	};

	/**
	* \brief Currently selected search separator
	*/
	static U32 PVSearchSeparatorSelected;
};
	extern PVSearchUtil* gPVSearchUtil;

#endif // PV_DATA_H
