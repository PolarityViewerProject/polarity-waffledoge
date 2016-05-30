
#include "llviewerprecompiledheaders.h"

#include "llpreviewscript.h"

#include "llassetstorage.h"
#include "llassetuploadresponders.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldir.h"
#include "llenvmanager.h"
#include "llexternaleditor.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llfloatergotoline.h"
#include "llexperiencecache.h"
#include "llfloaterexperienceprofile.h"
#include "llexperienceassociationresponder.h"
// [RLVa:KB] - Checked: 2011-05-22 (RLVa-1.3.1a)
#include "rlvhandler.h"
#include "rlvlocks.h"
// [/RLVa:KB]

const std::string HELLO_LSL =

static bool have_script_upload_cap(LLUUID& object_id)
{
	LLViewerObject* object = gObjectList.findObject(object_id);
	return object && (! object->getRegion()->getCapability("UpdateScriptTask").empty());
}


class ExperienceResponder : public LLHTTPClient::Responder
{
public:
	ExperienceResponder(const LLHandle<LLLiveLSLEditor>& parent):mParent(parent)
	{
	}

	LLHandle<LLLiveLSLEditor> mParent;

	/*virtual*/ void httpSuccess()
	{
		LLLiveLSLEditor* parent = mParent.get();
		if(!parent)
			return;

		parent->setExperienceIds(getContent()["experience_ids"]);		
	}
};

/// ---------------------------------------------------------------------------
/// LLLiveLSLFile
/// ---------------------------------------------------------------------------
class LLLiveLSLFile : public LLLiveFile
{
		//File picking cancelled by user, so nothing to do.
		return;
	}

	std::string filename = file_picker.getFirstFile();

	std::ifstream fin(filename.c_str());

	std::string line;
	std::string text;
	std::string linetotal;
	while (!fin.eof())
	{ 
	{
		LLFilePicker& file_picker = LLFilePicker::instance();
		if( file_picker.getSaveFile( LLFilePicker::FFSAVE_SCRIPT, self->mScriptName ) )
		{
			std::string filename = file_picker.getFirstFile();
			std::string scriptText=self->mEditor->getText();
			std::ofstream fout(filename.c_str());
			fout<<(scriptText);
			fout.close();
			self->mSaveCallback( self->mUserdata, FALSE );
		}
	}
}
		if(id == associated)
		{
			foundAssociated = true;
			position = ADD_TOP;
		}
		
		const LLSD& experience = LLExperienceCache::get(id);
		if(experience.isUndefined())
		{
			mExperiences->add(getString("loading"), id, position);
			last = id;
		}
		else
			mExperiences->add(experience_name_string, id, position);
		} 
	}

	if(!foundAssociated )
	{
		const LLSD& experience = LLExperienceCache::get(associated);
		if(experience.isDefined())
		{
			std::string experience_name_string = experience[LLExperienceCache::NAME].asString();
			if (experience_name_string.empty())
			{
				experience_name_string = LLTrans::getString("ExperienceNameUntitled");
		item->setEnabled(FALSE);
	}

	if(last.notNull())
	{
		mExperiences->setEnabled(FALSE);
		LLExperienceCache::get(last, boost::bind(&LLLiveLSLEditor::buildExperienceList, this));  
	}
	else
	{
		mExperiences->setEnabled(TRUE);
		mExperiences->sortByName(TRUE);
		mExperiences->setCurrentByIndex(mExperiences->getCurrentIndex());
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string lookup_url=region->getCapability("GetCreatorExperiences"); 
		if(!lookup_url.empty())
		{
			LLHTTPClient::get(lookup_url, new ExperienceResponder(getDerivedHandle<LLLiveLSLEditor>()));
		}
	}
}



/// ---------------------------------------------------------------------------
/// LLScriptEdContainer
/// ---------------------------------------------------------------------------

								item->getPermissions(), GP_OBJECT_MANIPULATE);
		BOOL is_modifiable = gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		if (gAgent.isGodlike() || (is_copyable && (is_modifiable || is_library)))
		{
			LLUUID* new_uuid = new LLUUID(mItemUUID);
			gAssetStorage->getInvItemAsset(LLHost::invalid,
										gAgent.getID(),
										gAgent.getSessionID(),
										item->getPermissions().getOwner(),
										LLUUID::null,
										item->getUUID(),
										item->getAssetUUID(),
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}

// Save needs to compile the text in the buffer. If the compile
// succeeds, then save both assets out to the database. If the compile
// fails, go ahead and save the text anyway.
void LLPreviewLSL::saveIfNeeded(bool sync /*= true*/)
{
	// LL_INFOS() << "LLPreviewLSL::saveIfNeeded()" << LL_ENDL;
	if(!mScriptEd->hasChanged())
	{
		return;
	}

	mPendingUploads = 0;
	mScriptEd->mErrorList->deleteAllItems();
	mScriptEd->mEditor->makePristine();

	// save off asset into file
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = filepath + ".lsl";

	mScriptEd->writeToFile(filename);

	if (sync)
	{
		mScriptEd->sync();
	}

	const LLInventoryItem *inv_item = getItem();
	// save it out to asset server
	std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
	if(inv_item)
	{
		getWindow()->incBusyCount();
		mPendingUploads++;
		if (!url.empty())
		{
			uploadAssetViaCaps(url, filename, mItemUUID);
		}
	}
}

void LLPreviewLSL::uploadAssetViaCaps(const std::string& url,
									  const std::string& filename,
									  const LLUUID& item_id)
{
	LL_INFOS() << "Update Agent Inventory via capability" << LL_ENDL;
	LLSD body;
	body["item_id"] = item_id;
	static LLCachedControl<bool> save_as_mono(gSavedSettings, "PVInventory_SaveScriptsAsMono", false);
	body["target"] = (save_as_mono) ? "mono" : "lsl2";
	LLHTTPClient::post(url, body, new LLUpdateAgentInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}

// static
void LLPreviewLSL::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLScriptSaveInfo* info = reinterpret_cast<LLScriptSaveInfo*>(user_data);
	if(0 == status)
	{
		if (info)
		{
			const LLViewerInventoryItem* item;
			item = (const LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if(item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setAssetUUID(asset_uuid);
				new_item->setTransactionID(info->mTransactionID);
				new_item->updateServer(FALSE);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				LL_WARNS() << "Inventory item for script " << info->mItemUUID
					<< " is no longer in agent inventory." << LL_ENDL;
			}

			// Find our window and close it if requested.
			LLPreviewLSL* self = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", info->mItemUUID);
			if (self)
			{
				getWindow()->decBusyCount();
				self->mPendingUploads--;
				if (self->mPendingUploads <= 0
					&& self->mCloseAfterSave)
				{
					self->closeFloater();
				}
			}
		}
	}
	else
	{
		LL_WARNS() << "Problem saving script: " << status << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("SaveScriptFailReason", args);
	}
	delete info;
}

// static
void LLPreviewLSL::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLUUID* instance_uuid = (LLUUID*)user_data;
	LLPreviewLSL* self = NULL;
	if(instance_uuid)
	{
		self = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", *instance_uuid);
	}
	if (0 == status)
	{
		if (self)
		{
			LLSD row;
			row["columns"][0]["value"] = "Compile successful!";
			row["columns"][0]["font"] = "SANSSERIF_SMALL";
			self->mScriptEd->mErrorList->addElement(row);

			// Find our window and close it if requested.
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
	}
	else
	{
		LL_WARNS() << "Problem saving LSL Bytecode (Preview)" << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("SaveBytecodeFailReason", args);
	}
	delete instance_uuid;
}

// static
void LLPreviewLSL::onLoadComplete( LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type,
								   void* user_data, S32 status, LLExtStat ext_status)
{
	LLScriptEdContainer(key),
	mAskedForRunningInfo(FALSE),
	mHaveRunningInfo(FALSE),
	mCloseAfterSave(FALSE),
	mPendingUploads(0),
	mIsModifiable(FALSE),
	mIsNew(false)
{
	mFactoryMap["script ed panel"] = LLCallbackMap(LLLiveLSLEditor::createScriptEdPanel, this);
}

BOOL LLLiveLSLEditor::postBuild()
{
												  const LLUUID& item_id,
												  bool is_script_running)
{
	LL_DEBUGS() << "LSL Bytecode saved" << LL_ENDL;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileFailed(const LLSD& compile_errors)
{
		row["columns"][0]["value"] = error_message;
		// *TODO: change to "MONOSPACE" and change llfontgl.cpp?
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}

void LLLiveLSLEditor::loadAsset()
{
	//LL_INFOS() << "LLLiveLSLEditor::loadAsset()" << LL_ENDL;
		if(object)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));

			if(item)
			{
				ExperienceAssociationResponder::fetchAssociatedExperience(item->getParentUUID(), item->getUUID(), boost::bind(&LLLiveLSLEditor::setAssociatedExperience, getDerivedHandle<LLLiveLSLEditor>(), _1));
				
				bool isGodlike = gAgent.isGodlike();
				bool copyManipulate = gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				mIsModifiable = gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				
				if(!isGodlike && (!copyManipulate || !mIsModifiable))
				{
	LLCheckBoxCtrl* runningCheckbox = getChild<LLCheckBoxCtrl>( "running");
	if(object && mAskedForRunningInfo && mHaveRunningInfo)
	{
		if(object->permAnyOwner())
		{
			runningCheckbox->setLabel(getString("script_running"));
			runningCheckbox->setEnabled(TRUE);

			if(object->permAnyOwner())
			{
				runningCheckbox->setLabel(getString("script_running"));
				runningCheckbox->setEnabled(TRUE);
			}
			else
			{
				runningCheckbox->setLabel(getString("public_objects_can_not_run"));
				runningCheckbox->setEnabled(FALSE);
				// *FIX: Set it to false so that the ui is correct for
				// a box that is released to public. It could be
				// incorrect after a release/claim cycle, but will be
				// correct after clicking on it.
				runningCheckbox->set(FALSE);
				mMonoCheckbox->set(FALSE);
			}
		}
		else
		{
			runningCheckbox->setLabel(getString("public_objects_can_not_run"));
			runningCheckbox->setEnabled(FALSE);

	mActive(active)
{
	llassert(item);
	mItem = new LLViewerInventoryItem(item);
}

// virtual
void LLLiveLSLEditor::saveIfNeeded(bool sync /*= true*/)
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	if(!object)
	{
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}

	if(mItem.isNull() || !mItem->isFinished())
	{
		// $NOTE: While the error message may not be exactly correct,
		// it's pretty close.
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}

// [RLVa:KB] - Checked: 2010-11-25 (RLVa-1.2.2b) | Modified: RLVa-1.2.2b
	if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.isLockedAttachment(object->getRootEdit())) )
	{
		RlvUtil::notifyBlockedGeneric();
		return;
	}
// [/RLVa:KB]

	// get the latest info about it. We used to be losing the script
	// name on save, because the viewer object version of the item,
	// and the editor version would get out of synch. Here's a good
	// place to synch them back up.
	LLInventoryItem* inv_item = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mItemUUID));
	if(inv_item)
	{
		mItem->copyItem(inv_item);
	}

	// Don't need to save if we're pristine
	if(!mScriptEd->hasChanged())
	{
		return;
	}

	mPendingUploads = 0;

	// save the script
	mScriptEd->enableSave(FALSE);
	mScriptEd->mEditor->makePristine();
	mScriptEd->mErrorList->deleteAllItems();

	// set up the save on the local machine.
	mScriptEd->mEditor->makePristine();
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = llformat("%s.lsl", filepath.c_str());

	mItem->setAssetUUID(asset_id);
	mItem->setTransactionID(tid);

	mScriptEd->writeToFile(filename);

	if (sync)
	{
		mScriptEd->sync();
	}
	
	// save it out to asset server
	std::string url = object->getRegion()->getCapability("UpdateScriptTask");
	getWindow()->incBusyCount();
	mPendingUploads++;
	BOOL is_running = getChild<LLCheckBoxCtrl>( "running")->get();
	if (!url.empty())
	{
		uploadAssetViaCaps(url, filename, mObjectUUID, mItemUUID, is_running, mScriptEd->getAssociatedExperience());
	}
}

void LLLiveLSLEditor::uploadAssetViaCaps(const std::string& url,
										 const std::string& filename,
										 const LLUUID& task_id,
										 const LLUUID& item_id,
										 BOOL is_running,
										 const LLUUID& experience_public_id )
{
	LL_INFOS() << "Update Task Inventory via capability " << url << LL_ENDL;
	LLSD body;
	body["task_id"] = task_id;
	body["item_id"] = item_id;
	body["is_script_running"] = is_running;
	body["target"] = monoChecked() ? "mono" : "lsl2";
	body["experience"] = experience_public_id;
	LLHTTPClient::post(url, body,
		new LLUpdateTaskInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}

void LLLiveLSLEditor::onSaveTextComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;

	if (status)
	{
		LL_WARNS() << "Unable to save text for a script." << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("CompileQueueSaveText", args);
	}
	else
	{
		LLSD floater_key;
		floater_key["taskid"] = data->mSaveObjectID;
		floater_key["itemid"] = data->mItem->getUUID();
		LLLiveLSLEditor* self = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", floater_key);
		if (self)
		{
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
	}
	delete data;
	data = NULL;
}


void LLLiveLSLEditor::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;
	if(!data)
		return;
	if(0 ==status)
	{
		LL_INFOS() << "LSL Bytecode saved" << LL_ENDL;
		LLSD floater_key;
		floater_key["taskid"] = data->mSaveObjectID;
		floater_key["itemid"] = data->mItem->getUUID();
		LLLiveLSLEditor* self = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", floater_key);
		if (self)
		{
			// Tell the user that the compile worked.
			self->mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
			// close the window if this completes both uploads
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
		LLViewerObject* object = gObjectList.findObject(data->mSaveObjectID);
		if(object)
		{
			object->saveScript(data->mItem, data->mActive, false);
			dialog_refresh_all();
			//LLToolDragAndDrop::dropScript(object, ids->first,
			//						  LLAssetType::AT_LSL_TEXT, FALSE);
		}
	}
	else
	{
		LL_INFOS() << "Problem saving LSL Bytecode (Live Editor)" << LL_ENDL;
		LL_WARNS() << "Unable to save a compiled script." << LL_ENDL;

		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("CompileQueueSaveBytecode", args);
	}

	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_uuid.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	LLFile::remove(dst_filename);
	delete data;
}

BOOL LLLiveLSLEditor::canClose()
{
	return (mScriptEd->canClose());
}
