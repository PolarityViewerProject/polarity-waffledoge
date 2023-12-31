/** 
 * @file llfloateravatarpicker.h
 * @brief was llavatarpicker.h
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

#ifndef LLFLOATERAVATARPICKER_H
#define LLFLOATERAVATARPICKER_H

#include "llfloater.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLAvatarName;
class LLScrollListCtrl;


static void getSelectedAvatarData(const LLScrollListCtrl* from, uuid_vec_t& avatar_ids, std::vector<LLAvatarName>& avatar_names);

class LLFloaterAvatarPicker :public LLFloater
{
public:
	typedef boost::signals2::signal<bool(const uuid_vec_t&), boost_boolean_combiner> validate_signal_t;
	typedef validate_signal_t::slot_type validate_callback_t;

	// The callback function will be called with an avatar name and UUID.
	typedef std::function<void (const uuid_vec_t&, const std::vector<LLAvatarName>&)> select_callback_t;
	// Call this to select an avatar.	
	static LLFloaterAvatarPicker* show(select_callback_t callback, 
									   BOOL allow_multiple = FALSE,
									   BOOL closeOnSelect = FALSE,
									   BOOL skip_agent = FALSE,
                                       const std::string& name = "",
                                       LLView * frustumOrigin = nullptr);

	LLFloaterAvatarPicker(const LLSD& key);
	virtual ~LLFloaterAvatarPicker();

	BOOL postBuild() override;

	void setOkBtnEnableCb(validate_callback_t cb);

	static void processAvatarPickerReply(class LLMessageSystem* msg, void**);
	void processResponse(const LLUUID& query_id, const LLSD& content);

	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg) override;

	void openFriendsTab();
	BOOL isExcludeAgentFromSearchResults() {return mExcludeAgentFromSearchResults;}

private:
	void editKeystroke(class LLLineEditor* caller, void* user_data);
	// <FS:Ansariel> Search by UUID
	void editKeystrokeUUID(class LLLineEditor* caller, void* user_data);
	void onBtnFindUUID();
	void onFindUUIDAvatarNameCache(const LLUUID& av_id, const LLAvatarName& av_name);
	boost::signals2::connection mFindUUIDAvatarNameCacheConnection;
	// </FS:Ansariel>

	void onBtnFind();
	void onBtnSelect();
	void onBtnRefresh();
	void onBtnSelf(); // <xenhat/> Self Button
	void onRangeAdjust();
	void onBtnClose();
	void onList();
	void onTabChanged();
	bool isSelectBtnEnabled();

	void populateNearMe();
	void populateFriend();
	BOOL visibleItemsSelected() const; // Returns true if any items in the current tab are selected.

    static void findCoro(std::string url, LLUUID mQueryID, std::string mName);
	void find();
	void setAllowMultiple(BOOL allow_multiple);
	LLScrollListCtrl* getActiveList() const;

    void drawFrustum();
	void draw() override;
	BOOL handleKeyHere(KEY key, MASK mask) override;

	LLUUID				mQueryID;
	int				    mNumResultsReturned;
	BOOL				mNearMeListComplete;
	BOOL				mCloseOnSelect;
	BOOL                mExcludeAgentFromSearchResults;
    LLHandle <LLView>   mFrustumOrigin;
    F32		            mContextConeOpacity;
    F32                 mContextConeInAlpha;
    F32                 mContextConeOutAlpha;
    F32                 mContextConeFadeTime;

	validate_signal_t mOkButtonValidateSignal;
	select_callback_t mSelectionCallback;

	bool mForSelfAgent; // <xenhat/> Self Button
};

#endif
