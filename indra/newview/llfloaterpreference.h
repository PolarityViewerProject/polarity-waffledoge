/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llconversationlog.h"

class LLConversationLogObserver;
class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSD;
class LLTextBox;

typedef std::map<std::string, std::string> notifications_map;
// <polarity> LookAt Logic
bool callbackcheckAllowedLookAt(const LLSD& notification, const LLSD& response);
// </polarity>

// Unused
#ifdef OLD_GRAPHIC_SETTINGS
typedef enum
	{
		GS_LOW_GRAPHICS,
		GS_MID_GRAPHICS,
		GS_HIGH_GRAPHICS,
		GS_ULTRA_GRAPHICS
		
	} EGraphicsSettings;
#endif

// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver, public LLConversationLogObserver
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);
	/*virtual*/ void changed();
	/*virtual*/ void changed(const LLUUID& session_id, U32 mask) {};

	// <polarity> code deduplication
	void updateAALabel();
	void updateMemorySlider(const bool& set_default = false);
	void resetTextureMemorySlider();

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email);

	// refresh all the graphics preferences menus
	static void refreshEnabledGraphics();
	
	// translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
	static void initDoNotDisturbResponse();

	// update Show Favorites checkbox
	static void updateShowFavoritesCheckbox(bool val);

	void processProperties( void* pData, EAvatarProcessorType type );
	void processProfileProperties(const LLAvatarData* pAvatarData );
	void storeAvatarProperties( const LLAvatarData* pAvatarData );
	void saveAvatarProperties( void );
	void selectPrivacyPanel();
	void selectChatPanel();

protected:	
	void		onBtnOK(const LLSD& userdata);
	void		onBtnCancel(const LLSD& userdata);
	void		onBtnApply();

	void		onClickClearCache();			// Clear viewer texture cache, vfs, and VO cache on next startup
	void		onClickBrowserClearCache();		// Clear web history and caches as well as viewer caches above
	void		onLanguageChange();
	void		onNotificationsChange(const std::string& OptionName);
	void		onNameTagOpacityChange(const LLSD& newvalue);

	// set value of "DoNotDisturbResponseChanged" in account settings depending on whether do not disturb response
	// string differs from default after user changes.
	void onDoNotDisturbResponseChanged();
	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	// callback for defaults
	void setHardwareDefaults();
	// callback for when client turns on shaders
	void onVertexShaderEnable();
	// callback for when client turns on impostors
	void onAvatarImpostorsEnable();

	// callback for commit in the "Single click on land" and "Double click on land" comboboxes.
	void onClickActionChange();
	// updates click/double-click action settings depending on controls values
	void updateClickActionSettings();
	// updates click/double-click action controls depending on values from settings.xml
	void updateClickActionControls();

public:
	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.	
	void saveSettings();
	// <polarity> LookAt Logic
	bool confirmNosyLookAt();
	// </polarity>
private:

	void setCacheLocation(const LLStringExplicit& location);

	void onClickSetCache();
	void onClickResetCache();
	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
	void onSelectSkin();
	void onClickSetKey();
public:
	void setKey(KEY key);
private:
	void onClickSetMiddleMouse();
	void onClickSetSounds();
	void onClickEnablePopup();
	void onClickDisablePopup();	
	void resetAllIgnored();
	void setAllIgnored();
	void onClickLogPath();
	bool moveTranscriptsAndLog();
	void enableHistory();
	void setPersonalInfo(const std::string& visibility, bool im_via_email);
	void refreshEnabledState();
	void disableUnavailableSettings();
	// <polarity> unused
	//void onCommitWindowedMode();
public:
	void refresh();	// Refresh enable/disable
private:
	// if the quality radio buttons are changed
	void onChangeQuality(const LLSD& data);
	
	void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
	void refreshUI();
	void updateMaxNonImpostors();
	void setMaxNonImpostorsText(U32 value, LLTextBox* text_box);
public:
	// <Black Dragon:NiranV> Debug Arrays
	static void onCommitX(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitY(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitZ(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitXd(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitYd(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitZd(LLUICtrl* ctrl, const LLSD& param);
	// <Black Dragon:NiranV> Vector4
	static void onCommitVec4X(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitVec4Y(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitVec4Z(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitVec4W(LLUICtrl* ctrl, const LLSD& param);

	// <Black Dragon:NiranV> Revert to Default
	void resetToDefault(LLUICtrl* ctrl, const LLSD& param);
private:
	// <Black Dragon:NiranV> Catznip's Borderless Window Mode
	void toggleFullscreenWindow();
	void applyGraphicsOptions(); // <polarity/>
	// <Black Dragon:NiranV> Refresh all controls
	void refreshGraphicControls();
	// </Black Dragon:NiranV>

	void onCommitParcelMediaAutoPlayEnable();
	void onCommitMediaEnabled();
	void onCommitMusicEnabled();
	void applyResolution();
	void onChangeMaturity();
	void onClickBlockList();
	void onClickProxySettings();
	void onClickTranslationSettings();
	void onClickPermsDefault();
	void onClickAutoReplace();
	void onClickSpellChecker();
	//void onClickAdvanced(); // <polarity> unused
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);
	void onLogChatHistorySaved();	
	void buildPopupLists();

public:
	static void refreshSkin(void* data);
private:
	void selectPanel(const LLSD& name);

private:

	void onDeleteTranscripts();
	void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
	void updateDeleteTranscriptsButton();
	void updateMaxComplexity();

	static std::string sSkin;
	notifications_map mNotificationOptions;
	bool mClickActionDirty; ///< Set to true when the click/double-click options get changed by user.
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	bool mLanguageChanged;
	bool mAvatarDataInitialized;
	std::string mPriorInstantMessageLogPath;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
	
	LLAvatarData mAvatarProperties;

	LOG_CLASS(LLFloaterPreference);
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ BOOL postBuild();
	
	virtual ~LLPanelPreference();

	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);
	virtual void setHardwareDefaults(){};

	// Disables "Allow Media to auto play" check box only when both
	// "Streaming Music" and "Media" are unchecked. Otherwise enables it.
	void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();
	
	class Updater;

protected:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

private:
	//for "Only friends and groups can call or IM me"
	static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
	//for "Show my Favorite Landmarks at Login"
	static void handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value);

	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;

	Updater* mBandWidthUpdater;
	LOG_CLASS(LLPanelPreference);
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
	BOOL postBuild();
	void draw();
	void cancel();
	void saveSettings();
	void resetDirtyChilds();
	void setHardwareDefaults();
protected:
	bool hasDirtyChilds();
	LOG_CLASS(LLPanelPreferenceGraphics);
};

// <polarity> We won't use this duplicated class/floater. Note: at some point we're going to need our own file for this.
#if 0
class LLFloaterPreferenceGraphicsAdvanced : public LLFloater
{
  public: 
	LLFloaterPreferenceGraphicsAdvanced(const LLSD& key);
	~LLFloaterPreferenceGraphicsAdvanced();
	void onOpen(const LLSD& key);
	void onClickCloseBtn(bool app_quitting);
	void disableUnavailableSettings();
	void refreshEnabledGraphics();
	void refreshEnabledState();
	void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
	void updateMaxNonImpostors();
	void setMaxNonImpostorsText(U32 value, LLTextBox* text_box);
	void updateMaxComplexity();
	void setMaxComplexityText(U32 value, LLTextBox* text_box);
	static void setIndirectControls();
	static void setIndirectMaxNonImpostors();
	static void setIndirectMaxArc();
	void refresh();
	// callback for when client turns on shaders
	void onVertexShaderEnable();
	LOG_CLASS(LLFloaterPreferenceGraphicsAdvanced);
};
#endif

class LLAvatarComplexityControls
{
  public: 
	static void updateMax(LLSliderCtrl* slider, LLTextBox* value_label);
	static void setText(U32 value, LLTextBox* text_box);
	static void setIndirectControls();
	static void setIndirectMaxNonImpostors();
	static void setIndirectMaxArc();
	LOG_CLASS(LLAvatarComplexityControls);
};

class LLFloaterPreferenceProxy : public LLFloater
{
public: 
	LLFloaterPreferenceProxy(const LLSD& key);
	~LLFloaterPreferenceProxy();

	/// show off our menu
	static void show();
	void cancel();
	
protected:
	BOOL postBuild();
	void onOpen(const LLSD& key);
	void onClose(bool app_quitting);
	void saveSettings();
	void onBtnOk();
	void onBtnCancel();
	void onClickCloseBtn(bool app_quitting = false);

	void onChangeSocksSettings();

private:
	
	bool mSocksSettingsDirty;
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
	LOG_CLASS(LLFloaterPreferenceProxy);
};


#endif  // LL_LLPREFERENCEFLOATER_H
