// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llcompilequeue.cpp
 * @brief LLCompileQueueData class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/**
 *
 * Implementation of the script queue which keeps an array of object
 * UUIDs and manipulates all of the scripts on each of them. 
 *
 */


#include "llviewerprecompiledheaders.h"

#include "llcompilequeue.h"

#include "llagent.h"
#include "llchat.h"
#include "llfloaterreg.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"

#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "lldir.h"
#include "llnotificationsutil.h"
#include "llviewerstats.h"
#include "llvfile.h"
#include "lltrans.h"

#include "llselectmgr.h"
#include "llexperiencecache.h"

#include "llviewerassetupload.h"
#include "llcorehttputil.h"
#ifdef LSL_PREPROCESSOR
#include "llsdutil.h" // for LLSDMap()
// <polarity> LSL PreProc
#include "pvscriptpreproc.h"
// </polarity>
#endif

namespace
{

    const std::string QUEUE_EVENTPUMP_NAME("ScriptActionQueue");

    // ObjectIventoryFetcher is an adapter between the LLVOInventoryListener::inventoryChanged 
    // callback mechanism and the LLEventPump coroutine architecture allowing the 
    // coroutine to wait for the inventory event.
    class ObjectInventoryFetcher: public LLVOInventoryListener
    {
    public:
        typedef boost::shared_ptr<ObjectInventoryFetcher> ptr_t;

        ObjectInventoryFetcher(LLEventPump &pump, LLViewerObject* object, void* user_data) :
            LLVOInventoryListener(),
            mPump(pump)
        {
            registerVOInventoryListener(object, this);
        }

	    void inventoryChanged(LLViewerObject* object,
            LLInventoryObject::object_list_t* inventory,
            S32 serial_num,
            void* user_data) override;

        void fetchInventory() 
        {
            requestVOInventory();
        }

        const LLInventoryObject::object_list_t & getInventoryList() const { return mInventoryList; }

    private:
        LLInventoryObject::object_list_t    mInventoryList;
        LLEventPump &                       mPump;
    };

    class HandleScriptUserData
    {
    public:

	HandleScriptUserData(const std::string &pumpname
#ifdef LSL_PREPROCESSOR
			// <FS:Ansariel> [LSL PreProc]
			, LLScriptQueueData* data
			// </FS:Ansariel>
#endif
        	) : mPumpname(pumpname)
#ifdef LSL_PREPROCESSOR
			,mData(data) // <FS:Ansariel/>
#endif
        { }

        const std::string &getPumpName() const { return mPumpname; }
#ifdef LSL_PREPROCESSOR
        // <FS:Ansariel> [LSL PreProc]
        LLScriptQueueData* getData() const { return mData; }
#endif
    private:
        std::string mPumpname;
#ifdef LSL_PREPROCESSOR
        // <FS:Ansariel> [LSL PreProc]
        LLScriptQueueData* mData;
#endif
    };


}

// *NOTE$: A minor specialization of LLScriptAssetUpload, it does not require a buffer 
// (and does not save a buffer to the vFS) and it finds the compile queue window and 
// displays a compiling message.
class LLQueuedScriptAssetUpload : public LLScriptAssetUpload
{
public:
    LLQueuedScriptAssetUpload(LLUUID taskId, LLUUID itemId, LLUUID assetId, TargetType_t targetType,
            bool isRunning, std::string scriptName, LLUUID queueId, LLUUID exerienceId, taskUploadFinish_f finish) :
        LLScriptAssetUpload(taskId, itemId, targetType, isRunning, 
                            exerienceId, std::string(), finish),
        mQueueId(queueId),
        mScriptName(scriptName)
    {
        setAssetId(assetId);
    }

	LLSD prepareUpload() override
    {
        /* *NOTE$: The parent class (LLScriptAssetUpload will attempt to save 
         * the script buffer into to the VFS.  Since the resource is already in 
         * the VFS we don't want to do that.  Just put a compiling message in
         * the window and move on
         */
        LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(mQueueId));
        if (queue)
        {
            // <FS:Ansariel> Translation fixes
            //std::string message = std::string("Compiling \"") + getScriptName() + std::string("\"...");
            LLStringUtil::format_map_t args;
            args["OBJECT_NAME"] = getScriptName();
            std::string message = queue->getString("Compiling", args);
            // </FS:Ansariel>

            queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
        }

        return LLSDMap("success", LLSD::Boolean(true));
    }


    std::string getScriptName() const { return mScriptName; }

private:
    void setScriptName(const std::string &scriptName) { mScriptName = scriptName; }

    LLUUID mQueueId;
    std::string mScriptName;
};

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

// <FS:KC> [LSL PreProc] moved to header
//struct LLScriptQueueData
//{
//	LLUUID mQueueID;
//	LLUUID mTaskId;
//	LLPointer<LLInventoryItem> mItem;
//	LLHost mHost;
//	LLUUID mExperienceId;
//	std::string mExperiencename;
//	LLScriptQueueData(const LLUUID& q_id, const LLUUID& task_id, LLInventoryItem* item) :
//		mQueueID(q_id), mTaskId(task_id), mItem(new LLInventoryItem(item)) {}
//
//};
// </FS:KC>

///----------------------------------------------------------------------------
/// Class LLFloaterScriptQueue
///----------------------------------------------------------------------------

// Default constructor
LLFloaterScriptQueue::LLFloaterScriptQueue(const LLSD& key) :
	LLFloater(key),
	mMessages(nullptr),
	mCloseBtn(nullptr),
	mDone(false),
	mMono(false)
{
	
}

// Destroys the object
LLFloaterScriptQueue::~LLFloaterScriptQueue()
{
}

BOOL LLFloaterScriptQueue::postBuild()
{
	childSetAction("close",onCloseBtn,this);
	getChildView("close")->setEnabled(FALSE);
	setVisible(true);
	return TRUE;
}

//#ifdef LSL_PREPROCESSOR
//nonstatic
void LLFloaterScriptQueue::Close()
{
	this->closeFloater();
}
//#endif

// static
void LLFloaterScriptQueue::onCloseBtn(void* user_data)
{
	LLFloaterScriptQueue* self = (LLFloaterScriptQueue*)user_data;
	self->closeFloater();
}

void LLFloaterScriptQueue::addObject(const LLUUID& id, std::string name)
{
    ObjectData obj = { id, name };
    mObjectList.push_back(obj);
}

BOOL LLFloaterScriptQueue::start()
{
#ifdef LSL_PREPROCESSOR

	LLNotificationsUtil::add("ConfirmScriptModify", LLSD(), LLSD(), boost::bind(&LLFloaterScriptQueue::onScriptModifyConfirmation, this, _1, _2));
	return true;
	/*
	//LL_INFOS() << "LLFloaterCompileQueue::start()" << LL_ENDL;
	std::string buffer;

	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectList.size());
	buffer = getString ("Starting", args);
	
	getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer);

	return startQueue();*/
}

bool LLFloaterScriptQueue::onScriptModifyConfirmation(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)// canceled
	{
		return true;
	}
#endif
	std::string buffer;

	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectList.size());
	buffer = getString ("Starting", args);

#ifdef LSL_PREPROCESSOR
	// <FS:Ansariel> Improve log output
	//getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
	addStringMessage(buffer);
	// </FS:Ansariel>

	LLButton* butt = getChild<LLButton>("close");
	butt->setLabel(LLTrans::getString("Cancel"));
#endif
	return startQueue();
}

void LLFloaterScriptQueue::addProcessingMessage(const std::string &message, const LLSD &args)
{
    std::string buffer(LLTrans::getString(message, args));

#ifdef LSL_PREPROCESSOR
    // <FS:Ansariel> Improve log output
    //getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
    addStringMessage(buffer);
    // </FS:Ansariel>
#else
    getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
#endif
}

void LLFloaterScriptQueue::addStringMessage(const std::string &message)
{
#ifdef LSL_PREPROCESSOR
    // <FS:Ansariel> Improve log output
    //getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
    LLScrollListCtrl* ctrl = getChild<LLScrollListCtrl>("queue output");
    BOOL is_at_end = ctrl->getScrollbar()->isAtEnd();
    ctrl->addSimpleElement(message, ADD_BOTTOM);
    if (is_at_end)
    {
        ctrl->setScrollPos(ctrl->getScrollPos() + 1);
    }
    // </FS:Ansariel>
#else
    getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
#endif
}


BOOL LLFloaterScriptQueue::isDone() const
{
	return (mCurrentObjectID.isNull() && (mObjectList.size() == 0));
}

///----------------------------------------------------------------------------
/// Class LLFloaterCompileQueue
///----------------------------------------------------------------------------
LLFloaterCompileQueue::LLFloaterCompileQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
#ifdef LSL_PREPROCESSOR
  , mLSLProc(NULL) // <FS:Ansariel> [LSL PreProc]
#endif
{
	setTitle(LLTrans::getString("CompileQueueTitle"));
	setStartString(LLTrans::getString("CompileQueueStart"));
														 															 
#ifdef LSL_PREPROCESSOR
	// <FS:KC> [LSL PreProc]
	static LLCachedControl<bool> _NACL_LSLPreprocessor(gSavedSettings, "_NACL_LSLPreprocessor");
	if(_NACL_LSLPreprocessor)
	{
		mLSLProc = new FSLSLPreprocessor();
	}
	// </FS:KC>
#endif
}

LLFloaterCompileQueue::~LLFloaterCompileQueue()
{ 
#ifdef LSL_PREPROCESSOR
	// <FS:Ansariel> [LSL PreProc]
	mLSLProc = nullptr;
	delete mLSLProc;
#endif
}

void LLFloaterCompileQueue::experienceIdsReceived( const LLSD& content )
{
	for(LLSD::array_const_iterator it  = content.beginArray(); it != content.endArray(); ++it)
	{
		mExperienceIds.insert(it->asUUID());
	}
}

BOOL LLFloaterCompileQueue::hasExperience( const LLUUID& id ) const
{
	return mExperienceIds.find(id) != mExperienceIds.end();
}

// //Attempt to record this asset ID.  If it can not be inserted into the set 
// //then it has already been processed so return false.

void LLFloaterCompileQueue::handleHTTPResponse(std::string pumpName, const LLSD &expresult)
{
    LLEventPumps::instance().post(pumpName, expresult);
}

// *TODO: handleSCriptRetrieval is passed into the VFS via a legacy C function pointer
// future project would be to convert these to C++ callables (std::function<>) so that 
// we can use bind and remove the userData parameter.
// 
void LLFloaterCompileQueue::handleScriptRetrieval(LLVFS *vfs, const LLUUID& assetId, 
    LLAssetType::EType type, void* userData, S32 status, LLExtStat extStatus)
{
    LLSD result(LLSD::emptyMap());

    result["asset_id"] = assetId;
    if (status)
    {
        result["error"] = status;
     
        if (status == LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE)
        {
            result["message"] = LLTrans::getString("CompileQueueProblemDownloading") + (":");
            result["alert"] = LLTrans::getString("CompileQueueScriptNotFound");
        }
        else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
        {
            result["message"] = LLTrans::getString("CompileQueueInsufficientPermFor") + (":");
            result["alert"] = LLTrans::getString("CompileQueueInsufficientPermDownload");
        }
        else
        {
            result["message"] = LLTrans::getString("CompileQueueUnknownFailure");
        }
#ifdef LSL_PREPROCESSOR
        // <FS:Ansariel> [LSL PreProc]
        // LSL PreProc error case
        delete ((HandleScriptUserData *)userData)->getData();
#endif
    }
#ifdef LSL_PREPROCESSOR
    // <FS:KC> [LSL PreProc]
    else if (gSavedSettings.getBOOL("_NACL_LSLPreprocessor"))
    {
        LLScriptQueueData* data = ((HandleScriptUserData *)userData)->getData();
        LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", data->mQueueID);

        if (queue && queue->mLSLProc)
        {
            LLVFile file(vfs, assetId, type);
            S32 file_length = file.getSize();
            std::vector<char> script_data(file_length + 1);
            file.read((U8*)&script_data[0], file_length);
            // put a EOS at the end
            script_data[file_length] = 0;

            queue->addProcessingMessage("CompileQueuePreprocessing", LLSD().with("SCRIPT", data->mItem->getName()));
            queue->mLSLProc->preprocess_script(assetId, data, type, LLStringExplicit(&script_data[0]));
        }
        result["preproc"] = true;
    }
    // </FS:KC> LSL Preprocessor
#endif
    LLEventPumps::instance().post(((HandleScriptUserData *)userData)->getPumpName(), result);

}

/*static*/
void LLFloaterCompileQueue::processExperienceIdResults(LLSD result, LLUUID parent)
{
    LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", parent);
    if (!queue)
        return;

    queue->experienceIdsReceived(result["experience_ids"]);

    // getDerived handle gets a handle that can be resolved to a parent class of the derived object.
    LLHandle<LLFloaterScriptQueue> hFloater(queue->getDerivedHandle<LLFloaterScriptQueue>());

    // note subtle difference here: getDerivedHandle in this case is for an LLFloaterCompileQueue
    fnQueueAction_t fn = boost::bind(LLFloaterCompileQueue::processScript,
        queue->getDerivedHandle<LLFloaterCompileQueue>(), _1, _2, _3);


    LLCoros::instance().launch("ScriptQueueCompile", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        queue->mStartString,
        hFloater,
        queue->mObjectList,
        fn));

}

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterCompileQueue::processScript(LLHandle<LLFloaterCompileQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLSD result;
    LLCheckedHandle<LLFloaterCompileQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro
    bool monocompile = floater->mMono;
    F32 fetch_timeout = gSavedSettings.getF32("QueueInventoryFetchTimeout");


    // Initial test to see if we can (or should) attempt to compile the script.
    LLInventoryItem *item = dynamic_cast<LLInventoryItem *>(inventory);

    if (!item)
    {
        LL_WARNS("SCRIPTQ") << "item retrieved is not an LLInventoryItem." << LL_ENDL;
        return true;
    }

    if (!item->getPermissions().allowModifyBy(gAgent.getID(), gAgent.getGroupID()) ||
        !item->getPermissions().allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
    {
            // <FS:Ansariel> Translation fixes
            //std::string buffer = "Skipping: " + item->getName() + "(Permissions)";
            LLStringUtil::format_map_t args;
            args["OBJECT_NAME"] = item->getName();
            std::string buffer = floater->getString( "SkippingPermissions", args );
            // </FS:Ansariel>
        floater->addStringMessage(buffer);
        return true;
    }

    // Attempt to retrieve the experience
    LLUUID experienceId;
    {
#ifdef LSL_PREPROCESSOR
        if (object->getRegion() && object->getRegion()->isCapabilityAvailable("GetMetadata"))
        {
#endif
        LLExperienceCache::instance().fetchAssociatedExperience(inventory->getParentUUID(), inventory->getUUID(),
            boost::bind(&LLFloaterCompileQueue::handleHTTPResponse, pump.getName(), _1));

        result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout,
            LLSDMap("timeout", LLSD::Boolean(true)));
#ifdef LSL_PREPROCESSOR
        }
        else
        {
            result = LLSD();
        }
        // </FS:Ansariel>
#endif

        if (result.has("timeout"))
        {   // A timeout filed in the result will always be true if present.
            LLStringUtil::format_map_t args;
            args["[OBJECT_NAME]"] = inventory->getName();
            std::string buffer = floater->getString("Timeout", args);
            floater->addStringMessage(buffer);
            return true;
        }

        if (result.has(LLExperienceCache::EXPERIENCE_ID))
        {
            experienceId = result[LLExperienceCache::EXPERIENCE_ID].asUUID();
            if (!floater->hasExperience(experienceId))
            {
                floater->addProcessingMessage("CompileNoExperiencePerm", 
                    LLSDMap("SCRIPT", inventory->getName())
                        ("EXPERIENCE", result[LLExperienceCache::NAME].asString()));
                return true;
            }
        }

    }

    {
#ifdef LSL_PREPROCESSOR
        // <FS:Ansariel> [LSL PreProc]
        //HandleScriptUserData    userData(pump.getName());
        HandleScriptUserData userData;
        if (gSavedSettings.getBOOL("_NACL_LSLPreprocessor"))
        {
            // Need to dump some stuff into an LLScriptQueueData struct for the LSL PreProc.
            LLScriptQueueData* datap = new LLScriptQueueData(floater->getKey().asUUID(), object->getID(), experienceId, item);
            userData = HandleScriptUserData(pump.getName(), datap);
        }
        else
        {
            userData = HandleScriptUserData(pump.getName(), NULL);
        }
        // </FS:Ansariel>
#else
        HandleScriptUserData    userData(pump.getName());
#endif
        // request the asset
        gAssetStorage->getInvItemAsset(LLHost(),
            gAgent.getID(),
            gAgent.getSessionID(),
            item->getPermissions().getOwner(),
            object->getID(),
            item->getUUID(),
            item->getAssetUUID(),
            item->getType(),
            &LLFloaterCompileQueue::handleScriptRetrieval,
            &userData);

        result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout,
            LLSDMap("timeout", LLSD::Boolean(true)));
    }

    if (result.has("timeout"))
    {   // A timeout filed in the result will always be true if present.
        LLStringUtil::format_map_t args;
        args["[OBJECT_NAME]"] = inventory->getName();
        std::string buffer = floater->getString("Timeout", args);
        floater->addStringMessage(buffer);
        return true;
    }

    if (result.has("error"))
    {
        LL_WARNS("SCRIPTQ") << "Inventory fetch returned with error. Code: " << result["error"].asString() << LL_ENDL;
        std::string buffer = result["message"].asString() + " " + inventory->getName();
        floater->addStringMessage(buffer);

        if (result.has("alert"))
        {
            LLSD args;
            args["MESSAGE"] = result["alert"].asString();
            LLNotificationsUtil::add("SystemMessage", args);
        }
        return true;
    }
#ifdef LSL_PREPROCESSOR
    // <FS:Ansariel> [LSL PreProc]
    if (result.has("preproc"))
    {
        // LSL Preprocessor handles it from here on
        return true;
    }
    // </FS:Ansariel>
#endif
    LLUUID assetId = result["asset_id"];

    std::string url = object->getRegion()->getCapability("UpdateScriptTask");

    {
        LLResourceUploadInfo::ptr_t uploadInfo(new LLQueuedScriptAssetUpload(object->getID(), 
            inventory->getUUID(), 
            assetId, 
            monocompile ? LLScriptAssetUpload::MONO : LLScriptAssetUpload::LSL2,
            true, 
            inventory->getName(), 
            LLUUID(), 
            experienceId, 
            boost::bind(&LLFloaterCompileQueue::handleHTTPResponse, pump.getName(), _4)));

        LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
    }

    result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout, LLSDMap("timeout", LLSD::Boolean(true)));

    if (result.has("timeout"))
    { // A timeout filed in the result will always be true if present.
        LLStringUtil::format_map_t args;
        args["[OBJECT_NAME]"] = inventory->getName();
        std::string buffer = floater->getString("Timeout", args);
        floater->addStringMessage(buffer);
        return true;
    }

    // Bytecode save completed
    if (result["compiled"])
    {
        std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" succeeded");

        //floater->addStringMessage(buffer);
        LL_INFOS() << buffer << LL_ENDL;
        // <FS:Ansariel> Translation fixes
        LLStringUtil::format_map_t args;
        args["OBJECT_NAME"] = inventory->getName();
        floater->addStringMessage(floater->getString("CompileSuccess", args));
        // </FS:Ansariel>
    }
    else
    {
        LLSD compile_errors = result["errors"];
        // <FS:Ansariel> Translation fixes
        //std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" failed:");
        LLStringUtil::format_map_t args;
        args["OBJECT_NAME"] = inventory->getName();
        std::string buffer = floater->getString( "CompileFailure", args );
        // </FS:Ansariel>
        floater->addStringMessage(buffer);
        for (LLSD::array_const_iterator line = compile_errors.beginArray();
            line < compile_errors.endArray(); line++)
        {
            std::string str = line->asString();
            str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

            floater->addStringMessage(str);
        }
        LL_INFOS() << result["errors"] << LL_ENDL;
    }

    return true;
}

bool LLFloaterCompileQueue::startQueue()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        std::string lookup_url = region->getCapability("GetCreatorExperiences");
        if (!lookup_url.empty())
        {
            LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t success =
                boost::bind(&LLFloaterCompileQueue::processExperienceIdResults, _1, getKey().asUUID());

            LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t failure =
                boost::bind(&LLFloaterCompileQueue::processExperienceIdResults, LLSD(), getKey().asUUID());

            LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(lookup_url,
                success, failure);
#ifdef LSL_PREPROCESSOR
            return true;
        }
        else
        {
            processExperienceIdResults(LLSD(), getKey().asUUID());
#else
	return TRUE;
#endif
        }
    }

    return true;
}


///----------------------------------------------------------------------------
/// Class LLFloaterResetQueue
///----------------------------------------------------------------------------

LLFloaterResetQueue::LLFloaterResetQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("ResetQueueTitle"));
	setStartString(LLTrans::getString("ResetQueueStart"));
}

LLFloaterResetQueue::~LLFloaterResetQueue()
{ 
}

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterResetQueue::resetObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("Resetting") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);
    
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_ScriptReset);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterResetQueue::startQueue()
{
    // Bind the resetObjectScripts method into a QueueAction function and pass it
    // into the object queue processing coroutine.
    fnQueueAction_t fn = boost::bind(LLFloaterResetQueue::resetObjectScripts,
        getDerivedHandle<LLFloaterScriptQueue>(), _1, _2, _3);

    LLCoros::instance().launch("ScriptResetQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        getDerivedHandle<LLFloaterScriptQueue>(),
        mObjectList,
        fn));

    return true;
}

///----------------------------------------------------------------------------
/// Class LLFloaterRunQueue
///----------------------------------------------------------------------------

LLFloaterRunQueue::LLFloaterRunQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("RunQueueTitle"));
	setStartString(LLTrans::getString("RunQueueStart"));
}

LLFloaterRunQueue::~LLFloaterRunQueue()
{ 
}

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterRunQueue::runObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("Running") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);

    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_SetScriptRunning);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->addBOOLFast(_PREHASH_Running, TRUE);
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterRunQueue::startQueue()
{
    LLHandle<LLFloaterScriptQueue> hFloater(getDerivedHandle<LLFloaterScriptQueue>());
    fnQueueAction_t fn = boost::bind(LLFloaterRunQueue::runObjectScripts, hFloater, _1, _2, _3);

    LLCoros::instance().launch("ScriptRunQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        hFloater,
        mObjectList,
        fn));

    return true;
}


///----------------------------------------------------------------------------
/// Class LLFloaterNotRunQueue
///----------------------------------------------------------------------------

LLFloaterNotRunQueue::LLFloaterNotRunQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("NotRunQueueTitle"));
	setStartString(LLTrans::getString("NotRunQueueStart"));
}

LLFloaterNotRunQueue::~LLFloaterNotRunQueue()
{ 
}

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterNotRunQueue::stopObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, 
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("NotRunning") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);

    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_SetScriptRunning);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->addBOOLFast(_PREHASH_Running, FALSE);
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterNotRunQueue::startQueue()
{
    LLHandle<LLFloaterScriptQueue> hFloater(getDerivedHandle<LLFloaterScriptQueue>());

    fnQueueAction_t fn = boost::bind(&LLFloaterNotRunQueue::stopObjectScripts, hFloater, _1, _2, _3);
    LLCoros::instance().launch("ScriptQueueNotRun", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        hFloater,
        mObjectList,
        fn));

    return true;
}

// <FS> Delete scripts
///----------------------------------------------------------------------------
/// Class LLFloaterDeleteQueue 
///----------------------------------------------------------------------------

LLFloaterDeleteQueue::LLFloaterDeleteQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
    setTitle(LLTrans::getString("DeleteQueueTitle"));
    setStartString(LLTrans::getString("DeleteQueueStart"));
}

LLFloaterDeleteQueue::~LLFloaterDeleteQueue()
{ 
}

bool LLFloaterDeleteQueue::deleteObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater,
	const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
	LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
	// Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
	// which is caught in objectScriptProcessingQueueCoro

	std::string buffer;
	buffer = floater->getString("Deleting") + (": ") + inventory->getName();
	floater->addStringMessage(buffer);

#ifdef LSL_PREPROCESSOR_OLD_CODE
	auto inv_uuid = inventory->getUUID();
	const_cast<LLViewerObject*>(object.get())->removeInventory(inv_uuid);
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_RemoveTaskInventory);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_InventoryData);
    msg->addU32Fast(_PREHASH_LocalID, object->getLocalID());
    msg->addUUIDFast(_PREHASH_ItemID, inv_uuid);
    msg->sendReliable(object->getRegion()->getHost());
#else
	const_cast<LLViewerObject*>(object.get())->removeInventory(inventory->getUUID());
#endif
	return true;
}

bool LLFloaterDeleteQueue::startQueue()
{
	LLHandle<LLFloaterScriptQueue> hFloater(getDerivedHandle<LLFloaterScriptQueue>());

	fnQueueAction_t fn = boost::bind(&LLFloaterDeleteQueue::deleteObjectScripts, hFloater, _1, _2, _3);
	LLCoros::instance().launch("ScriptDeleteQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
		mStartString,
		hFloater,
		mObjectList,
		fn));

	return true;
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
void ObjectInventoryFetcher::inventoryChanged(LLViewerObject* object,
        LLInventoryObject::object_list_t* inventory, S32 serial_num, void* user_data)
{
    mInventoryList.clear();
    mInventoryList.assign(inventory->begin(), inventory->end());

    mPump.post(LLSDMap("changed", LLSD::Boolean(true)));

}

void LLFloaterScriptQueue::objectScriptProcessingQueueCoro(std::string action, LLHandle<LLFloaterScriptQueue> hfloater,
    object_data_list_t objectList, fnQueueAction_t func)
{
    LLCoros::set_consuming(true);
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);    
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle. 
    // This is expected if the dialog closes.
    LLEventMailDrop        maildrop(QUEUE_EVENTPUMP_NAME, true);
    F32 fetch_timeout = gSavedSettings.getF32("QueueInventoryFetchTimeout");


    try
    {
        for (object_data_list_t::iterator itObj(objectList.begin()); (itObj != objectList.end()); ++itObj)
        {
            bool firstForObject = true;
			auto object_ptr = (*itObj); // <polarity/>
            LLUUID object_id = object_ptr.mObjectId; // <polarity/>
            LL_INFOS("SCRIPTQ") << "Next object in queue with ID=" << object_id.asString() << LL_ENDL;

            LLPointer<LLViewerObject> obj = gObjectList.findObject(object_id);
            LLInventoryObject::object_list_t inventory;
            if (obj)
            {
                ObjectInventoryFetcher::ptr_t fetcher(new ObjectInventoryFetcher(maildrop, obj, nullptr));

                fetcher->fetchInventory();

                LLStringUtil::format_map_t args;
				auto object_name = object_ptr.mObjectName; // <polarity/>
                args["[OBJECT_NAME]"] = object_name; // <polarity/>
                floater->addStringMessage(floater->getString("LoadingObjInv", args));

                LLSD result = llcoro::suspendUntilEventOnWithTimeout(maildrop, fetch_timeout,
                    LLSDMap("timeout", LLSD::Boolean(true)));

                if (result.has("timeout"))
                {    // A timeout filed in the result will always be true if present.
                    LL_WARNS("SCRIPTQ") << "Unable to retrieve inventory for object " << object_id.asString() <<
                        ". Skipping to next object." << LL_ENDL;

                    LLStringUtil::format_map_t args;
                    args["[OBJECT_NAME]"] = object_name; // <polarity/>
                    floater->addStringMessage(floater->getString("Timeout", args));

                    continue;
                }

                inventory.assign(fetcher->getInventoryList().begin(), fetcher->getInventoryList().end());
            }
            else
            {
                LL_WARNS("SCRIPTQ") << "Unable to retrieve object with ID of " << object_id <<
                    ". Skipping to next." << LL_ENDL;
                continue;
            }

            // TODO: Get the name of the object we are looking at here so that we can display it below.
            //std::string objName = (dynamic_cast<LLInventoryObject *>(obj.get()))->getName();
            LL_DEBUGS("SCRIPTQ") << "Object has " << inventory.size() << " items." << LL_ENDL;

            for (LLInventoryObject::object_list_t::iterator itInv = inventory.begin();
                itInv != inventory.end(); ++itInv)
            {
                floater.check();

                // note, we have a smart pointer to the obj above... but if we didn't we'd check that 
                // it still exists here.

				auto script_ptr = (*itInv); // <polarity/>
                if ((script_ptr->getType() == LLAssetType::AT_LSL_TEXT)) // <polarity/>
                {
                    LL_DEBUGS("SCRIPTQ") << "Inventory item " << script_ptr->getUUID().asString() << "\"" << script_ptr->getName() << "\"" << LL_ENDL; // <polarity/>
                    if (firstForObject)
                    {
                        //floater->addStringMessage(objName + ":");
                        firstForObject = false;
                    }

                    if (!func(obj, script_ptr, maildrop)) // <polarity/>
                    {
                        continue;
                    }
                }

                // no other explicit suspension point in this loop.  func(...) MIGHT suspend
                // but offers no guarantee of doing so.
                llcoro::suspend();
            }
        }

        // <FS:Ansariel> Translation fixes
        //floater->addStringMessage("Done");
        floater->addStringMessage(floater->getString("Done"));
        // </FS:Ansariel>
        LLButton* butt = floater->getChild<LLButton>("close");
        butt->setLabel(LLTrans::getString("Done"));
        butt->setEnabled(TRUE);
        static LLCachedControl<bool> auto_close_floater(gSavedSettings, "PVUI_ScriptQueueAutoClose", true);
        if(auto_close_floater)
        {
            LL_INFOS("SCRIPTQ") << "Automatically closing compile queue window." << LL_ENDL;
            floater->Close();
        }
    }
    catch (LLCheckedHandleBase::Stale &)
    {
        // This is expected.  It means that floater has been closed before 
        // processing was completed.
        LL_DEBUGS("SCRIPTQ") << "LLExeceptionStaleHandle caught! Floater has most likely been closed." << LL_ENDL;
    }
}
#ifdef LSL_PREPROCESSOR
// <FS:KC> [LSL PreProc]
class LLScriptAssetUploadWithId: public LLScriptAssetUpload
{
public:
	LLScriptAssetUploadWithId(	LLUUID taskId, LLUUID itemId, TargetType_t targetType, 
		bool isRunning, std::string scriptName, LLUUID queueId, LLUUID exerienceId, std::string buffer, taskUploadFinish_f finish )
		:  LLScriptAssetUpload( taskId, itemId, targetType,  isRunning, exerienceId, buffer, finish),
		mScriptName(scriptName),
        mQueueId(queueId)
	{
	}

    virtual LLSD prepareUpload()
    {
        LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(mQueueId));
        if (queue)
        {
            LLStringUtil::format_map_t args;
            args["OBJECT_NAME"] = getScriptName();
            std::string message = queue->getString("Compiling", args);

            queue->addStringMessage(message);
        }

        return LLBufferedAssetUploadInfo::prepareUpload();
    }

    std::string getScriptName() const { return mScriptName; }

private:
    void setScriptName(const std::string &scriptName) { mScriptName = scriptName; }

    LLUUID mQueueId;
    std::string mScriptName;
};

/*static*/
void LLFloaterCompileQueue::finishLSLUpload(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response, std::string scriptName, LLUUID queueId)
{
    LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(queueId));
    if (queue)
    {
        // Bytecode save completed
        if (response["compiled"])
        {
            std::string message = std::string("Compilation of \"") + scriptName + std::string("\" succeeded");
            LL_INFOS() << message << LL_ENDL;
            LLStringUtil::format_map_t args;
            args["OBJECT_NAME"] = scriptName;
            queue->addStringMessage(queue->getString("CompileSuccess", args));
        }
        else
        {
            LLSD compile_errors = response["errors"];
            for (LLSD::array_const_iterator line = compile_errors.beginArray();
                line < compile_errors.endArray(); line++)
            {
                std::string str = line->asString();
                str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
                queue->addStringMessage(str);
            }
            LL_INFOS() << response["errors"] << LL_ENDL;
        }
    }
}

// This is the callback after the script has been processed by preproc
// static
void LLFloaterCompileQueue::scriptPreprocComplete(const LLUUID& asset_id, LLScriptQueueData* data, LLAssetType::EType type, const std::string& script_text)
{
	LL_INFOS() << "LLFloaterCompileQueue::scriptPreprocComplete()" << LL_ENDL;
	if (!data)
	{
		return;
	}
	LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", data->mQueueID);
	
	if (queue)
	{
		std::string filename;
		std::string uuid_str;
		asset_id.toString(uuid_str);
		filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str) + llformat(".%s",LLAssetType::lookup(type));
		
		const bool is_running = true;
		LLViewerObject* object = gObjectList.findObject(data->mTaskId);
		if (object)
		{
			std::string scriptName = data->mItem->getName();
			std::string url = object->getRegion()->getCapability("UpdateScriptTask");
			if (!url.empty())
			{
				queue->addProcessingMessage("CompileQueuePreprocessingComplete", LLSD().with("SCRIPT", scriptName));

				LLBufferedAssetUploadInfo::taskUploadFinish_f proc = boost::bind(&LLFloaterCompileQueue::finishLSLUpload, _1, _2, _3, _4, 
					scriptName, data->mQueueID);

				LLResourceUploadInfo::ptr_t uploadInfo( new LLScriptAssetUploadWithId(
					data->mTaskId,
					data->mItem->getUUID(),
					(queue->mMono) ? LLScriptAssetUpload::MONO : LLScriptAssetUpload::LSL2,
					is_running,
					scriptName,
					data->mQueueID,
					data->mExperienceId,
					script_text,
					proc));

				LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
			}
			else
			{
				queue->addStringMessage(LLTrans::getString("CompileQueueServiceUnavailable"));
			}
		}
	}
	delete data;
}

// static
void LLFloaterCompileQueue::scriptLogMessage(LLScriptQueueData* data, std::string message)
{
	if (!data)
	{
		return;
	}
	LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", data->mQueueID);
	if (queue)
	{
		queue->addStringMessage(message);
	}
}
// </FS:KC>
#endif