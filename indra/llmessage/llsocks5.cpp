/**
 * @file llsocks5.cpp
 * @brief Socks 5 implementation
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include <string>

#include "linden_common.h"
#include "net.h"
#include "llhost.h"
#include "message.h"
#include "llsocks5.h"

// Static class variable instances

// We want this to be static to avoid excessive indirection on every
// incoming packet just to do a simple bool test. The getter for this
// member is also static
bool LLSocks::sUdpProxyEnabled;
bool LLSocks::sHttpProxyEnabled;

LLSocks::LLSocks()
{
	sUdpProxyEnabled  = false;
	sHttpProxyEnabled = false;
	mProxyControlChannel = 0;
	mProxyType = LLPROXY_SOCKS;
}

// Perform a Socks5 authentication and UDP association to the proxy
// specified by proxy, and associate UDP port message_port
int LLSocks::proxyHandshake(LLHost proxy, U32 message_port)
{
	int result;

	/* Socks 5 Auth request */
	socks_auth_request_t  socks_auth_request;
	socks_auth_response_t socks_auth_response;

	socks_auth_request.version     = SOCKS_VERSION;       // Socks version 5
	socks_auth_request.num_methods = 1;                   // Sending 1 method
	socks_auth_request.methods     = mAuthMethodSelected; // send only the selected method

	result = tcp_handshake(mProxyControlChannel, (char*)&socks_auth_request, sizeof(socks_auth_request_t), (char*)&socks_auth_response, sizeof(socks_auth_response_t));
	if (result != 0)
	{
		llwarns << "Socks authentication request failed, error on TCP control channel : " << result << llendl;
		stopProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (socks_auth_response.method == AUTH_NOT_ACCEPTABLE)
	{
		llwarns << "Socks5 server refused all our authentication methods" << llendl;
		stopProxy();
		return SOCKS_NOT_ACCEPTABLE;
	}

	// SOCKS5 USERNAME/PASSWORD authentication
	if (socks_auth_response.method == METHOD_PASSWORD)
	{
		// The server has requested a username/password combination
		U32 request_size = mSocksUsername.size() + mSocksPassword.size() + 3;
		// char * password_auth = (char *)malloc(request_size);
		char * password_auth = new char[request_size];
		password_auth[0] = 0x01;
		password_auth[1] = mSocksUsername.size();
		memcpy(&password_auth[2], mSocksUsername.c_str(), mSocksUsername.size());
		password_auth[mSocksUsername.size()+2] = mSocksPassword.size();
		memcpy(&password_auth[mSocksUsername.size()+3], mSocksPassword.c_str(), mSocksPassword.size());

		authmethod_password_reply_t password_reply;

		result = tcp_handshake(mProxyControlChannel, password_auth, request_size, (char*)&password_reply, sizeof(authmethod_password_reply_t));
		delete[] password_auth;

		if (result != 0)
		{
			llwarns << "Socks authentication failed, error on TCP control channel : " << result << llendl;
			stopProxy();
			return SOCKS_CONNECT_ERROR;
		}

		if (password_reply.status != AUTH_SUCCESS)
		{
			llwarns << "Socks authentication failed" << llendl;
			stopProxy();
			return SOCKS_AUTH_FAIL;
		}
	}

	/* SOCKS5 connect request */

	socks_command_request_t  connect_request;
	socks_command_response_t connect_reply;

	connect_request.version = SOCKS_VERSION;         //Socks V5
	connect_request.command = COMMAND_UDP_ASSOCIATE; // Associate UDP
	connect_request.flag    = FIELD_RESERVED;
	connect_request.atype   = ADDRESS_IPV4;
	connect_request.address = 0; // 0.0.0.0 We are not fussy about address
	// UDP is promiscuous receive for our protocol
	connect_request.port    = 0; // Port must be 0 if you ever want to connect via NAT and your router does port rewrite for you

	result = tcp_handshake(mProxyControlChannel, (char*)&connect_request, sizeof(socks_command_request_t), (char*)&connect_reply, sizeof(socks_command_response_t));
	if (result != 0)
	{
		llwarns << "Socks connect request failed, error on TCP control channel : " << result << llendl;
		stopProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (connect_reply.reply != REPLY_REQUEST_GRANTED)
	{
		//Something went wrong
		llwarns << "Connection to SOCKS5 server failed, UDP forward request not granted" << llendl;
		stopProxy();
		return SOCKS_UDP_FWD_NOT_GRANTED;
	}

	mUDPProxy.setPort(ntohs(connect_reply.port)); // reply port is in network byte order
	mUDPProxy.setAddress(proxy.getAddress());
	// All good now we have been given the UDP port to send requests that need forwarding.
	llinfos << "Socks 5 UDP proxy connected on " << mUDPProxy << llendl;
	return SOCKS_OK;
}

int LLSocks::startProxy(LLHost proxy, U32 message_port)
{
	int status;

	mTCPProxy = proxy;

	if (mProxyControlChannel)
	{
		tcp_close_channel(mProxyControlChannel);
		mProxyControlChannel = 0;
	}

	mProxyControlChannel = tcp_open_channel(proxy);
	if (mProxyControlChannel == -1)
	{
		return SOCKS_HOST_CONNECT_FAILED;
	}

	status = proxyHandshake(proxy, message_port);
	if (status == SOCKS_OK)
	{
		sUdpProxyEnabled = true;
	}
	return status;
}

int LLSocks::startProxy(std::string host, U32 port)
{
	mTCPProxy.setHostByName(host);
	mTCPProxy.setPort(port);
	return startProxy(mTCPProxy, (U32)gMessageSystem->mPort);
}

void LLSocks::stopProxy()
{
	sUdpProxyEnabled = false;

	// If the Socks proxy is requested to stop and we are using that for http as well
	// then we must shut down any http proxy operations. But it is allowable if web
	// proxy is being used to continue proxying http.

	if(LLPROXY_SOCKS == mProxyType)
	{
		sHttpProxyEnabled = false;
	}

	if (mProxyControlChannel)
	{
		tcp_close_channel(mProxyControlChannel);
		mProxyControlChannel = 0;
	}
}

void LLSocks::setAuthNone()
{
	mAuthMethodSelected = METHOD_NOAUTH;
}

void LLSocks::setAuthPassword(std::string username, std::string password)
{
	mAuthMethodSelected = METHOD_PASSWORD;
	mSocksUsername      = username;
	mSocksPassword      = password;
}

void LLSocks::enableHttpProxy(LLHost httpHost, LLHttpProxyType type)
{ 
	sHttpProxyEnabled = true;
	mHTTPProxy        = httpHost;
	mProxyType        = type;
}
