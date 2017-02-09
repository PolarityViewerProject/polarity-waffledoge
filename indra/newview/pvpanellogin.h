/** 
 * @file pvpanellogin.h
 * @brief Login dialog and logo display
 *
 * Based on FSPanelLogin used in Firestorm Viewer
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
 * Portions Copyright (C)
 *  2002-2016 Phoenix-Firestorm Viewer
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
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */
// Original file: llpanellogin.h

#ifndef LL_PANELLOGIN_H
#define LL_PANELLOGIN_H

#include "llpanel.h"
#include "llpointer.h"			// LLPointer<>
#include "llmediactrl.h"	// LLMediaCtrlObserver

class LLLineEditor;
class LLSLURL;
class LLCredential;

class LLPanelLogin:	
	public LLPanel,
	public LLViewerMediaObserver
{
	LOG_CLASS(LLPanelLogin);
public:
	LLPanelLogin(const LLRect &rect,
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	virtual void setFocus( BOOL b );

	static void show(const LLRect &rect,
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	static void setFields(LLPointer<LLCredential> credential, bool from_startup = false);

	static void getFields(LLPointer<LLCredential>& credential, BOOL& remember);

	static BOOL areCredentialFieldsDirty();
	static void setLocation(const LLSLURL& slurl);
	static void autologinToLocation(const LLSLURL& slurl);
	
	/// Call when preferences that control visibility may have changed
	static void updateLocationSelectorsVisibility();

	static void closePanel();

	void setSiteIsAlive( bool alive );

	void showLoginWidgets();

	static void loadLoginPage();	
	static void giveFocus();
	static void setAlwaysRefresh(bool refresh); 
	
	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);
	static void updateServer();  // update the combo box, change the login page to the new server, clear the combo

	/// to be called from LLStartUp::setStartSLURL
	static void onUpdateStartSLURL(const LLSLURL& new_start_slurl);

	// called from prefs when initializing panel
	static bool getShowFavorites();

	static void clearPassword() { sPassword.clear(); }

private:
	void addFavoritesToStartLocation();
	void addUsersToCombo(BOOL show_server);
	void onSelectUser();
#if SETTINGS_PRESETS
	void onModeChange(const LLSD& original_value, const LLSD& new_value);
	void onModeChangeConfirm(const LLSD& original_value, const LLSD& new_value, const LLSD& notification, const LLSD& response);
#endif
	void onSelectServer();
	void onLocationSLURL();
	void onUsernameTextChanged();

public:
	static void onClickConnect(void*);
	static void doLoginButtonLockUnlock();
private:
	static void onClickNewAccount(void*);
	static void onClickVersion(void*);
	static void onClickForgotPassword(void*);
	static void onClickHelp(void*);
	static void onPassKey(LLLineEditor* caller, void* user_data);
	static void updateServerCombo();
	static void onClickRemove(void*);
	static void onRemoveCallback(const LLSD& notification, const LLSD& response);
	static bool sLoginButtonEnabled;
#if LOGIN_MGR_HELP
	static void onClickGridMgrHelp(void*);
#endif
	static void gridListChanged(bool success);
	static std::string credentialName();

private:
	void updateLoginButtons();

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	BOOL            mPasswordModified;
	bool			mShowFavorites;

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;

	unsigned int mUsernameLength;
	unsigned int mPasswordLength;
	unsigned int mLocationLength;

	std::string		mPreviousUsername;
	static std::string	sPassword;
};

#endif //LL_PANELLOGIN_H
