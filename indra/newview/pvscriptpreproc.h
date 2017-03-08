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

#pragma once
#ifndef FS_LSLPREPROC_H
#define FS_LSLPREPROC_H

#include "llviewerprecompiledheaders.h"
#include "llpreviewscript.h"
#include "llinventoryfunctions.h"

////////////////////////////////////////////////////////////////////////////////////////
/// Boost include shenanigans
//apparently LL #defined this function which happens to precisely match
//a boost::wave function name, destroying the internet, silly grey furries
#ifdef equivalent
#undef equivalent
#endif
#ifdef __GNUC__
// There is a sprintf( ... "%d", size_t_value) buried inside boost::wave.
// In order to not mess with system header, I rather disable that warning here.
#pragma GCC diagnostic ignored "-Wformat"
#elif defined(_MSC_VER)
#pragma warning (disable: 4477) // MSVC14 needs this as well.
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif
#include <boost/assert.hpp>
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class
#include <boost/wave/preprocessing_hooks.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

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

	//std::string uncollide_string_literals(std::string script);

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

private:

	class ScriptMatches : public LLInventoryCollectFunctor
	{
	public:
		ScriptMatches(const std::string& name)
		{
			mName = name;
		}
		virtual ~ScriptMatches()
		{
		}
		virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
		{
			return (item && item->getName() == mName && item->getType() == LLAssetType::AT_LSL_TEXT);
		}
	private:
		std::string mName;
	};
	class trace_include_files : public boost::wave::context_policies::default_preprocessing_hooks
	{
	public:
		explicit trace_include_files(FSLSLPreprocessor* proc) : mProc(proc)
		{
			mAssetStack.push(LLUUID::null.asString());
			mFileStack.push(proc->mMainScriptName);
		}
		template <typename ContextT>
		bool found_include_directive(ContextT const& ctx, std::string const &filename, bool include_next);
		template <typename ContextT>
		void opened_include_file(ContextT const& ctx, std::string const &relname, std::string const& absname, bool is_system_include);
		template <typename ContextT>
		void returning_from_include_file(ContextT const& ctx);
		template <typename ContextT, typename ExceptionT>
		void throw_exception(ContextT const& ctx, ExceptionT const& e);
	private:
		FSLSLPreprocessor* mProc;
		std::stack<std::string> mAssetStack;
		std::stack<std::string> mFileStack;
	};
	static inline std::string shortfile(std::string in);
	struct ProcCacheInfo
	{
		LLViewerInventoryItem* item;
		FSLSLPreprocessor* self;
	};

	static void cache_script(std::string name, std::string content);

	static std::string scopeript2(std::string& top, S32 fstart, char left = '{', char right = '}');
	static inline S32 const_iterator_to_pos(std::string::const_iterator begin, std::string::const_iterator cursor)
	{
		return std::distance(begin, cursor);
	}

	static void shredder(std::string& text);

	static void subst_lazy_references(std::string& script, std::string retype, std::string fn);
	static std::string reformat_lazy_lists(std::string script);
	static inline std::string randstr(S32 len, std::string chars);
	static inline std::string quicklabel();
	/* unused:
	static std::string minimalize_whitespace(std::string in)
	{
	return boost::regex_replace(in, boost::regex("\\s*",boost::regex::perl), "\n");
	}
	*/
	static std::string reformat_switch_statements(std::string script);
};

#endif // FS_LSLPREPROC_H
