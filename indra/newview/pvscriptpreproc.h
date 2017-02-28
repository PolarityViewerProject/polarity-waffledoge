/**
 * @file pvscriptpreproc.h
 * @brief Re-implementation of Modular System's LSL Preprocessor
 * No copyright infringement intended
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2017 Xenhat Liamano
 * Portions Copyright (C)
 *  2010 Modular Systems
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

#ifndef FS_LSLPREPROC_H
#define FS_LSLPREPROC_H

#include "llviewerprecompiledheaders.h"
#include "llpreviewscript.h"

#define DARWINPREPROC
//force preproc on mac

struct LLScriptQueueData;

class FSLSLPreprocessor
{
	LOG_CLASS(FSLSLPreprocessor);
public:

	FSLSLPreprocessor(LLScriptEdCore* corep)
		: mDefinitionCaching(false),
		  mCore(corep),
		  mWaving(false),
		  mClose(FALSE),
		  mSync(false),
		  mStandalone(false),
		  mData(nullptr),
		  mType()
	{
	}
	
	FSLSLPreprocessor()
		: mDefinitionCaching(false),
		  mCore(nullptr),
		  mWaving(false),
		  mClose(FALSE),
		  mSync(false),
		  mStandalone(true),
		  mData(nullptr),
		  mType()
	{
	}

	static bool mono_directive(std::string const& text, bool agent_inv = true);
	std::string encode(const std::string& script);
	std::string decode(const std::string& script);

	std::string lslopt(std::string script);
	std::string lslcomp(std::string script);

	static LLUUID findInventoryByName(std::string name);
	static void FSProcCacheCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type,
									void *userdata, S32 result, LLExtStat extstat);
	void preprocess_script(BOOL close = FALSE, bool sync = false, bool defcache = false);
	void preprocess_script(const LLUUID& asset_id, LLScriptQueueData* data, LLAssetType::EType type, const std::string& script_data);
	void start_process();
	void display_message(const std::string& err);
	void display_error(const std::string& err);

	std::string uncollide_string_literals(std::string script);

	//dual function, determines if files have been modified this session and if we have cached them
	//also assetids exposed in-preprocessing as a predefined macro for use in include once style include files, e.g. #define THISFILE file_ ## __ASSETIDRAW__
	//in case it isn't obvious, the viewer only sets the asset id on a successful script save (of a full perm script), or in preproc on-cache
	//so this is only applicable to fully permissive scripts; which is just fine, since if it isn't full perm it isn't really useful as a include anyway.
	//in the event of a no-trans script (only less than full thats readable), the server sends null key, and we will set a random uuid. 
	//This uuid should be overwritten if they edit that script, whether with the real id or null key is irrelevant in this case.
	//theoretically, if the asset IDs were exposed for full perm scripts without downloading the script at least once, it would save unnecessary caching
	//as this isn't the case I'm not going to preserve this structure across logins.

	//(it seems rather dumb that readable scripts don't show the asset id without a DL, but thats beside the point.)
	static std::map<std::string, LLUUID> cached_assetids;

	static std::map<std::string, std::string> decollided_literals;

	std::set<std::string> caching_files;
	std::set<std::string> defcached_files;
	bool mDefinitionCaching;

	LLScriptEdCore* mCore;
	bool mWaving;
	BOOL mClose;
	bool mSync;
	std::string mMainScriptName;
	
	// Compile queue
	bool mStandalone;
	std::string mScript;
	LLUUID mAssetID;
	LLScriptQueueData* mData;
	LLAssetType::EType mType;
};

#endif // FS_LSLPREPROC_H
