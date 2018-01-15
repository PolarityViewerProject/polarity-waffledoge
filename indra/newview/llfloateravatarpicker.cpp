// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llfloateravatarpicker.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfloateravatarpicker.h"

// Viewer includes
#include "llagent.h"
#include "llcallingcard.h"
#include "llfocusmgr.h"
#include "llfloaterreg.h"
#include "llimview.h"			// for gIMMgr
#include "lltooldraganddrop.h"	// for LLToolDragAndDrop
#include "llviewercontrol.h"
#include "llviewerregion.h"		// getCapability()
#include "llworld.h"

// Linden libraries
#include "llavatarnamecache.h"	// IDEVO
#include "llbutton.h"
#include "llcachename.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltabcontainer.h"
#include "lldraghandle.h"
#include "message.h"
#include "llcorehttputil.h"

//#include "llsdserialize.h"

static const U32 AVATAR_PICKER_SEARCH_TIMEOUT = 180U;

//put it back as a member once the legacy path is out?
static std::map<LLUUID, LLAvatarName> sAvatarNameMap;

LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(select_callback_t callback,
												   BOOL allow_multiple,
												   BOOL closeOnSelect,
												   BOOL skip_agent,
                                                   const std::string& name,
                                                   LLView * frustumOrigin)
{
	// *TODO: Use a key to allow this not to be an effective singleton
	LLFloaterAvatarPicker* floater = 
		LLFloaterReg::showTypedInstance<LLFloaterAvatarPicker>("avatar_picker", LLSD(name));
	if (!floater)
	{
		LL_WARNS() << "Cannot instantiate avatar picker" << LL_ENDL;
		return nullptr;
	}
	
	floater->mSelectionCallback = callback;
	floater->setAllowMultiple(allow_multiple);
	floater->mNearMeListComplete = FALSE;
	floater->mCloseOnSelect = closeOnSelect;
	floater->mExcludeAgentFromSearchResults = skip_agent;
	
	if (!closeOnSelect)
	{
		// Use Select/Close
		std::string select_string = floater->getString("Select");
		std::string close_string = floater->getString("Close");
		floater->getChild<LLButton>("ok_btn")->setLabel(select_string);
		floater->getChild<LLButton>("cancel_btn")->setLabel(close_string);
	}

    if(frustumOrigin)
    {
        floater->mFrustumOrigin = frustumOrigin->getHandle();
    }

	return floater;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker(const LLSD& key)
  : LLFloater(key),
	mNumResultsReturned(0),
	mNearMeListComplete(FALSE),
	mCloseOnSelect(FALSE),
    mContextConeOpacity	(0.f),
    mContextConeInAlpha(0.f),
    mContextConeOutAlpha(0.f),
    mContextConeFadeTime(0.f),
	mForSelfAgent(false), // <xenhat/> Self Button
	mFindUUIDAvatarNameCacheConnection() // <FS:Ansariel> Search by UUID
{
	mCommitCallbackRegistrar.add("Refresh.FriendList", boost::bind(&LLFloaterAvatarPicker::populateFriend, this));

    mContextConeInAlpha = gSavedSettings.getF32("ContextConeInAlpha");
    mContextConeOutAlpha = gSavedSettings.getF32("ContextConeOutAlpha");
    mContextConeFadeTime = gSavedSettings.getF32("ContextConeFadeTime");
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	getChild<LLLineEditor>("Edit")->setKeystrokeCallback( boost::bind(&LLFloaterAvatarPicker::editKeystroke, this, _1, _2), nullptr);

	childSetAction("Find", boost::bind(&LLFloaterAvatarPicker::onBtnFind, this));
	getChildView("Find")->setEnabled(FALSE);
	childSetAction("Refresh", boost::bind(&LLFloaterAvatarPicker::onBtnRefresh, this));
	getChild<LLUICtrl>("near_me_range")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onRangeAdjust, this));
	
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("SearchResults");
	list->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	list->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
	list->setContextMenu(LLScrollListCtrl::MENU_AVATAR_MINI);
	getChildView("SearchResults")->setEnabled(FALSE);
	
	list = getChild<LLScrollListCtrl>("NearMe");
	list->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	list->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
	list->setContextMenu(LLScrollListCtrl::MENU_AVATAR_MINI);

	list = getChild<LLScrollListCtrl>("Friends");
	list->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	list->setContextMenu(LLScrollListCtrl::MENU_AVATAR_MINI);
	getChild<LLUICtrl>("Friends")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

	childSetAction("ok_btn", boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	getChildView("ok_btn")->setEnabled(FALSE);
	childSetAction("cancel_btn", boost::bind(&LLFloaterAvatarPicker::onBtnClose, this));

	getChild<LLUICtrl>("Edit")->setFocus(TRUE);

	LLPanel* search_panel = getChild<LLPanel>("SearchPanel");
	if (search_panel)
	{
		// Start searching when Return is pressed in the line editor.
		search_panel->setDefaultBtn("Find");
	}

	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("no_results"));

	getChild<LLTabContainer>("ResidentChooserTabs")->setCommitCallback(
		boost::bind(&LLFloaterAvatarPicker::onTabChanged, this));
	
	// <FS:Ansariel> Search by UUID
	getChild<LLLineEditor>("EditUUID")->setKeystrokeCallback(boost::bind(&LLFloaterAvatarPicker::editKeystrokeUUID, this, _1, _2), NULL);
	childSetAction("FindUUID", boost::bind(&LLFloaterAvatarPicker::onBtnFindUUID, this));
	getChildView("FindUUID")->setEnabled(FALSE);

	LLScrollListCtrl* searchresultsuuid = getChild<LLScrollListCtrl>("SearchResultsUUID");
	searchresultsuuid->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	searchresultsuuid->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
	searchresultsuuid->setEnabled(FALSE);
	searchresultsuuid->setCommentText(getString("no_results"));

	getChild<LLPanel>("SearchPanelUUID")->setDefaultBtn("FindUUID");
	// </FS:Ansariel>
	
	childSetAction("me_button", boost::bind(&LLFloaterAvatarPicker::onBtnSelf, this)); // <xenhat/> Self Button

	setAllowMultiple(FALSE);
	
	center();
	
	populateFriend();

	return TRUE;
}

void LLFloaterAvatarPicker::setOkBtnEnableCb(validate_callback_t cb)
{
	mOkButtonValidateSignal.connect(cb);
}

void LLFloaterAvatarPicker::onTabChanged()
{
	getChildView("ok_btn")->setEnabled(isSelectBtnEnabled());
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
	// <FS:Ansariel> Search by UUID
	if (mFindUUIDAvatarNameCacheConnection.connected())
	{
		mFindUUIDAvatarNameCacheConnection.disconnect();
	}
	// </FS:Ansariel>
	gFocusMgr.releaseFocusIfNeeded( this );
}
// <FS:Ansariel> Search by UUID
void LLFloaterAvatarPicker::onBtnFindUUID()
{
	LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResultsUUID");
	search_results->deleteAllItems();
	search_results->setCommentText("searching");

	if (mFindUUIDAvatarNameCacheConnection.connected())
	{
		mFindUUIDAvatarNameCacheConnection.disconnect();
	}
	LLAvatarNameCache::get(LLUUID(getChild<LLLineEditor>("EditUUID")->getText()), boost::bind(&LLFloaterAvatarPicker::onFindUUIDAvatarNameCache, this, _1, _2));
}

void LLFloaterAvatarPicker::onFindUUIDAvatarNameCache(const LLUUID& av_id, const LLAvatarName& av_name)
{
	mFindUUIDAvatarNameCacheConnection.disconnect();
	LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResultsUUID");
	search_results->deleteAllItems();

	if (av_name.getAccountName() != "(?\?\?).(?\?\?)")
	{
		LLSD data;
		data["id"] = av_id;

		data["columns"][0]["name"] = "nameUUID";
		data["columns"][0]["value"] = av_name.getDisplayName();

		data["columns"][1]["name"] = "usernameUUID";
		data["columns"][1]["value"] = av_name.getUserName();

		search_results->addElement(data);
		search_results->setEnabled(TRUE);
		search_results->sortByColumnIndex(1, TRUE);
		search_results->selectFirstItem();
		onList();
		search_results->setFocus(TRUE);

		getChildView("ok_btn")->setEnabled(TRUE);

		// <xenhat> Self Button
		if (mForSelfAgent)
		{
			mForSelfAgent = false;
			uuid_vec_t			avatar_ids;
			std::vector<LLAvatarName>	avatar_names;
			getSelectedAvatarData(search_results, avatar_ids, avatar_names);
			mSelectionCallback(avatar_ids, avatar_names);
			onBtnClose();
		}
		// </xenhat>
	}
	else
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = getChild<LLUICtrl>("EditUUID")->getValue().asString();
		LLSD data;
		data["id"] = LLUUID::null;
		data["columns"][0]["column"] = "nameUUID";
		data["columns"][0]["value"] = getString("not_found", map);
		search_results->addElement(data);
		search_results->setEnabled(FALSE);
		getChildView("ok_btn")->setEnabled(FALSE);
	}
}
// </FS:Ansariel>

void LLFloaterAvatarPicker::onBtnFind()
{
	find();
}

void getSelectedAvatarData(const LLScrollListCtrl* from, uuid_vec_t& avatar_ids, std::vector<LLAvatarName>& avatar_names)
{
	std::vector<LLScrollListItem*> items = from->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID().notNull())
		{
			avatar_ids.push_back(item->getUUID());

			std::map<LLUUID, LLAvatarName>::iterator itr = sAvatarNameMap.find(item->getUUID());
			if (itr != sAvatarNameMap.end())
			{
				avatar_names.push_back(itr->second);
			}
			else
			{
				// the only case where it isn't in the name map is friends
				// but it should be in the name cache
				LLAvatarName av_name;
				LLAvatarNameCache::get(item->getUUID(), &av_name);
				avatar_names.push_back(av_name);
			}
		}
	}
}

void LLFloaterAvatarPicker::onBtnSelect()
{

	// If select btn not enabled then do not callback
	if (!isSelectBtnEnabled())
		return;

	if(mSelectionCallback)
	{
		std::string acvtive_panel_name;
		LLScrollListCtrl* list = nullptr;
		LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();
		if(active_panel)
		{
			acvtive_panel_name = active_panel->getName();
		}
		if(acvtive_panel_name == "SearchPanel")
		{
			list = getChild<LLScrollListCtrl>("SearchResults");
		}
		else if(acvtive_panel_name == "NearMePanel")
		{
			list = getChild<LLScrollListCtrl>("NearMe");
		}
		else if (acvtive_panel_name == "FriendsPanel")
		{
			list = getChild<LLScrollListCtrl>("Friends");
		}
		// <FS:Ansariel> Search by UUID
		else if (acvtive_panel_name == "SearchPanelUUID")
		{
			list = getChild<LLScrollListCtrl>("SearchResultsUUID");
		}
		// </FS:Ansariel>

		if(list)
		{
			uuid_vec_t			avatar_ids;
			std::vector<LLAvatarName>	avatar_names;
			getSelectedAvatarData(list, avatar_ids, avatar_names);
			mSelectionCallback(avatar_ids, avatar_names);
		}
	}
	getChild<LLScrollListCtrl>("SearchResults")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("NearMe")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("Friends")->deselectAllItems(TRUE);
	// <FS:Ansariel> Search by UUID
	getChild<LLScrollListCtrl>("SearchResultsUUID")->deselectAllItems(TRUE);
	// </FS:Ansariel>
	if(mCloseOnSelect)
	{
		mCloseOnSelect = FALSE;
		closeFloater();		
	}
}

void LLFloaterAvatarPicker::onBtnRefresh()
{
	getChild<LLScrollListCtrl>("NearMe")->deleteAllItems();
	getChild<LLScrollListCtrl>("NearMe")->setCommentText(getString("searching"));
	mNearMeListComplete = FALSE;
}

// <xenhat> Self Button
void LLFloaterAvatarPicker::onBtnSelf()
{
	getChild<LLScrollListCtrl>("SearchResults")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("NearMe")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("Friends")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("SearchResultsUUID")->deselectAllItems(TRUE);
	// TODO: Do all this without invoking UI actions
	getChild<LLTabContainer>("ResidentChooserTabs")->selectTabByName("SearchPanelUUID");
	auto edit_box = getChild<LLUICtrl>("EditUUID");
	edit_box->setEnabled(TRUE);
	edit_box->setFocus(TRUE);
	edit_box->setValue(gAgentID);
	getChild<LLUICtrl>("FindUUID")->setEnabled(TRUE);
	mForSelfAgent = true;
	onBtnFindUUID();
}
// </xenhat> Self Button

void LLFloaterAvatarPicker::onBtnClose()
{
	closeFloater();
}

void LLFloaterAvatarPicker::onRangeAdjust()
{
	onBtnRefresh();
}

void LLFloaterAvatarPicker::onList()
{
	getChildView("ok_btn")->setEnabled(isSelectBtnEnabled());
}

void LLFloaterAvatarPicker::populateNearMe()
{
	BOOL all_loaded = TRUE;
	BOOL empty = TRUE;
	LLScrollListCtrl* near_me_scroller = getChild<LLScrollListCtrl>("NearMe");
	near_me_scroller->deleteAllItems();

	uuid_vec_t avatar_ids;
	LLWorld::getInstance()->getAvatars(&avatar_ids, nullptr, gAgent.getPositionGlobal(), gSavedSettings.getF32("AvatarPickerRange"));
	for (auto it = avatar_ids.cbegin(), it_end = avatar_ids.cend(); it != it_end; ++it)
	{
		const LLUUID& av = *it;
		if(av == gAgent.getID()) continue;
		LLSD element;
		element["id"] = av; // value
		LLAvatarName av_name;

		if (!LLAvatarNameCache::get(av, &av_name))
		{
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = LLCacheName::getDefaultName();
			all_loaded = FALSE;
		}			
		else
		{
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = av_name.getDisplayName();
			element["columns"][1]["column"] = "username";
			element["columns"][1]["value"] = av_name.getUserName();

			sAvatarNameMap[av] = av_name;
		}
		near_me_scroller->addElement(element);
		empty = FALSE;
	}

	if (empty)
	{
		getChildView("NearMe")->setEnabled(FALSE);
		getChildView("ok_btn")->setEnabled(FALSE);
		near_me_scroller->setCommentText(getString("no_one_near"));
	}
	else 
	{
		getChildView("NearMe")->setEnabled(TRUE);
		getChildView("ok_btn")->setEnabled(TRUE);
		near_me_scroller->selectFirstItem();
		onList();
		near_me_scroller->setFocus(TRUE);
	}

	if (all_loaded)
	{
		mNearMeListComplete = TRUE;
	}
}

void LLFloaterAvatarPicker::populateFriend()
{
	LLScrollListCtrl* friends_scroller = getChild<LLScrollListCtrl>("Friends");
	friends_scroller->deleteAllItems();
	LLCollectAllBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	LLCollectAllBuddies::buddy_map_t::iterator it;
	
	for (it = collector.mOnline.begin(); it!=collector.mOnline.end(); ++it)
	{
		friends_scroller->addStringUUIDItem(it->second, it->first);
	}
	for (it = collector.mOffline.begin(); it!=collector.mOffline.end(); ++it)
	{
		friends_scroller->addStringUUIDItem(it->second, it->first);
	}
	friends_scroller->sortByColumnIndex(0, TRUE);
}

void LLFloaterAvatarPicker::drawFrustum()
{
    if(mFrustumOrigin.get())
    {
        LLView * frustumOrigin = mFrustumOrigin.get();
        LLRect origin_rect;
        frustumOrigin->localRectToOtherView(frustumOrigin->getLocalRect(), &origin_rect, this);
        // draw context cone connecting color picker with color swatch in parent floater
        LLRect local_rect = getLocalRect();
        if (hasFocus() && frustumOrigin->isInVisibleChain() && mContextConeOpacity > 0.001f)
        {
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLEnable(GL_CULL_FACE);
			gGL.begin(LLRender::TRIANGLE_STRIP);
			{
				gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
				gGL.vertex2i(origin_rect.mLeft, origin_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
				gGL.vertex2i(origin_rect.mRight, origin_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
				gGL.vertex2i(origin_rect.mRight, origin_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
				gGL.vertex2i(origin_rect.mLeft, origin_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
				gGL.vertex2i(origin_rect.mLeft, origin_rect.mTop);
			}
			gGL.end();
        }

        if (gFocusMgr.childHasMouseCapture(getDragHandle()))
        {
            mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLSmoothInterpolation::getInterpolant(mContextConeFadeTime));
        }
        else
        {
            mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLSmoothInterpolation::getInterpolant(mContextConeFadeTime));
        }
    }
}

void LLFloaterAvatarPicker::draw()
{
    drawFrustum();

	// sometimes it is hard to determine when Select/Ok button should be disabled (see LLAvatarActions::shareWithAvatars).
	// lets check this via mOkButtonValidateSignal callback periodically.
	static LLFrameTimer timer;
	if (timer.hasExpired())
	{
		timer.setTimerExpirySec(0.33f); // three times per second should be enough.

		// simulate list changes.
		onList();
		timer.start();
	}

	LLFloater::draw();
	if (!mNearMeListComplete && getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel() == getChild<LLPanel>("NearMePanel"))
	{
		populateNearMe();
	}
}

BOOL LLFloaterAvatarPicker::visibleItemsSelected() const
{
	LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();

	if(active_panel == getChild<LLPanel>("SearchPanel"))
	{
		return getChild<LLScrollListCtrl>("SearchResults")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("NearMePanel"))
	{
		return getChild<LLScrollListCtrl>("NearMe")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("FriendsPanel"))
	{
		return getChild<LLScrollListCtrl>("Friends")->getFirstSelectedIndex() >= 0;
	}
	// <FS:Ansariel> Search by UUID
	else if (active_panel == getChild<LLPanel>("SearchPanelUUID"))
	{
		return getChild<LLScrollListCtrl>("SearchResultsUUID")->getFirstSelectedIndex() >= 0;
	}
	// </FS:Ansariel>
	return FALSE;
}

/*static*/
void LLFloaterAvatarPicker::findCoro(std::string url, LLUUID queryID, std::string name)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    httpOpts->setTimeout(AVATAR_PICKER_SEARCH_TIMEOUT);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status || (status == LLCore::HttpStatus(HTTP_BAD_REQUEST)))
    {
        LLFloaterAvatarPicker* floater =
            LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker", name);
        if (floater)
        {
            result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
            floater->processResponse(queryID, result);
        }
    }
}


void LLFloaterAvatarPicker::find()
{
	//clear our stored LLAvatarNames
	sAvatarNameMap.clear();

	std::string text = getChild<LLUICtrl>("Edit")->getValue().asString();

	mQueryID.generate();

	std::string url;
	url.reserve(128); // avoid a memory allocation or two

	LLViewerRegion* region = gAgent.getRegion();
	if(region)
	{
		url = region->getCapability("AvatarPickerSearch");
		// Prefer use of capabilities to search on both SLID and display name
		if (!url.empty())
		{
			// capability urls don't end in '/', but we need one to parse
			// query parameters correctly
			if (url.size() > 0 && url[url.size()-1] != '/')
			{
				url += "/";
			}
			url += "?page_size=100&names=";
			std::replace(text.begin(), text.end(), '.', ' ');
			url += LLURI::escape(text);
			LL_INFOS() << "avatar picker " << url << LL_ENDL;

            LLCoros::instance().launch("LLFloaterAvatarPicker::findCoro",
                boost::bind(&LLFloaterAvatarPicker::findCoro, url, mQueryID, getKey().asString()));
		}
		else
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("AvatarPickerRequest");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->addUUID("QueryID", mQueryID);	// not used right now
			msg->nextBlock("Data");
			msg->addString("Name", text);
			gAgent.sendReliableMessage();
		}
	}
	getChild<LLScrollListCtrl>("SearchResults")->deleteAllItems();
	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("searching"));
	
	getChildView("ok_btn")->setEnabled(FALSE);
	mNumResultsReturned = 0;
}

void LLFloaterAvatarPicker::setAllowMultiple(BOOL allow_multiple)
{
	getChild<LLScrollListCtrl>("SearchResults")->setAllowMultipleSelection(allow_multiple);
	getChild<LLScrollListCtrl>("NearMe")->setAllowMultipleSelection(allow_multiple);
	getChild<LLScrollListCtrl>("Friends")->setAllowMultipleSelection(allow_multiple);
	// <FS:Ansariel> Search by UUID
	getChild<LLScrollListCtrl>("SearchResultsUUID")->setAllowMultipleSelection(allow_multiple);
}

LLScrollListCtrl* LLFloaterAvatarPicker::getActiveList() const
{
	std::string acvtive_panel_name;
	LLScrollListCtrl* list = nullptr;
	LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();
	if(active_panel)
	{
		acvtive_panel_name = active_panel->getName();
	}
	if(acvtive_panel_name == "SearchPanel")
	{
		list = getChild<LLScrollListCtrl>("SearchResults");
	}
	else if(acvtive_panel_name == "NearMePanel")
	{
		list = getChild<LLScrollListCtrl>("NearMe");
	}
	else if (acvtive_panel_name == "FriendsPanel")
	{
		list = getChild<LLScrollListCtrl>("Friends");
	}
	// <FS:Ansariel> Search by UUID
	else if (acvtive_panel_name == "SearchPanelUUID")
	{
		list = getChild<LLScrollListCtrl>("SearchResultsUUID");
	}
	// </FS:Ansariel>
	return list;
}

BOOL LLFloaterAvatarPicker::handleDragAndDrop(S32 x, S32 y, MASK mask,
											  BOOL drop, EDragAndDropType cargo_type,
											  void *cargo_data, EAcceptance *accept,
											  std::string& tooltip_msg)
{
	LLScrollListCtrl* list = getActiveList();
	if(list)
	{
		LLRect rc_list;
		LLRect rc_point(x,y,x,y);
		if (localRectToOtherView(rc_point, &rc_list, list))
		{
			// Keep selected only one item
			list->deselectAllItems(TRUE);
			list->selectItemAt(rc_list.mLeft, rc_list.mBottom, mask);
			LLScrollListItem* selection = list->getFirstSelected();
			if (selection)
			{
				LLUUID session_id = LLUUID::null;
				LLUUID dest_agent_id = selection->getUUID();
				std::string avatar_name = selection->getColumn(0)->getValue().asString();
				if (dest_agent_id.notNull() && dest_agent_id != gAgentID)
				{
					if (drop)
					{
						// Start up IM before give the item
						session_id = gIMMgr->addSession(avatar_name, IM_NOTHING_SPECIAL, dest_agent_id);
					}
					return LLToolDragAndDrop::handleGiveDragAndDrop(dest_agent_id, session_id, drop,
																	cargo_type, cargo_data, accept, getName());
				}
			}
		}
	}
	*accept = ACCEPT_NO;
	return TRUE;
}


void LLFloaterAvatarPicker::openFriendsTab()
{
	LLTabContainer* tab_container = getChild<LLTabContainer>("ResidentChooserTabs");
	if (tab_container == nullptr)
	{
		llassert(tab_container != NULL);
		return;
	}

	tab_container->selectTabByName("FriendsPanel");
}

// static 
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	avatar_id;
	std::string first_name;
	std::string last_name;

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "QueryID", query_id);

	// Not for us
	if (agent_id != gAgent.getID()) return;
	
	bool found = false;
	LLFloaterAvatarPicker* floater = nullptr;
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("avatar_picker");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		 iter != inst_list.end(); ++iter)
	{
		floater = dynamic_cast<LLFloaterAvatarPicker*>(*iter);
		if (floater && floater->mQueryID == query_id)
		{
			found = true;
			break;
		}
	}
	if (!found) return;

	LLScrollListCtrl* search_results = floater->getChild<LLScrollListCtrl>("SearchResults");

	// clear "Searching" label on first results
	if (floater->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}

	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocks("Data");
	for (S32 i = 0; i < num_new_rows; i++)
	{			
		msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID,	avatar_id, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, first_name, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_LastName,	last_name, i);

		if (avatar_id != agent_id || !floater->isExcludeAgentFromSearchResults()) // exclude agent from search results?
		{
			std::string avatar_name;
			if (avatar_id.isNull())
			{
				LLStringUtil::format_map_t map;
				map["[TEXT]"] = floater->getChild<LLUICtrl>("Edit")->getValue().asString();
				avatar_name = floater->getString("not_found", map);
				search_results->setEnabled(FALSE);
				floater->getChildView("ok_btn")->setEnabled(FALSE);
			}
			else
			{
				avatar_name = LLCacheName::buildFullName(first_name, last_name);
				search_results->setEnabled(TRUE);
				found_one = TRUE;

				LLAvatarName av_name;
				av_name.fromString(avatar_name);
				const LLUUID& id = avatar_id;
				sAvatarNameMap[id] = av_name;

			}
			LLSD element;
			element["id"] = avatar_id; // value
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = avatar_name;
			search_results->addElement(element);
		}
	}

	if (found_one)
	{
		floater->getChildView("ok_btn")->setEnabled(TRUE);
		search_results->selectFirstItem();
		floater->onList();
		search_results->setFocus(TRUE);
	}
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id, const LLSD& content)
{
	// Check for out-of-date query
	if (query_id == mQueryID)
	{
		LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResults");

		LLSD agents = content["agents"];

		// clear "Searching" label on first results
		search_results->deleteAllItems();

		LLSD item;
		LLSD::array_const_iterator it = agents.beginArray();
		for ( ; it != agents.endArray(); ++it)
		{
			const LLSD& row = *it;
			if (row["id"].asUUID() != gAgent.getID() || !mExcludeAgentFromSearchResults)
			{
				item["id"] = row["id"];
				LLSD& columns = item["columns"];
				columns[0]["column"] = "name";
				columns[0]["value"] = row["display_name"];
				columns[1]["column"] = "username";
				columns[1]["value"] = row["username"];
				search_results->addElement(item);

				// add the avatar name to our list
				LLAvatarName avatar_name;
				avatar_name.fromLLSD(row);
				sAvatarNameMap[row["id"].asUUID()] = avatar_name;
			}
		}

		if (search_results->isEmpty())
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = getChild<LLUICtrl>("Edit")->getValue().asString();
			LLSD element;
			element["id"] = LLUUID::null;
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = getString("not_found", map);
			search_results->addElement(element);
			search_results->setEnabled(false);
			getChildView("ok_btn")->setEnabled(false);
		}
		else
		{
			getChildView("ok_btn")->setEnabled(true);
			search_results->setEnabled(true);
			search_results->sortByColumnIndex(1, TRUE);
			std::string text = getChild<LLUICtrl>("Edit")->getValue().asString();
			if (!search_results->selectItemByLabel(text, TRUE, 1))
			{
				search_results->selectFirstItem();
			}			
			onList();
			search_results->setFocus(TRUE);
		}
	}
}

//static
void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
	getChildView("Find")->setEnabled(caller->getText().size() > 0);
}

// <FS:Ansariel> Search by UUID
void LLFloaterAvatarPicker::editKeystrokeUUID(LLLineEditor* caller, void* user_data)
{
	if (caller)
	{
		LLUUID id(caller->getText());
		getChildView("FindUUID")->setEnabled(!id.isNull());
	}
}
// </FS:Ansariel>

// virtual
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		if (getChild<LLUICtrl>("Edit")->hasFocus())
		{
			onBtnFind();
		}
		// <FS:Ansariel> Search by UUID
		else if (getChild<LLUICtrl>("EditUUID")->hasFocus())
		{
			onBtnFindUUID();
		}
		// </FS:Ansariel>
		else
		{
			onBtnSelect();
		}
		return TRUE;
	}
	if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		closeFloater();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

bool LLFloaterAvatarPicker::isSelectBtnEnabled()
{
	bool ret_val = visibleItemsSelected();

	if ( ret_val )
	{
		std::string acvtive_panel_name;
		LLScrollListCtrl* list = nullptr;
		LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();

		if(active_panel)
		{
			acvtive_panel_name = active_panel->getName();
		}

		if(acvtive_panel_name == "SearchPanel")
		{
			list = getChild<LLScrollListCtrl>("SearchResults");
		}
		else if(acvtive_panel_name == "NearMePanel")
		{
			list = getChild<LLScrollListCtrl>("NearMe");
		}
		else if (acvtive_panel_name == "FriendsPanel")
		{
			list = getChild<LLScrollListCtrl>("Friends");
		}
		// <FS:Ansariel> Search by UUID
		else if (acvtive_panel_name == "SearchPanelUUID")
		{
			list = getChild<LLScrollListCtrl>("SearchResultsUUID");
		}
		// </FS:Ansariel>

		if(list)
		{
			uuid_vec_t avatar_ids;
			std::vector<LLAvatarName> avatar_names;
			getSelectedAvatarData(list, avatar_ids, avatar_names);
			if (avatar_ids.size() >= 1) 
			{
				ret_val = mOkButtonValidateSignal.num_slots()?mOkButtonValidateSignal(avatar_ids):true;
			}
			else
			{
				ret_val = false;
			}
		}
	}

	return ret_val;
}
