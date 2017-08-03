/** 
 * @file llfloaterregioninfo.h
 * @author Aaron Brashears
 * @brief Declaration of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERREGIONINFO_H
#define LL_LLFLOATERREGIONINFO_H

#include "llassettype.h"
#include "llfloater.h"
#include "llhost.h"
#include "llpanel.h"
#include "llextendedstatus.h"

#include "llenvmanager.h" // for LLEnvironmentSettings
#include "lleventcoro.h"

class LLAvatarName;
class LLDispatcher;
class LLLineEditor;
class LLMessageSystem;
class LLPanelRegionInfo;
class LLTabContainer;
class LLViewerRegion;
class LLViewerTextEditor;
class LLInventoryItem;
class LLCheckBoxCtrl;
class LLComboBox;
class LLNameListCtrl;
class LLRadioGroup;
class LLSliderCtrl;
class LLSpinCtrl;
class LLTextBox;
class LLVFS;

class LLPanelRegionGeneralInfo;
class LLPanelRegionDebugInfo;
class LLPanelRegionTerrainInfo;
class LLPanelEstateInfo;
class LLPanelEstateCovenant;
class LLPanelExperienceListEditor;
class LLPanelExperiences;
class LLPanelRegionExperiences;

class LLEventTimer;
class LLEnvironmentSettings;
class LLWLParamManager;
class LLWaterParamManager;
class LLWLParamSet;
class LLWaterParamSet;

class LLFloaterRegionInfo : public LLFloater
{
	friend class LLFloaterReg;
public:


	/*virtual*/ void onOpen(const LLSD& key) override;
	/*virtual*/ BOOL postBuild() override;

	static void processEstateOwnerRequest(LLMessageSystem* msg, void**);

	// get and process region info if necessary.
	static void processRegionInfo(LLMessageSystem* msg);

	static const LLUUID& getLastInvoice() { return sRequestInvoice; }
	static void nextInvoice() { sRequestInvoice.generate(); }
	//static S32 getSerial() { return sRequestSerial; }
	//static void incrementSerial() { sRequestSerial++; }

	static LLPanelEstateInfo* getPanelEstate();
	static LLPanelEstateCovenant* getPanelCovenant();
	static LLPanelRegionTerrainInfo* getPanelRegionTerrain();
	static LLPanelRegionExperiences* getPanelExperiences();
	static LLPanelRegionGeneralInfo* getPanelGeneral();

	// from LLPanel
	void refresh() override;
	
	void requestRegionInfo();
	void requestMeshRezInfo();
	void enableTopButtons();
	void disableTopButtons();

private:
	
	LLFloaterRegionInfo(const LLSD& seed);
	~LLFloaterRegionInfo();


	
protected:
	void onTabSelected(const LLSD& param);
	void disableTabCtrls();
	void refreshFromRegion(LLViewerRegion* region);

	// member data
	LLTabContainer* mTab;
	typedef std::vector<LLPanelRegionInfo*> info_panels_t;
	info_panels_t mInfoPanels;
	//static S32 sRequestSerial;	// serial # of last EstateOwnerRequest
	static LLUUID sRequestInvoice;
};


// Base class for all region information panels.
class LLPanelRegionInfo : public LLPanel
{
public:
	LLPanelRegionInfo();
	
	void onBtnSet();
	void onChangeChildCtrl(LLUICtrl* ctrl);
	void onChangeAnything();
	static void onChangeText(LLLineEditor* caller, void* user_data);
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	virtual bool estateUpdate(LLMessageSystem* msg) { return true; }

	BOOL postBuild() override;
	virtual void updateChild(LLUICtrl* child_ctrl);
	
	void enableButton(const std::string& btn_name, BOOL enable = TRUE);
	void disableButton(const std::string& btn_name);
	
	void onClickManageTelehub();
	
protected:
	void initCtrl(const std::string& name);
	
	// Returns TRUE if update sent and apply button should be
	// disabled.
	virtual BOOL sendUpdate() { return TRUE; }
	
	typedef std::vector<std::string> strings_t;
	//typedef std::vector<U32> integers_t;
	void sendEstateOwnerMessage(
					 LLMessageSystem* msg,
					 const std::string& request,
					 const LLUUID& invoice,
					 const strings_t& strings);
	
	// member data
	LLHost mHost;
};

/////////////////////////////////////////////////////////////////////////////
// Actual panels start here
/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionGeneralInfo : public LLPanelRegionInfo
{
	
public:
	LLPanelRegionGeneralInfo()
		:	LLPanelRegionInfo(),
			mObjBonusFactor(0.f) {}
	~LLPanelRegionGeneralInfo() {}

	bool refreshFromRegion(LLViewerRegion* region) override;
	
	// LLPanel
	BOOL postBuild() override;
	
	void onBtnSet();
	void setObjBonusFactor(F32 object_bonus_factor) {mObjBonusFactor = object_bonus_factor;}

protected:
	BOOL sendUpdate() override;
	void onClickKick();
	void onKickCommit(const uuid_vec_t& ids);
	static void onClickKickAll(void* userdata);
	bool onKickAllCommit(const LLSD& notification, const LLSD& response);
	static void onClickMessage(void* userdata);
	bool onMessageCommit(const LLSD& notification, const LLSD& response);
	bool onChangeObjectBonus(const LLSD& notification, const LLSD& response);

	F32 mObjBonusFactor;

};

/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionDebugInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionDebugInfo()
		:	LLPanelRegionInfo(), mTargetAvatar() {}
	~LLPanelRegionDebugInfo() {}
	// LLPanel
	BOOL postBuild() override;

	bool refreshFromRegion(LLViewerRegion* region) override;
	
protected:
	BOOL sendUpdate() override;

	void onClickChooseAvatar();
	void callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);
	static void onClickReturn(void *);
	bool callbackReturn(const LLSD& notification, const LLSD& response);
	static void onClickTopColliders(void*);
	static void onClickTopScripts(void*);
	static void onClickRestart(void* data);
	bool callbackRestart(const LLSD& notification, const LLSD& response);
	static void onClickCancelRestart(void* data);
	static void onClickDebugConsole(void* data);
	
private:
	LLUUID mTargetAvatar;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionTerrainInfo : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelRegionTerrainInfo);

public:
	LLPanelRegionTerrainInfo()
		: LLPanelRegionInfo(),
		mConfirmedTextureHeights(false),
		mAskedTextureHeights(false) {}
	~LLPanelRegionTerrainInfo() {}

	BOOL postBuild() override;												// LLPanel

	bool refreshFromRegion(LLViewerRegion* region) override;					// refresh local settings from region update from simulator
	void setEnvControls(bool available);									// Whether environment settings are available for this region

	BOOL validateTextureSizes();
	BOOL validateTextureHeights();

	//static void onChangeAnything(LLUICtrl* ctrl, void* userData);			// callback for any change, to enable commit button

	BOOL sendUpdate() override;

	static void onClickDownloadRaw(void*);
	static void onClickUploadRaw(void*);
	static void onClickBakeTerrain(void*);
	bool callbackBakeTerrain(const LLSD& notification, const LLSD& response);
	bool callbackTextureHeights(const LLSD& notification, const LLSD& response);

private:
	bool mConfirmedTextureHeights;
	bool mAskedTextureHeights;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelEstateInfo : public LLPanelRegionInfo
{
public:
	static void initDispatch(LLDispatcher& dispatch);
	
	void onChangeFixedSun();
	void onChangeUseGlobalTime();
	void onChangeAccessOverride();
	
	void onClickEditSky();
	void onClickEditSkyHelp();	
	void onClickEditDayCycle();
	void onClickEditDayCycleHelp();

	void onClickAddAllowedAgent();
	void onClickRemoveAllowedAgent();
	void onClickAddAllowedGroup();
	void onClickRemoveAllowedGroup();
	void onClickAddBannedAgent();
	void onClickRemoveBannedAgent();
	void onClickAddEstateManager();
	void onClickRemoveEstateManager();
	void onClickKickUser();

	// Group picker callback is different, can't use core methods below
	bool addAllowedGroup(const LLSD& notification, const LLSD& response);
	void addAllowedGroup2(LLUUID id);

	// Core methods for all above add/remove button clicks
	static void accessAddCore(U32 operation_flag, const std::string& dialog_name);
	static bool accessAddCore2(const LLSD& notification, const LLSD& response);
	static void accessAddCore3(const uuid_vec_t& ids, void* data);

	static void accessRemoveCore(U32 operation_flag, const std::string& dialog_name, const std::string& list_ctrl_name);
	static bool accessRemoveCore2(const LLSD& notification, const LLSD& response);

	// used for both add and remove operations
	static bool accessCoreConfirm(const LLSD& notification, const LLSD& response);
	bool kickUserConfirm(const LLSD& notification, const LLSD& response);

	// Send the actual EstateOwnerRequest "estateaccessdelta" message
	static void sendEstateAccessDelta(U32 flags, const LLUUID& agent_id);

	void onKickUserCommit(const uuid_vec_t& ids);
	static void onClickMessageEstate(void* data);
	bool onMessageCommit(const LLSD& notification, const LLSD& response);
	
	LLPanelEstateInfo();
	~LLPanelEstateInfo() {}
	
	void updateControls(LLViewerRegion* region);
	
	static void updateEstateName(const std::string& name);
	static void updateEstateOwnerName(const std::string& name);

	bool refreshFromRegion(LLViewerRegion* region) override;
	bool estateUpdate(LLMessageSystem* msg) override;
	
	// LLPanel
	BOOL postBuild() override;
	void updateChild(LLUICtrl* child_ctrl) override;
	void refresh() override;

	void refreshFromEstate();
	
	static bool isLindenEstate();
	
	const std::string getOwnerName() const;
	void setOwnerName(const std::string& name);

protected:
	BOOL sendUpdate() override;
	// confirmation dialog callback
	bool callbackChangeLindenEstate(const LLSD& notification, const LLSD& response);

	void commitEstateAccess();
	void commitEstateManagers();
	
	void clearAccessLists();
	BOOL checkSunHourSlider(LLUICtrl* child_ctrl);

	U32 mEstateID;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelEstateCovenant : public LLPanelRegionInfo
{
public:
	LLPanelEstateCovenant();
	~LLPanelEstateCovenant() {}
	
	// LLPanel
	BOOL postBuild() override;
	void updateChild(LLUICtrl* child_ctrl) override;
	bool refreshFromRegion(LLViewerRegion* region) override;
	bool estateUpdate(LLMessageSystem* msg) override;

	// LLView overrides
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg) override;
	static bool confirmChangeCovenantCallback(const LLSD& notification, const LLSD& response);
	static void resetCovenantID(void* userdata);
	static bool confirmResetCovenantCallback(const LLSD& notification, const LLSD& response);
	void sendChangeCovenantID(const LLUUID &asset_id);
	void loadInvItem(LLInventoryItem *itemp);
	static void onLoadComplete(LLVFS *vfs,
							   const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);

	// Accessor functions
	static void updateCovenantText(const std::string& string, const LLUUID& asset_id);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);

	const LLUUID& getCovenantID() const { return mCovenantID; }
	void setCovenantID(const LLUUID& id) { mCovenantID = id; }
	std::string getEstateName() const;
	void setEstateName(const std::string& name);
	std::string getOwnerName() const;
	void setOwnerName(const std::string& name);
	void setCovenantTextEditor(const std::string& text);

	typedef enum e_asset_status
	{
		ASSET_ERROR,
		ASSET_UNLOADED,
		ASSET_LOADING,
		ASSET_LOADED
	} EAssetStatus;

protected:
	BOOL sendUpdate() override;
	LLTextBox*				mEstateNameText;
	LLTextBox*				mEstateOwnerText;
	LLTextBox*				mLastModifiedText;
	// CovenantID from sim
	LLUUID					mCovenantID;
	LLViewerTextEditor*		mEditor;
	EAssetStatus			mAssetStatus;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelEnvironmentInfo : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelEnvironmentInfo);

public:
	LLPanelEnvironmentInfo();

	// LLPanel
	/*virtual*/ BOOL postBuild() override;
	/*virtual*/ void onOpen(const LLSD& key) override;

	// LLView
	/*virtual*/ void onVisibilityChange(BOOL new_visibility) override;

	// LLPanelRegionInfo
	/*virtual*/ bool refreshFromRegion(LLViewerRegion* region) override;

private:
	void refresh() override;
	void setControlsEnabled(bool enabled);
	void setApplyProgress(bool started);
	void setDirty(bool dirty);

	void sendRegionSunUpdate();
	void fixEstateSun();

	void populateWaterPresetsList();
	void populateSkyPresetsList();
	void populateDayCyclesList();

	bool getSelectedWaterParams(LLSD& water_params);
	bool getSelectedSkyParams(LLSD& sky_params, std::string& preset_name);
	bool getSelectedDayCycleParams(LLSD& day_cycle, LLSD& sky_map, short& scope);

	void onSwitchRegionSettings();
	void onSwitchDayCycle();

	void onSelectWaterPreset();
	void onSelectSkyPreset();
	void onSelectDayCycle();

	void onBtnApply();
	void onBtnCancel();

	void onRegionSettingschange();
	void onRegionSettingsApplied(bool ok);

	/// New environment settings that are being applied to the region.
	LLEnvironmentSettings	mNewRegionSettings;

	bool			mEnableEditing;

	LLRadioGroup*	mRegionSettingsRadioGroup;
	LLRadioGroup*	mDayCycleSettingsRadioGroup;

	LLComboBox*		mWaterPresetCombo;
	LLComboBox*		mSkyPresetCombo;
	LLComboBox*		mDayCyclePresetCombo;
};

class LLPanelRegionExperiences : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelEnvironmentInfo);

public:
	LLPanelRegionExperiences()
		: mTrusted(nullptr)
		, mAllowed(nullptr)
		, mBlocked(nullptr)
	{}
	/*virtual*/ BOOL postBuild() override;
	BOOL sendUpdate() override;
	
	static bool experienceCoreConfirm(const LLSD& notification, const LLSD& response);
	static void sendEstateExperienceDelta(U32 flags, const LLUUID& agent_id);

	static void infoCallback(LLHandle<LLPanelRegionExperiences> handle, const LLSD& content);
	bool refreshFromRegion(LLViewerRegion* region) override;
	void sendPurchaseRequest()const;
	void processResponse( const LLSD& content );
private:
	void refreshRegionExperiences();

    static std::string regionCapabilityQuery(LLViewerRegion* region, const std::string &cap);

	LLPanelExperienceListEditor* setupList(const char* control_name, U32 add_id, U32 remove_id);
	static LLSD addIds( LLPanelExperienceListEditor* panel );

	void itemChanged(U32 event_type, const LLUUID& id);

	LLPanelExperienceListEditor* mTrusted;
	LLPanelExperienceListEditor* mAllowed;
	LLPanelExperienceListEditor* mBlocked;
	LLUUID mDefaultExperience;
};

#endif
