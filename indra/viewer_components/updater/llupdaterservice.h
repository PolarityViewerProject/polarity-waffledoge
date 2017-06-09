/** 
 * @file llupdaterservice.h
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_UPDATERSERVICE_H
#define LL_UPDATERSERVICE_H

#include <boost/function.hpp>
#include "llmd5.h"
#include "llhasheduniqueid.h"
#include "llexception.h"

class LLUpdaterServiceImpl;

class LLUpdaterService
{
public:
	class UsageError: public LLException
	{
	public:
		UsageError(const std::string& msg) : LLException(msg) {}
	};
	
	// Name of the event pump through which update events will be delivered.
	static std::string const & pumpName(void);
	
	// Returns true if an update has been completely downloaded and is now ready to install.
	static bool updateReadyToInstall(void);
	
	// Type codes for events posted by this service.  Stored the event's 'type' element.
	enum eUpdaterEvent {
		INVALID,
		DOWNLOAD_COMPLETE,
		DOWNLOAD_ERROR,
		INSTALL_ERROR,
		PROGRESS,
		STATE_CHANGE
	};
	
	enum eUpdaterState {
		INITIAL,
		CHECKING_FOR_UPDATE,
		TEMPORARY_ERROR,
		DOWNLOADING,
		INSTALLING,
		UP_TO_DATE,
		TERMINAL,
		FAILURE
	};

	LLUpdaterService();
	~LLUpdaterService();

	void initialize(const std::string & channel,
				    const std::string & version,
					const std::string &  platform,
					const std::string &  platform_version,
					const bool&         willing_to_test,
					const unsigned char       uniqueid[MD5HEX_STR_SIZE],
					const std::string & auth_token = ""
					);

	void setCheckPeriod(unsigned int seconds) const;
	void setBandwidthLimit(U64 bytesPerSecond) const;
	
	void startChecking(bool install_if_ready = false) const;
	void stopChecking() const;
	bool forceCheck(const bool is_willing_to_test, const std::string auth_token_in = "") const;
	bool isChecking() const;
	eUpdaterState getState() const;

	typedef boost::function<void (void)> app_exit_callback_t;
	template <typename F>
	void setAppExitCallback(F const &callable) 
	{ 
		app_exit_callback_t aecb = callable;
		setImplAppExitCallback(aecb);
	}
	
	// If an update is or has been downloaded, this method will return the
	// version string for that update.  An empty string will be returned
	// otherwise.
	std::string updatedVersion(void) const;

private:
	std::shared_ptr<LLUpdaterServiceImpl> mImpl;
	void setImplAppExitCallback(app_exit_callback_t aecb) const;
};

// Returns the full version as a string.
std::string const & ll_get_version(void);

#endif // LL_UPDATERSERVICE_H
