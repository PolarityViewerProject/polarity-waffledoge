/** 
 * @file llsyswellwindow.cpp
 * @brief                                    // TODO
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */


#include "llviewerprecompiledheaders.h" // must be first include

#include "llflatlistview.h"

#include "llsyswellwindow.h"

#include "llbottomtray.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include "llchiclet.h"
#include "lltoastpanel.h"
#include "llnotificationmanager.h"


//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLDockableFloater(NULL, key),
													mChannel(NULL),
													mMessageList(NULL),
													mSeparator(NULL)
{
	LLIMMgr::getInstance()->addSessionObserver(this);
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLSysWellWindow::findIMChiclet, this, _1));

	mTypedItemsCount[IT_NOTIFICATION] = 0;
	mTypedItemsCount[IT_INSTANT_MESSAGE] = 0;
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mMessageList = getChild<LLFlatListView>("notification_list");

	// init connections to the list's update events
	connectListUpdaterToSignal("notify");
	connectListUpdaterToSignal("groupnotify");
	connectListUpdaterToSignal("offer");

	// get a corresponding channel
	initChannel();

	LLPanel::Params params;
	mSeparator = LLUICtrlFactory::create<LLPanel>(params);
	LLUICtrlFactory::instance().buildPanel(mSeparator, "panel_separator.xml");

	LLRect rc = mSeparator->getRect();
	rc.setOriginAndSize(0, 0, mMessageList->getItemsRect().getWidth(), rc.getHeight());
	mSeparator->setRect(rc);
	mSeparator->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
	mSeparator->setVisible(FALSE);

	mMessageList->addItem(mSeparator);

	return LLDockableFloater::postBuild();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setMinimized(BOOL minimize)
{
	// we don't show empty Message Well window
	if (!minimize && isWindowEmpty())
	{
		return;
	}

	LLDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::connectListUpdaterToSignal(std::string notification_type)
{
	LLNotificationsUI::LLNotificationManager* manager = LLNotificationsUI::LLNotificationManager::getInstance();
	LLNotificationsUI::LLEventHandler* n_handler = manager->getHandlerForNotification(notification_type);
	if(n_handler)
	{
		n_handler->setNotificationIDCallback(boost::bind(&LLSysWellWindow::removeItemByID, this, _1));
	}
	else
	{
		llwarns << "LLSysWellWindow::connectListUpdaterToSignal() - could not get a handler for '" << notification_type <<"' type of notifications" << llendl;
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onStartUpToastClick(S32 x, S32 y, MASK mask)
{
	onChicletClick();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onChicletClick()
{
	// 1 - remove StartUp toast and channel if present
	if(!LLNotificationsUI::LLScreenChannel::getStartUpToastShown())
	{
		LLNotificationsUI::LLChannelManager::getInstance()->onStartUpToastClose();
	}

	// 2 - toggle instance of SysWell's chiclet-window
	toggleWindow();
}

//---------------------------------------------------------------------------------
LLSysWellWindow::~LLSysWellWindow()
{
	LLIMMgr::getInstance()->removeSessionObserver(this);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::addItem(LLSysWellItem::Params p)
{
	LLSD value = p.notification_id;
	// do not add clones
	if( mMessageList->getItemByValue(value))
		return;

	LLSysWellItem* new_item = new LLSysWellItem(p);
	if (mMessageList->addItem(new_item, value, ADD_TOP))
	{
		handleItemAdded(IT_NOTIFICATION);

		reshapeWindow();

		new_item->setOnItemCloseCallback(boost::bind(&LLSysWellWindow::onItemClose, this, _1));
		new_item->setOnItemClickCallback(boost::bind(&LLSysWellWindow::onItemClick, this, _1));
	}
	else
	{
		llwarns << "Unable to add Notification into the list, notification ID: " << p.notification_id
			<< ", title: " << p.title
			<< llendl;

		new_item->die();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::clear()
{
	mMessageList->clear();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::removeItemByID(const LLUUID& id)
{
	if(mMessageList->removeItemByValue(id))
	{
		handleItemRemoved(IT_NOTIFICATION);
		reshapeWindow();
	}
	else
	{
		llwarns << "Unable to remove notification from the list, ID: " << id
			<< llendl;
	}

	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClick(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	if(mChannel)
		mChannel->loadStoredToastByNotificationIDToChannel(id);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClose(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	removeItemByID(id);
	if(mChannel)
		mChannel->killToastByNotificationID(id);
}

//--------------------------------------------------------------------------
void LLSysWellWindow::onStoreToast(LLPanel* info_panel, LLUUID id)
{
	LLSysWellItem::Params p;	
	p.notification_id = id;
	p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
	addItem(p);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::initChannel() 
{
	LLNotificationsUI::LLScreenChannelBase* channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(
																LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
	mChannel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(channel);
	if(mChannel)
	{
		mChannel->setOnStoreToastCallback(boost::bind(&LLSysWellWindow::onStoreToast, this, _1, _2));
	}
	else
	{
		llwarns << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << llendl;
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectScaled();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::toggleWindow()
{
	if (getDockControl() == NULL)
	{
		setDockControl(new LLDockControl(
				LLBottomTray::getInstance()->getSysWell(), this,
				getDockTongue(), LLDockControl::TOP, boost::bind(&LLSysWellWindow::getAllowedRect, this, _1)));
	}

	if(!getVisible() || isMinimized())
	{
		if(mChannel)
		{
			mChannel->removeAndStoreAllStorableToasts();
		}
		if(isWindowEmpty())
		{
			return;
		}

		setVisible(TRUE);
	}
	else if (isDocked())
	{
		setVisible(FALSE);
	}
	else if(!isDocked())
	{
		// bring to front undocked floater
		setVisible(TRUE);
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	if(visible)
	{
		if (LLBottomTray::instanceExists())
		{
			LLBottomTray::getInstance()->getSysWell()->setToggleState(TRUE);
		}
	}
	else
	{
		if (LLBottomTray::instanceExists())
		{
			LLBottomTray::getInstance()->getSysWell()->setToggleState(FALSE);
		}
	}

	LLDockableFloater::setVisible(visible);

	// update notification channel state	
	if(mChannel)
	{
		mChannel->updateShowToastsState();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setDocked(bool docked, bool pop_on_undock)
{
	LLDockableFloater::setDocked(docked, pop_on_undock);

	// update notification channel state
	if(mChannel)
	{
		mChannel->updateShowToastsState();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// save difference between floater height and the list height to take it into account while calculating new window height
	// it includes height from floater top to list top and from floater bottom and list bottom
	static S32 parent_list_delta_height = getRect().getHeight() - mMessageList->getRect().getHeight();

	S32 notif_list_height = mMessageList->getItemsRect().getHeight() + 2 * mMessageList->getBorderWidth();

	LLRect curRect = getRect();

	S32 new_window_height = notif_list_height + parent_list_delta_height;

	if (new_window_height > MAX_WINDOW_HEIGHT)
	{
		new_window_height = MAX_WINDOW_HEIGHT;
	}
	S32 newY = curRect.mTop + new_window_height - curRect.getHeight();
	curRect.setLeftTopAndSize(curRect.mLeft, newY, MIN_WINDOW_WIDTH, new_window_height);
	reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
	setRect(curRect);

	// update notification channel state
	// update on a window reshape is important only when a window is visible and docked
	if(mChannel && getVisible() && isDocked())
	{
		mChannel->updateShowToastsState();
	}
}

//---------------------------------------------------------------------------------
LLChiclet* LLSysWellWindow::findIMChiclet(const LLUUID& sessionId)
{
	LLChiclet* res = NULL;
	RowPanel* panel = mMessageList->getTypedItemByValue<RowPanel>(sessionId);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::addIMRow(const LLUUID& sessionId, S32 chicletCounter,
		const std::string& name, const LLUUID& otherParticipantId)
{
	RowPanel* item = new RowPanel(this, sessionId, chicletCounter, name, otherParticipantId);
	if (mMessageList->insertItemAfter(mSeparator, item, sessionId))
	{
		handleItemAdded(IT_INSTANT_MESSAGE);
	}
	else
	{
		llwarns << "Unable to add IM Row into the list, sessionID: " << sessionId
			<< ", name: " << name
			<< ", other participant ID: " << otherParticipantId
			<< llendl;

		item->die();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::delIMRow(const LLUUID& sessionId)
{
	if (mMessageList->removeItemByValue(sessionId))
	{
		handleItemRemoved(IT_INSTANT_MESSAGE);
	}
	else
	{
		llwarns << "Unable to remove IM Row from the list, sessionID: " << sessionId
			<< llendl;
	}

	// remove all toasts that belong to this session from a screen
	if(mChannel)
		mChannel->removeToastsBySessionID(sessionId);

	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}

//---------------------------------------------------------------------------------
bool LLSysWellWindow::isWindowEmpty()
{
	// keep in mind, mSeparator is always in the list
	return mMessageList->size() == 1;
}

//---------------------------------------------------------------------------------
//virtual
void LLSysWellWindow::sessionAdded(const LLUUID& session_id,
		const std::string& name, const LLUUID& other_participant_id)
{
	if (mMessageList->getItemByValue(session_id) == NULL)
	{
		S32 chicletCounter = LLIMModel::getInstance()->getNumUnread(session_id);
		if (chicletCounter > -1)
		{
			addIMRow(session_id, chicletCounter, name, other_participant_id);	
			reshapeWindow();
		}
	}
}

//---------------------------------------------------------------------------------
//virtual
void LLSysWellWindow::sessionRemoved(const LLUUID& sessionId)
{
	delIMRow(sessionId);
	reshapeWindow();
}

void LLSysWellWindow::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	//for outgoing ad-hoc and group im sessions only
	LLChiclet* chiclet = findIMChiclet(old_session_id);
	if (chiclet)
	{
		chiclet->setSessionId(new_session_id);
		mMessageList->updateValue(old_session_id, new_session_id);
	}
}

void LLSysWellWindow::handleItemAdded(EItemType added_item_type)
{
	bool should_be_shown = ++mTypedItemsCount[added_item_type] == 1 && anotherTypeExists(added_item_type);

	if (should_be_shown && !mSeparator->getVisible())
	{
		mSeparator->setVisible(TRUE);

		// refresh list to recalculate mSeparator position
		mMessageList->reshape(mMessageList->getRect().getWidth(), mMessageList->getRect().getHeight());
	}
}

void LLSysWellWindow::handleItemRemoved(EItemType removed_item_type)
{
	bool should_be_hidden = --mTypedItemsCount[removed_item_type] == 0;

	if (should_be_hidden && mSeparator->getVisible())
	{
		mSeparator->setVisible(FALSE);

		// refresh list to recalculate mSeparator position
		mMessageList->reshape(mMessageList->getRect().getWidth(), mMessageList->getRect().getHeight());
	}
}

bool LLSysWellWindow::anotherTypeExists(EItemType item_type)
{
	bool exists = false;
	switch(item_type)
	{
	case IT_INSTANT_MESSAGE:
		if (mTypedItemsCount[IT_NOTIFICATION] > 0)
		{
			exists = true;
		}
		break;
	case IT_NOTIFICATION:
		if (mTypedItemsCount[IT_INSTANT_MESSAGE] > 0)
		{
			exists = true;
		}
		break;
	}
	return exists;
}

//---------------------------------------------------------------------------------
LLSysWellWindow::RowPanel::RowPanel(const LLSysWellWindow* parent, const LLUUID& sessionId,
		S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId) :
		LLPanel(LLPanel::Params()), mChiclet(NULL), mParent(parent)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_activeim_row.xml", NULL);

	// Choose which of the pre-created chiclets (IM/group) to use.
	// The other one gets hidden.

	LLIMChiclet::EType im_chiclet_type = LLIMChiclet::getIMSessionType(sessionId);
	switch (im_chiclet_type)
	{
	case LLIMChiclet::TYPE_GROUP:
		mChiclet = getChild<LLIMGroupChiclet>("group_chiclet");
		break;
	case LLIMChiclet::TYPE_AD_HOC:
		mChiclet = getChild<LLAdHocChiclet>("adhoc_chiclet");		
		break;
	case LLIMChiclet::TYPE_UNKNOWN: // assign mChiclet a non-null value anyway
	case LLIMChiclet::TYPE_IM:
		mChiclet = getChild<LLIMP2PChiclet>("p2p_chiclet");
		break;
	}

	// Initialize chiclet.
	mChiclet->setCounter(chicletCounter);
	mChiclet->setSessionId(sessionId);
	mChiclet->setIMSessionName(name);
	mChiclet->setOtherParticipantId(otherParticipantId);
	mChiclet->setVisible(true);

	LLTextBox* contactName = getChild<LLTextBox>("contact_name");
	contactName->setValue(name);

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLSysWellWindow::RowPanel::onClosePanel, this));
}

//---------------------------------------------------------------------------------
LLSysWellWindow::RowPanel::~RowPanel()
{
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onClosePanel()
{
	gIMMgr->leaveSession(mChiclet->getSessionId());
	// This row panel will be removed from the list in LLSysWellWindow::sessionRemoved().
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLSysWellWindow::RowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if (!mChiclet->pointInView(x, y) && !mCloseBtn->getRect().pointInRect(x, y)) // prevent double call of LLIMChiclet::onMouseDown()
		mChiclet->onMouseDown();

	return LLPanel::handleMouseDown(x, y, mask);
}

// EOF
