/** 
 * @file llupdatechecker.h
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

#ifndef LL_UPDATERCHECKER_H
#define LL_UPDATERCHECKER_H

#include "llmd5.h"
#include "lleventcoro.h"
#include "llcoros.h"

//
// Implements asynchronous checking for updates.
//
class LLUpdateChecker {
public:
    //
    // The client interface implemented by a requestor checking for an update.
    //
    class Client
    {
    public:
	    virtual ~Client()
	    {
	    }

	    // An error occurred while checking for an update.
        virtual void error(std::string const & message) = 0;

        // A successful response was received from the viewer version manager
        virtual void response(LLSD const & content) = 0;
    };

	// An exception that may be raised on check errors.
	class CheckError;
	
	LLUpdateChecker(Client & client);
	
	// Check status of current app on the given host for the channel and version provided.
	void checkVersion(const std::string & urlBase, 
					  const std::string & channel,
					  const std::string & version,
					  const std::string & platform,
					  const std::string & platform_version,
					  const bool&         willing_to_test,
					  const unsigned char       uniqueid[MD5HEX_STR_SIZE],
					  const std::string & auth_token
					) const;
	
private:
    class Implementation
    {
    public:
        typedef boost::shared_ptr<Implementation> ptr_t;

        Implementation(Client & client);
        ~Implementation();
        void checkVersion(std::string const & urlBase,
            const std::string & channel,
            const std::string & version,
            const std::string & platform,
            const std::string & platform_version,
            const bool &        willing_to_test,
			const unsigned char       uniqueid[MD5HEX_STR_SIZE],
            const std::string & auth_token
            );


    private:
        static const char * sLegacyProtocolVersion;
        static const char * sProtocolVersion;
        const char* mProtocol;

        Client & mClient;
        bool         mInProgress;
        std::string   mVersion;
        std::string   mUrlBase;
        std::string   mChannel;
        std::string   mPlatform;
        std::string   mPlatformVersion;
        unsigned char mUniqueId[MD5HEX_STR_SIZE];
        bool          mWillingToTest;
		std::string   mAuthToken;

        std::string buildUrl(std::string const & urlBase,
            const std::string & channel,
            const std::string & version,
            const std::string & platform,
            const std::string & platform_version,
            const bool &        willing_to_test,
            const unsigned char uniqueid[MD5HEX_STR_SIZE],
            const std::string & auth_token
            );

        void checkVersionCoro(std::string url);

        LOG_CLASS(LLUpdateChecker::Implementation);
    };


    Implementation::ptr_t       mImplementation;
	//LLPointer<Implementation> mImplementation;
};

#endif
