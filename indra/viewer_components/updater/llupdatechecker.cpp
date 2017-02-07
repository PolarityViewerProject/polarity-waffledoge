// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llupdaterservice.cpp
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

#include "linden_common.h"
#include <stdexcept>
#include <boost/format.hpp>
#include "llsd.h"
#include "llupdatechecker.h"
#include "lluri.h"
#include "llcorehttputil.h"
#if LL_DARWIN
#include <CoreServices/CoreServices.h>
#endif

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif


class LLUpdateChecker::CheckError:
	public std::runtime_error
{
public:
	CheckError(const char * message):
		runtime_error(message)
	{
		; // No op.
	}
};


// LLUpdateChecker
//-----------------------------------------------------------------------------
LLUpdateChecker::LLUpdateChecker(Client & client):
	mImplementation(new Implementation(client))
{
	; // No op.
}

void LLUpdateChecker::checkVersion(const std::string & urlBase, 
								   const std::string & channel,
								   const std::string & version,
								   const std::string & platform,
								   const std::string & platform_version,
								   const bool&         willing_to_test,
								   const unsigned char       uniqueid[MD5HEX_STR_SIZE],
								   const std::string & auth_token

) const
{
	mImplementation->checkVersion(urlBase,
								  channel,
								  version,
								  platform,
								  platform_version,
								  willing_to_test,
								  uniqueid,
								  auth_token);
}


// LLUpdateChecker::Implementation
//-----------------------------------------------------------------------------
const char * LLUpdateChecker::Implementation::sProtocolVersion = "v1.1";


LLUpdateChecker::Implementation::Implementation(Client & client):
	mProtocol(sProtocolVersion),
	mClient(client),
	mInProgress(false),
	mWillingToTest(false)
{
	; // No op.
}


LLUpdateChecker::Implementation::~Implementation()
{
	; // No op.
}


void LLUpdateChecker::Implementation::checkVersion(const std::string & urlBase, 
												   const std::string & channel,
												   const std::string & version,
												   const std::string & platform,
												   const std::string & platform_version,
												   const bool &        willing_to_test,
												   const unsigned char       uniqueid[MD5HEX_STR_SIZE],
												   const std::string & auth_token
												  )
{
	if (!mInProgress)
	{
		mInProgress = true;

		mUrlBase     	 = urlBase;
		mChannel     	 = channel;
		mVersion     	 = version;
		mPlatform        = platform;
		mPlatformVersion = platform_version;
		memcpy(mUniqueId, uniqueid, MD5HEX_STR_SIZE);
		mWillingToTest   = willing_to_test;
		mAuthToken       = auth_token;
	
		mProtocol = sProtocolVersion;

		std::string checkUrl = buildUrl(urlBase,
										channel,
										version,
										platform,
										platform_version,
										willing_to_test,
										uniqueid,
										auth_token
										);
		LL_INFOS("UpdaterService") << "checking for updates at " << checkUrl << LL_ENDL;

        LLCoros::instance().launch("LLUpdateChecker::Implementation::checkVersionCoro",
            boost::bind(&Implementation::checkVersionCoro, this, checkUrl));

	}
	else
	{
		LL_WARNS("UpdaterService") << "attempting to restart a check when one is in progress; ignored" << LL_ENDL;
	}
}

void LLUpdateChecker::Implementation::checkVersionCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("checkVersionCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LL_INFOS("checkVersionCoro") << "Getting update information from " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    mInProgress = false;

    if (status != LLCore::HttpStatus(HTTP_OK))
    {
        std::string server_error;
        if (result.has("error_code"))
        {
            server_error += result["error_code"].asString();
        }
        if (result.has("error_text"))
        {
            server_error += server_error.empty() ? "" : ": ";
            server_error += result["error_text"].asString();
        }

        LL_WARNS("UpdaterService") << "response error " << status.getStatus()
            << " " << status.toString()
            << " (" << server_error << ")"
            << LL_ENDL;
        mClient.error(status.toString());
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    mClient.response(result);
}

std::string LLUpdateChecker::Implementation::buildUrl(const std::string & urlBase, 
													  const std::string & channel,
													  const std::string & version,
													  const std::string & platform,
													  const std::string & platform_version,
													  const bool &        willing_to_test,
													  const unsigned char       uniqueid[MD5HEX_STR_SIZE],
													  const std::string & auth_token
													  )
{
	LLSD path;
	path.append(mProtocol);
	path.append(channel);
	path.append(version);
	path.append(platform);
	path.append(platform_version);
	path.append(willing_to_test ? "testok" : "testno");
	path.append((char*)uniqueid);
	// terrible, terrible hack because buildHTTP is escaping our parameters
	auto partial_url = LLURI::buildHTTP(urlBase, path).asString();
	if (auth_token != "")
	{
		partial_url += "/?token=" + auth_token;
	}
	return partial_url;
}
