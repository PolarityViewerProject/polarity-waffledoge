// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file pvpanelLogin.cpp
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
// Original file: llpanellogin.cpp

#include "llviewerprecompiledheaders.h"

#include "pvpanellogin.h"
#include "lllayoutstack.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfloaterreg.h"
//#include "llfontgl.h"
#include "llmd5.h"
//#include "v4color.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"		// for secondlife:///app/login/
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llfloaterpreference.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llsecapi.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
//#include "lluiconstants.h"
#include "llslurl.h"
#include "llversioninfo.h"
#include "llviewerhelp.h"
#include "llviewertexturelist.h"
#if SETTINGS_PRESET
#include "llviewermenu.h"			// for handle_preferences()
#endif
#include "llviewernetwork.h"
#include "llviewerwindow.h"			// to link into child list
#include "lluictrlfactory.h"
#include "llweb.h"
#include "llmediactrl.h"
#include "llrootview.h"

#include "llfloatertos.h"
#include "lltrans.h"
#include "llglheaders.h"
#include "llpanelloginlistener.h"

#include "pvdata.h"

#if LL_WINDOWS
#pragma warning(disable: 4355)      // 'this' used in initializer list
#endif  // LL_WINDOWS

#include "llsdserialize.h"

const S32 MAX_PASSWORD_SL = 16;
const S32 MAX_PASSWORD_OPENSIM = 255;

LLPanelLogin *LLPanelLogin::sInstance = NULL;
BOOL LLPanelLogin::sCapslockDidNotification = FALSE;
std::string LLPanelLogin::sPassword = "";

bool LLPanelLogin::sLoginButtonEnabled = false;

// Helper for converting a user name into the canonical "Firstname Lastname" form.
// For new accounts without a last name "Resident" is added as a last name.
static std::string canonicalize_username(const std::string& name);

class LLLoginLocationAutoHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLLoginLocationAutoHandler() : LLCommandHandler("location_login", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{	
		if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		{
			if ( tokens.size() == 0 || tokens.size() > 4 ) 
				return false;

			// unescape is important - uris with spaces are escaped in this code path
			// (e.g. space -> %20) and the code to log into a region doesn't support that.
			const std::string region = LLURI::unescape( tokens[0].asString() );

			// just region name as payload 
			if ( tokens.size() == 1 )
			{
				// region name only - slurl will end up as center of region
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x coord as payload 
			if ( tokens.size() == 2 )
			{
				// invalid to only specify region and x coordinate
				// slurl code will revert to same as region only, so do this anyway
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x/y coord as payload 
			if ( tokens.size() == 3 )
			{
				// region and x/y specified - default z to 0
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;

				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;

				const LLVector3 location(xpos, ypos, 0.0f);
				LLSLURL slurl(region, location);

				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x/y/z coord as payload 
			if ( tokens.size() == 4 )
			{
				// region and x/y/z specified - ok
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;

				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;

				F32 zpos;
				codec.clear();
				codec.str(tokens[3].asString());
				codec >> zpos;

				const LLVector3 location(xpos, ypos, zpos);
				LLSLURL slurl(region, location);

				LLPanelLogin::autologinToLocation(slurl);
			};
		}	
		return true;
	}
};
LLLoginLocationAutoHandler gLoginLocationAutoHandler;

//---------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------
LLPanelLogin::LLPanelLogin(const LLRect &rect,
	void(*callback)(S32 option, void* user_data),
	void *cb_data)
	: LLPanel(),
	mCallback(callback),
	mCallbackData(cb_data),
	mUsernameLength(0),
	mPasswordLength(0),
	mLocationLength(0),
	mShowFavorites(false)
{
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	mPasswordModified = FALSE;
	LLPanelLogin::sInstance = this;

	LLView* login_holder = gViewerWindow->getLoginPanelHolder();
	if (login_holder)
	{
		login_holder->addChild(this);
	}

	buildFromFile( "panel_login.xml");

	reshape(rect.getWidth(), rect.getHeight());
	
#if SETTINGS_PRESETS
	LLUICtrl& mode_combo = getChildRef<LLUICtrl>("mode_combo");
	if(mode_combo)
	{
		mode_combo.setValue(gSavedSettings.getString("SessionSettingsFile"));
		mode_combo.setCommitCallback(boost::bind(&LLPanelLogin::onModeChange, this, getChild<LLUICtrl>("mode_combo")->getValue(), _2));
	}
#endif

	LLLineEditor* password_edit(getChild<LLLineEditor>("password_edit"));
	password_edit->setKeystrokeCallback(onPassKey, this);
	// STEAM-14: When user presses Enter with this field in focus, initiate login
	password_edit->setCommitCallback(boost::bind(&LLPanelLogin::onClickConnect, this));

	// change z sort of clickable text to be behind buttons
	// sendChildToBack(getChildView("forgot_password_text"));

	LLComboBox* favorites_combo = getChild<LLComboBox>("start_location_combo");
	updateLocationSelectorsVisibility(); // separate so that it can be called from preferences
	favorites_combo->setFocusLostCallback(boost::bind(&LLPanelLogin::onLocationSLURL, this));
	
	LLComboBox* server_choice_combo = getChild<LLComboBox>("server_combo");
	server_choice_combo->setCommitCallback(boost::bind(&LLPanelLogin::onSelectServer, this));
	
	updateServer();
	if(LLStartUp::getStartSLURL().getType() != LLSLURL::LOCATION)
	{
		LLSLURL slurl(gSavedSettings.getString("LoginLocation"));
		LLStartUp::setStartSLURL(slurl);
	}

	// LLTextBox* create_new_account_text = getChild<LLTextBox>("create_new_account_text");
	// create_new_account_text->setClickedCallback(onClickNewAccount, NULL);

#if LOGIN_MGR_HELP
	LLTextBox* grid_mgr_help_text = getChild<LLTextBox>("grid_login_text");
	grid_mgr_help_text->setClickedCallback(onClickGridMgrHelp, NULL);
#endif

	LLSLURL start_slurl(LLStartUp::getStartSLURL());
	if ( !start_slurl.isSpatial() ) // has a start been established by the command line or NextLoginLocation ? 
	{
		// no, so get the preference setting
		std::string defaultStartLocation = gSavedSettings.getString("LoginLocation");
		LL_INFOS("AppInit")<<"default LoginLocation '"<<defaultStartLocation<<"'"<<LL_ENDL;
		LLSLURL defaultStart(defaultStartLocation);
		if ( defaultStart.isSpatial() )
		{
			LLStartUp::setStartSLURL(defaultStart);
		}
		else
		{
			LL_INFOS("AppInit")<<"no valid LoginLocation, using home"<<LL_ENDL;
			LLSLURL homeStart(LLSLURL::SIM_LOCATION_HOME);
			LLStartUp::setStartSLURL(homeStart);
		}
	}
	else
	{
		LLPanelLogin::onUpdateStartSLURL(start_slurl); // updates grid if needed
	}
	
	childSetAction("remove_user_btn", onClickRemove, this);
	childSetAction("connect_btn", onClickConnect, this);

	//@todo add conditional focus on connect button when credentials are filled
	// so that "enter to login" works again
	// getChild<LLPanel>("login")->setDefaultBtn(findChild<LLButton>("connect_btn"));
	// getChild<LLPanel>("start_location_panel")->setDefaultBtn(findChild<LLButton>("connect_btn"));

	//std::string channel = LLVersionInfo::getChannel();
	//std::string version = llformat("%s (%d)",
	//							   LLVersionInfo::getShortVersion().c_str(),
	//							   LLVersionInfo::getBuild());
	
	LLTextBox* forgot_password_text = getChild<LLTextBox>("forgot_password_text");
	if (forgot_password_text)
	{
		forgot_password_text->setClickedCallback(onClickForgotPassword, NULL);
	}
	
	loadLoginPage();

	LLComboBox* username_combo(getChild<LLComboBox>("username_combo"));
	username_combo->setTextChangedCallback(boost::bind(&LLPanelLogin::onUsernameTextChanged, this));
	username_combo->setCommitCallback(boost::bind(&LLPanelLogin::onSelectUser, this));
	username_combo->setFocusLostCallback(boost::bind(&LLPanelLogin::onSelectUser, this));
	mPreviousUsername = username_combo->getValue().asString();

	updateLoginButtons();

}

void LLPanelLogin::addFavoritesToStartLocation()
{
	mShowFavorites = false;
	// Clear the combo.
	LLComboBox* combo = getChild<LLComboBox>("start_location_combo");
	if (!combo) return;
	int num_items = combo->getItemCount();
	for (int i = num_items - 1; i > 2; i--)
	{
		combo->remove(i);
	}

	// Load favorites into the combo.
	std::string user_defined_name = getChild<LLComboBox>("username_combo")->getSimple();
	std::string canonical_user_name = canonicalize_username(user_defined_name);
	U32 resident_pos = canonical_user_name.find("Resident");
	if (resident_pos > 0)
	{
		canonical_user_name = canonical_user_name.substr(0, resident_pos - 1);
	}
	std::string current_grid = getChild<LLComboBox>("server_combo")->getSimple();
	std::string current_user = canonical_user_name + " @ " + current_grid;
	LL_DEBUGS("Favorites") << "Current user: \"" << current_user << "\"" << LL_ENDL;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	mUsernameLength = current_user.length();
	updateLoginButtons();

	LLSD fav_llsd;
	llifstream file;
	file.open(filename.c_str());
	if (!file.is_open())
	{
		return;
	}
	LLSDSerialize::fromXML(fav_llsd, file);

	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		// The account name in stored_favorites.xml has Resident last name even if user has
		// a single word account name, so it can be compared case-insensitive with the
		// user defined "firstname lastname".
		S32 res = LLStringUtil::compareInsensitive(current_user, iter->first);
		if (res != 0)
		{
			LL_DEBUGS() << "Skipping favorites for " << iter->first << LL_ENDL;
			continue;
		}

		combo->addSeparator();
		LL_DEBUGS() << "Loading favorites for " << iter->first << LL_ENDL;
		LLSD user_llsd = iter->second;
		for (LLSD::array_const_iterator iter1 = user_llsd.beginArray();
			iter1 != user_llsd.endArray(); ++iter1)
		{
			std::string label = (*iter1)["name"].asString();
			std::string value = (*iter1)["slurl"].asString();
			if(label != "" && value != "")
			{
				mShowFavorites = true;
				combo->add(label, value);
			}
		}
		break;
	}

	LLFloaterPreference::updateShowFavoritesCheckbox(mShowFavorites);
}

LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// Controls having keyboard focus by default
	// must reset it on destroy. (EXT-2748)
	gFocusMgr.setDefaultKeyboardFocus(NULL);
}

// virtual
void LLPanelLogin::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			LLPanelLogin::giveFocus();
		}
		else
		{
			LLPanel::setFocus(b);
		}
	}
}

// static
void LLPanelLogin::giveFocus()
{
	if( sInstance )
	{
		// Grab focus and move cursor to first blank input field
		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		std::string pass = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();

		BOOL have_username = !username.empty();
		BOOL have_pass = !pass.empty();

		LLLineEditor* edit = NULL;
		LLComboBox* combo = NULL;
		if (have_username && !have_pass)
		{
			// User saved his name but not his password.  Move
			// focus to password field.
			edit = sInstance->getChild<LLLineEditor>("password_edit");
		}
		else
		{
			// User doesn't have a name, so start there.
			combo = sInstance->getChild<LLComboBox>("username_combo");
		}

		if (edit)
		{
			edit->setFocus(TRUE);
			edit->selectAll();
		}
		else if (combo)
		{
			combo->setFocus(TRUE);
			combo->focusEditor();
		}
	}
}

// static
void LLPanelLogin::showLoginWidgets()
{
	if (sInstance)
	{
		// *NOTE: Mani - This may or may not be obselete code.
		// It seems to be part of the defunct? reg-in-client project.
		sInstance->getChildView("login_widgets")->setVisible( true);
		LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");

		// *TODO: Append all the usual login parameters, like first_login=Y etc.
		std::string splash_screen_url = LLGridManager::getInstance()->getLoginPage();
		web_browser->navigateTo( splash_screen_url, HTTP_CONTENT_TEXT_HTML );
		LLUICtrl* username_combo = sInstance->getChild<LLUICtrl>("username_combo");
		username_combo->setFocus(TRUE);
	}
}

// static
void LLPanelLogin::show(const LLRect &rect,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
    if (!LLPanelLogin::sInstance)
    {
        new LLPanelLogin(rect, callback, callback_data);
    }

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		sInstance->setFocus(TRUE);
	}

	// Make sure that focus always goes here (and use the latest sInstance that was just created)
	gFocusMgr.setDefaultKeyboardFocus(sInstance);
}

// static
void LLPanelLogin::setFields(LLPointer<LLCredential> credential, bool from_startup /* = false*/)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted setFields with no login view shown" << LL_ENDL;
		return;
	}
	LL_INFOS("Credentials") << "Setting login fields to " << *credential << LL_ENDL;

	std::string login_id;
	LLSD identifier = credential->getIdentifier();
	if (identifier["type"].asString() == "agent")
	{
		std::string firstname = identifier["first_name"].asString();
		std::string lastname = identifier["last_name"].asString();
		login_id = firstname;
		if (!lastname.empty() && lastname != "Resident")
		{
			// support traditional First Last name SLURLs
			login_id += " ";
			login_id += lastname;
		}
	}

	const std::string cred_name = credential->getCredentialName();
	LLComboBox* username_combo = sInstance->getChild<LLComboBox>("username_combo");
	if (!username_combo->selectByValue(cred_name))
	{
		username_combo->setTextEntry(login_id);
		sInstance->mPasswordModified = TRUE;
		sInstance->getChild<LLButton>("remove_user_btn")->setEnabled(FALSE);
	}
	sInstance->mPreviousUsername = username_combo->getValue().asString();
	sInstance->addFavoritesToStartLocation();
	// if the password exists in the credential, set the password field with
	// a filler to get some stars
	LLSD authenticator = credential->getAuthenticator();
	LL_INFOS("Credentials") << "Setting authenticator field " << authenticator["type"].asString() << LL_ENDL;
	bool remember;
	if(authenticator.isMap() && 
	   authenticator.has("secret") && 
	   (authenticator["secret"].asString().size() > 0))
	{
		
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD_SL characters so we get a 
		// nice row of asterisks.
		const std::string filler("123456789!123456");
		sInstance->getChild<LLLineEditor>("password_edit")->setText(filler);
		sInstance->mPasswordLength = filler.length();
		sInstance->updateLoginButtons();
		remember = true;

		// We run into this case, if a user tries to login with a newly entered password
		// and the login fails with some error (except wrong credentials). In that case,
		// LLStartUp will bring us back here and provide us with the credentials that were
		// used for the login. In case we don't have credentials already saved for this
		// username OR the password hash is different, we have to set the password back
		// to the one the user entered or we will end up trying to login with the filler
		// as password!
		if (from_startup)
		{
			LLPointer<LLCredential> stored_credential = gSecAPIHandler->loadCredential(cred_name);
			if (stored_credential->getAuthenticator().size() == 0 ||
				(stored_credential->getAuthenticator().has("secret") && stored_credential->getAuthenticator()["secret"].asString() != authenticator["secret"].asString()))
			{
				sInstance->getChild<LLLineEditor>("password_edit")->setText(sPassword);
				sInstance->mPasswordLength = sPassword.length();
				sInstance->updateLoginButtons();
				sInstance->mPasswordModified = TRUE;
			}
		}
	}
	else
	{
		sInstance->getChild<LLLineEditor>("password_edit")->clear();
		sInstance->mPasswordLength = 0;
		sInstance->updateLoginButtons();
		remember = false;
	}
	if (from_startup)
	{
		remember = gSavedSettings.getBOOL("RememberPassword");
	}
	sInstance->getChild<LLUICtrl>("remember_check")->setValue(remember);
}


// static
void LLPanelLogin::getFields(LLPointer<LLCredential>& credential,
							 BOOL& remember)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getFields with no login view shown" << LL_ENDL;
		return;
	}
	
	// load the credential so we can pass back the stored password or hash if the user did
	// not modify the password field.
	credential = gSecAPIHandler->loadCredential(credentialName());

	LLSD identifier = LLSD::emptyMap();
	LLSD authenticator = LLSD::emptyMap();
	
	if(credential.notNull())
	{
		authenticator = credential->getAuthenticator();
	}

	std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
	LLStringUtil::trim(username);
	size_t arobase = username.find('@');
	if (arobase != std::string::npos)
	{
		username = username.substr(0, arobase);
	}
	std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
	sPassword = password;

	LL_INFOS("Credentials", "Authentication") << "retrieving username:" << username << LL_ENDL;
	// determine if the username is a first/last form or not.
	size_t separator_index = username.find_first_of(' ');
	if (separator_index == username.npos
		&& !LLGridManager::getInstance()->isSystemGrid())
	{
		LL_INFOS("Credentials", "Authentication") << "account: " << username << LL_ENDL;
		// single username, so this is a 'clear' identifier
		identifier["type"] = CRED_IDENTIFIER_TYPE_ACCOUNT;
		identifier["account_name"] = username;
		
		if (LLPanelLogin::sInstance->mPasswordModified)
		{
			authenticator = LLSD::emptyMap();
			// password is plaintext
			authenticator["type"] = CRED_AUTHENTICATOR_TYPE_CLEAR;
			authenticator["secret"] = password;
		}
	}
	else
	{
		// Be lenient in terms of what separators we allow for two-word names
		// and allow legacy users to login with firstname.lastname
#ifdef OPENSIM
		if (LLGridManager::getInstance()->isInSecondLife())
		{
			separator_index = username.find_first_of(" ._");
		}
		else
		{
			separator_index = username.find_first_of(" .");
		}
#else
		separator_index = username.find_first_of(" ._");
#endif
		std::string first = username.substr(0, separator_index);
		std::string last;
		if (separator_index != username.npos)
		{
			last = username.substr(separator_index+1, username.npos);
		LLStringUtil::trim(last);
		}
		else
		{
			// ...on Linden grids, single username users as considered to have
			// last name "Resident"
			// *TODO: Make login.cgi support "account_name" like above
			last = "Resident";
		}
		
		if (last.find_first_of(' ') == last.npos)
		{
			LL_INFOS("Credentials", "Authentication") << "agent: " << username << LL_ENDL;
			// traditional firstname / lastname
			identifier["type"] = CRED_IDENTIFIER_TYPE_AGENT;
			identifier["first_name"] = first;
			identifier["last_name"] = last;
		
			if (LLPanelLogin::sInstance->mPasswordModified)
			{
				authenticator = LLSD::emptyMap();
				authenticator["type"] = CRED_AUTHENTICATOR_TYPE_HASH;
				authenticator["algorithm"] = "md5";
				LLMD5 pass((const U8 *)password.c_str());
				char md5pass[33];               /* Flawfinder: ignore */
				pass.hex_digest(md5pass);
				authenticator["secret"] = md5pass;
			}
		}
	}
	credential = gSecAPIHandler->createCredential(credentialName(), identifier, authenticator);
	remember = sInstance->getChild<LLUICtrl>("remember_check")->getValue();
}


// static
BOOL LLPanelLogin::areCredentialFieldsDirty()
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getServer with no login view shown" << LL_ENDL;
	}
	else
	{
		LLComboBox* combo = sInstance->getChild<LLComboBox>("username_combo");
		if(combo && combo->isDirty())
		{
			return true;
		}
		LLLineEditor* ctrl = sInstance->getChild<LLLineEditor>("password_edit");
		if(ctrl && ctrl->isDirty()) 
		{
			return true;
		}
	}
	return false;	
}


// static
void LLPanelLogin::updateLocationSelectorsVisibility()
{
	if (sInstance) 
	{
		BOOL show_start = gSavedSettings.getBOOL("ShowStartLocation");
		sInstance->getChild<LLComboBox>("start_location_combo")->setVisible(show_start);

		BOOL show_server = gSavedSettings.getBOOL("ForceShowGrid");
		sInstance->getChild<LLComboBox>("server_combo")->setVisible(show_server);
		sInstance->addUsersToCombo(show_server);
	}	
}

// static - called from LLStartUp::setStartSLURL
void LLPanelLogin::onUpdateStartSLURL(const LLSLURL& new_start_slurl)
{
	if (!sInstance) return;

	LL_DEBUGS("AppInit")<<new_start_slurl.asString()<<LL_ENDL;

	LLComboBox* location_combo = sInstance->getChild<LLComboBox>("start_location_combo");
	/*
	 * Determine whether or not the new_start_slurl modifies the grid.
	 *
	 * Note that some forms that could be in the slurl are grid-agnostic.,
	 * such as "home".  Other forms, such as
	 * https://grid.example.com/region/Party%20Town/20/30/5 
	 * specify a particular grid; in those cases we want to change the grid
	 * and the grid selector to match the new value.
	 */
	enum LLSLURL::SLURL_TYPE new_slurl_type = new_start_slurl.getType();
	switch ( new_slurl_type )
	{
	case LLSLURL::LOCATION:
	  {
		std::string slurl_grid = LLGridManager::getInstance()->getGrid(new_start_slurl.getGrid());
		if ( ! slurl_grid.empty() ) // is that a valid grid?
		{
			if ( slurl_grid != LLGridManager::getInstance()->getGrid() ) // new grid?
			{
				// the slurl changes the grid, so update everything to match
				LLGridManager::getInstance()->setGridChoice(slurl_grid);

				// update the grid selector to match the slurl
				LLComboBox* server_combo = sInstance->getChild<LLComboBox>("server_combo");
				std::string server_label(LLGridManager::getInstance()->getGridLabel(slurl_grid));
				server_combo->setSimple(server_label);

				updateServer(); // to change the links and splash screen
			}
			location_combo->setTextEntry(new_start_slurl.getLocationString());
			sInstance->mLocationLength = new_start_slurl.getLocationString().length();
			sInstance->updateLoginButtons();
		}
		else
		{
			// the grid specified by the slurl is not known
			LLNotificationsUtil::add("InvalidLocationSLURL");
			LL_WARNS("AppInit")<<"invalid LoginLocation:"<<new_start_slurl.asString()<<LL_ENDL;
			location_combo->setTextEntry(LLStringUtil::null);
		}
	  }
 	break;

	case LLSLURL::HOME_LOCATION:
		location_combo->setCurrentByIndex(1); // home location
		break;
		
	case LLSLURL::LAST_LOCATION:
		location_combo->setCurrentByIndex(0); // last location
		break;

	default:
		LL_WARNS("AppInit")<<"invalid login slurl, using home"<<LL_ENDL;
		location_combo->setCurrentByIndex(1); // home location
		break;
	}
}

void LLPanelLogin::setLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"setting Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above
}

void LLPanelLogin::autologinToLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"automatically logging into Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above

	if ( LLPanelLogin::sInstance != NULL )
	{
		void* unused_parameter = 0;
		LLPanelLogin::sInstance->onClickConnect(unused_parameter);
	}
}

// static
void LLPanelLogin::closePanel()
{
	if (sInstance)
	{
		LLPanelLogin::sInstance->getParent()->removeChild( LLPanelLogin::sInstance );

		delete sInstance;
		sInstance = NULL;
	}
}

// static
void LLPanelLogin::setAlwaysRefresh(bool refresh)
{
	if (sInstance && LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");

		if (web_browser)
		{
			web_browser->setAlwaysRefresh(refresh);
		}
	}
}



void LLPanelLogin::loadLoginPage()
{
	if (!sInstance) return;

	LLURI login_page = LLURI(LLGridManager::getInstance()->getLoginPage());
	LLSD params(login_page.queryMap());

	LL_DEBUGS("AppInit") << "login_page: " << login_page << LL_ENDL;

	// allow users (testers really) to specify a different login content URL
	std::string force_login_url = gSavedSettings.getString("ForceLoginURL");
	if ( force_login_url.length() > 0 )
	{
		login_page = LLURI(force_login_url);
	}

	// Language
	params["lang"] = LLUI::getLanguage();

	// First Login?
	if (gSavedSettings.getBOOL("FirstLoginThisInstall"))
	{
		params["firstlogin"] = "TRUE"; // not bool: server expects string TRUE
	}

	// Channel and Version
	params["version"] = llformat("%s (%d)",
								 LLVersionInfo::getShortVersion().c_str(),
								 LLVersionInfo::getBuild());
	params["channel"] = LLVersionInfo::getChannel();

	// Grid
	params["grid"] = LLGridManager::getInstance()->getGridId();

	// add OS info
	params["os"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();

	// sourceid
	params["sourceid"] = gSavedSettings.getString("sourceid");

	// login page (web) content version
	params["login_content_version"] = gSavedSettings.getString("LoginContentVersion");

	// Make an LLURI with this augmented info
	LLURI login_uri(LLURI::buildHTTP(login_page.authority(),
									 login_page.path(),
									 params));

	LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");
	if (web_browser->getCurrentNavUrl() != login_uri.asString())
	{
		LL_DEBUGS("AppInit") << "loading:    " << login_uri << LL_ENDL;
		web_browser->navigateTo( login_uri.asString(), HTTP_CONTENT_TEXT_HTML );
	}
}

void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
}

//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------

// static
void LLPanelLogin::onClickConnect(void *)
{
	if (sInstance && sInstance->mCallback)
	{
		// login button disabled
		if (!sLoginButtonEnabled)
		{
			return;
		}
		// JC - Make sure the fields all get committed.
		sInstance->setFocus(FALSE);

		LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
		LLSD combo_val = combo->getSelectedValue();

		// the grid definitions may come from a user-supplied grids.xml, so they may not be good
		LL_DEBUGS("AppInit")<<"grid "<<combo_val.asString()<<LL_ENDL;
		try
		{
			LLGridManager::getInstance()->setGridChoice(combo_val.asString());
		}
		catch (LLInvalidGridName ex)
		{
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);
			return;
		}

		// The start location SLURL has already been sent to LLStartUp::setStartSLURL

		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
		gSavedSettings.setString("UserLoginInfo", credentialName());

		if(username.empty())
		{
			// user must type in something into the username field
			LLNotificationsUtil::add("MustHaveAccountToLogIn");
		}
		else if(password.empty())
		{
		    LLNotificationsUtil::add("MustEnterPasswordToLogIn");
		}
		else
		{
			LLPointer<LLCredential> cred;
			BOOL remember;
			getFields(cred, remember);
			std::string identifier_type;
			cred->identifierType(identifier_type);
			LLSD allowed_credential_types;
			LLGridManager::getInstance()->getLoginIdentifierTypes(allowed_credential_types);
			
			// check the typed in credential type against the credential types expected by the server.
			for(LLSD::array_iterator i = allowed_credential_types.beginArray();
				i != allowed_credential_types.endArray();
			    ++i)
			{
				
				if(i->asString() == identifier_type)
				{
					// yay correct credential type
					sInstance->mCallback(0, sInstance->mCallbackData);
					return;
				}
			}
			
			// Right now, maingrid is the only thing that is picky about
			// credential format, as it doesn't yet allow account (single username)
			// format creds.  - Rox.  James, we wanna fix the message when we change
			// this.
			LLNotificationsUtil::add("InvalidCredentialFormat");			
		}
	}
}

// static
void LLPanelLogin::onClickNewAccount(void*)
{
	if (sInstance)
	{
#ifdef OPENSIM
		LLSD grid_info;
		LLGridManager::getInstance()->getGridData(grid_info);

		if (LLGridManager::getInstance()->isInOpenSim() && grid_info.has(GRID_REGISTER_NEW_ACCOUNT))
			LLWeb::loadURLInternal(grid_info[GRID_REGISTER_NEW_ACCOUNT]);
		else
#endif // OPENSIM
			LLWeb::loadURLExternal(LLTrans::getString("create_account_url"));
	}
}


// static
void LLPanelLogin::onClickVersion(void*)
{
	LLFloaterReg::showInstance("sl_about"); 
}

//static
void LLPanelLogin::onClickForgotPassword(void*)
{
	if (sInstance)
	{
#ifdef OPENSIM
		LLSD grid_info;
		LLGridManager::getInstance()->getGridData(grid_info);

		if (LLGridManager::getInstance()->isInOpenSim() && grid_info.has(GRID_FORGOT_PASSWORD))
			LLWeb::loadURLInternal(grid_info[GRID_FORGOT_PASSWORD]);
		else
#endif // OPENSIM
		LLWeb::loadURLExternal(sInstance->getString( "forgot_password_url" ));
	}
}

//static
void LLPanelLogin::onClickHelp(void*)
{
	if (sInstance)
	{
		LLViewerHelp* vhelp = LLViewerHelp::getInstance();
		vhelp->showTopic(vhelp->preLoginTopic());
	}
}

// static
void LLPanelLogin::onPassKey(LLLineEditor* caller, void* user_data)
{
	LLPanelLogin *self = (LLPanelLogin *)user_data;
	self->mPasswordModified = TRUE;
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == FALSE)
	{
		// *TODO: use another way to notify user about enabled caps lock, see EXT-6858
		sCapslockDidNotification = TRUE;
	}

	LLLineEditor* password_edit(self->getChild<LLLineEditor>("password_edit"));
	self->mPasswordLength = password_edit->getText().length();
	self->updateLoginButtons();
}


void LLPanelLogin::updateServer()
{
	if (sInstance)
	{
		try 
		{
			// if they've selected another grid, we should load the credentials
			// for that grid and set them to the UI.
			if(!sInstance->areCredentialFieldsDirty())
			{
				LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(credentialName());
				sInstance->setFields(credential);
			}

			// grid changed so show new splash screen (possibly)
			updateServerCombo();
			loadLoginPage();

#ifdef OPENSIM
			sInstance->getChild<LLLineEditor>("password_edit")->setMaxTextChars(LLGridManager::getInstance()->isInSecondLife() ? MAX_PASSWORD_SL : MAX_PASSWORD_OPENSIM);
#endif
		}
		catch (LLInvalidGridName ex)
		{
			LL_WARNS("AppInit")<<"server '"<<ex.name()<<"' selection failed"<<LL_ENDL;
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);	
			return;
		}
	}
}

void LLPanelLogin::updateLoginButtons()
{
	LLButton* login_btn = getChild<LLButton>("connect_btn");

	sLoginButtonEnabled = false;
	auto gPVDataOldAPI = PVDataOldAPI::getInstance();
	if (gPVDataOldAPI->getDataDone())
	{
		if (!gPVDataOldAPI->isBlockedRelease())
		{
			sLoginButtonEnabled = true;
			static const std::string loginString = LLTrans::getString("Login");
			login_btn->setLabel(loginString);
		}
		else
		{
			//enable_button = false;
			static const std::string outdatedString = LLTrans::getString("Outdated");
			login_btn->setLabel(outdatedString);
			LLSD args;
			args["REASON"] = gPVDataOldAPI->getErrorMessage();
			LLNotificationsUtil::add("BlockedReleaseReason", args);
		}
	}
	else
	{
		//enable_button = false;
		static const std::string plzHoldString = LLTrans::getString("PleaseHold");
		login_btn->setLabel(plzHoldString);
	}
	if (mUsernameLength == 0 || mPasswordLength == 0)
	{
		sLoginButtonEnabled = false;
		login_btn->setLabel(LLTrans::getString("EnterCredentials"));
	}
	login_btn->setEnabled(sLoginButtonEnabled);
}

//static
void LLPanelLogin::doLoginButtonLockUnlock()
{
	if (sInstance)
	{
		sInstance->updateLoginButtons();
	}
}

void LLPanelLogin::onSelectServer()
{
	// The user twiddled with the grid choice ui.
	// apply the selection to the grid setting.
	LLPointer<LLCredential> credential;

	LLComboBox* server_combo = getChild<LLComboBox>("server_combo");
	LLSD server_combo_val = server_combo->getSelectedValue();
	LL_INFOS("AppInit") << "grid "<<server_combo_val.asString()<< LL_ENDL;
	LLGridManager::getInstance()->setGridChoice(server_combo_val.asString());
	
	/*
	 * Determine whether or not the value in the start_location_combo makes sense
	 * with the new grid value.
	 *
	 * Note that some forms that could be in the location combo are grid-agnostic,
	 * such as "MyRegion/128/128/0".  There could be regions with that name on any
	 * number of grids, so leave them alone.  Other forms, such as
	 * https://grid.example.com/region/Party%20Town/20/30/5 specify a particular
	 * grid; in those cases we want to clear the location.
	 */
	LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	S32 index = location_combo->getCurrentIndex();
	switch (index)
	{
	case 0: // last location
	case 1: // home location
		// do nothing - these are grid-agnostic locations
		break;
		
	default:
		{
			std::string location = location_combo->getValue().asString();
			LLSLURL slurl(location); // generata a slurl from the location combo contents
			if (   slurl.getType() == LLSLURL::LOCATION
				&& slurl.getGrid() != LLGridManager::getInstance()->getGrid()
				)
			{
				// the grid specified by the location is not this one, so clear the combo
				location_combo->setCurrentByIndex(0); // last location on the new grid
				location_combo->setTextEntry(LLStringUtil::null);
			}
		}			
		break;
	}

	updateServer();
}

void LLPanelLogin::onLocationSLURL()
{
	LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	std::string location = location_combo->getValue().asString();
	LL_DEBUGS("AppInit")<<location<<LL_ENDL;

	LLStartUp::setStartSLURL(location); // calls onUpdateStartSLURL, above 
}

/////////////////////////////
//   Firestorm functions   //
/////////////////////////////

std::string canonicalize_username(const std::string& name)
{
	std::string cname = name;
	
	size_t arobase = cname.find('@');
	if (arobase > 0)
	{
		cname = cname.substr(0, arobase - 1);
	}
	
	// determine if the username is a first/last form or not.
	size_t separator_index = cname.find_first_of(" ._");
	std::string first = cname.substr(0, separator_index);
	std::string last;
	if (separator_index != cname.npos)
	{
		last = cname.substr(separator_index + 1, cname.npos);
		LLStringUtil::trim(last);
	}
	else
	{
		// ...on Linden grids, single username users as considered to have
		// last name "Resident"
		last = "Resident";
	}

	// Username in traditional "firstname lastname" form.
	return first + ' ' + last;
}

void LLPanelLogin::addUsersToCombo(BOOL show_server)
{
	LLComboBox* combo = getChild<LLComboBox>("username_combo");
	if (!combo) return;
	
	combo->removeall();
	std::string current_creds=credentialName();
	if(current_creds.find('@') < 1)
	{
		current_creds = gSavedSettings.getString("UserLoginInfo");
	}
	
	std::vector<std::string> logins = gSecAPIHandler->listCredentials();
	LLUUID selectid;
	LLStringUtil::trim(current_creds);
	for (std::vector<std::string>::iterator login_choice = logins.begin();
		 login_choice != logins.end();
	     ++login_choice)
	{
		std::string name = *login_choice;
		LLStringUtil::trim(name);
		
		std::string credname = name;
		std::string gridname = name;
		size_t arobase = gridname.find('@');
		if (arobase != std::string::npos && arobase + 1 < gridname.length() && arobase > 1)
		{
			gridname = gridname.substr(arobase + 1, gridname.length() - arobase - 1);
			name = name.substr(0,arobase);
			
			const std::string grid_label = LLGridManager::getInstance()->getGridLabel(gridname);
			
			bool add_grid = false;
			/// We only want to append a grid label when the user has enabled logging into other grids, or
			/// they are using the OpenSim build. That way users who only want Second Life Agni can remain
			/// blissfully ignorant. We will also not show them any saved credential that isn't Agni because
			/// they don't want them.
			if (SECOND_LIFE_MAIN_LABEL == grid_label)
			{
				if (show_server)
					name.append( " @ " + grid_label);
				add_grid = true;
			}
#ifdef OPENSIM
			else if (!grid_label.empty() && show_server)
			{
				name.append(" @ " + grid_label);
				add_grid = true;
			}
#else  // OPENSIM
			else if (SECOND_LIFE_BETA_LABEL == grid_label && show_server)
			{
				name.append(" @ " + grid_label);
				add_grid = true;
			}
#endif // OPENSIM
			if (add_grid)
			{
				combo->add(name,LLSD(credname));
			}
		}
	}
	combo->sortByName();
	combo->selectByValue(LLSD(current_creds));
}

// static
void LLPanelLogin::onClickRemove(void*)
{
	if (sInstance)
	{
		LLComboBox* combo = sInstance->getChild<LLComboBox>("username_combo");
		std::string credName = combo->getValue().asString();
		LLNotificationsUtil::add("ConfirmRemoveCredential", LLSD().with("NAME", combo->getSelectedItemLabel()), LLSD().with("CredName", credName), boost::bind(&LLPanelLogin::onRemoveCallback, _1, _2));
	}
}

// static
void LLPanelLogin::onRemoveCallback(const LLSD& notification, const LLSD& response)
{
	if (sInstance)
	{
		S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		if (option == 0)
		{
			std::string credName = notification["payload"]["CredName"].asString();
			if (credName == gSavedSettings.getString("UserLoginInfo"))
			{
				gSavedSettings.getControl("UserLoginInfo")->resetToDefault();
			}
			LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(credName);
			gSecAPIHandler->deleteCredential(credential);
			sInstance->addUsersToCombo(gSavedSettings.getBOOL("ForceShowGrid"));

			if (!sInstance->getChild<LLComboBox>("username_combo")->selectFirstItem())
			{
				sInstance->getChild<LLUICtrl>("username_combo")->clear();
				sInstance->getChild<LLUICtrl>("password_edit")->clear();
			}
		}
	}
}

#if LOGIN_MGR_HELP
//static
void LLPanelLogin::onClickGridMgrHelp(void*)
{
	if (sInstance)
	{
		LLViewerHelp* vhelp = LLViewerHelp::getInstance();
		vhelp->showTopic(vhelp->gridMgrHelpTopic());
	}
}

#endif
void LLPanelLogin::onSelectUser()
{
	LL_INFOS("AppInit") << "onSelectUser()" << LL_ENDL;
	
	if (!sInstance)
	{
		return;
	}

	LLComboBox* combo = sInstance->getChild<LLComboBox>("username_combo");

	if (combo->getValue().asString() == sInstance->mPreviousUsername)
	{
		return;
	}

	LLSD combo_val = combo->getSelectedValue();
	if (combo_val.isUndefined())
	{
		// Previously unknown username was entered
		if (!sInstance->mPasswordModified)
		{
			// Clear password unless manually entered
			sInstance->getChild<LLLineEditor>("password_edit")->clear();
		}
		sInstance->addFavoritesToStartLocation();
		sInstance->mPreviousUsername = combo->getValue().asString();
		sInstance->mUsernameLength = combo->getValue().asString().length();
		sInstance->getChild<LLButton>("remove_user_btn")->setEnabled(FALSE);
		return;
	}

	// Saved username was either selected or entered; continue...
	const std::string cred_name = combo_val.asString();

	LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(cred_name);
	sInstance->setFields(credential);
	sInstance->mPasswordModified = FALSE;
	sInstance->getChild<LLButton>("remove_user_btn")->setEnabled(TRUE);

	size_t arobase = cred_name.find('@');
	if (arobase != std::string::npos && arobase + 1 < cred_name.length())
	{
		const std::string grid_name = cred_name.substr(arobase + 1, cred_name.length() - arobase - 1);

		// Grid has changed - set new grid and update server combo
		if (LLGridManager::getInstance()->getGrid() != grid_name)
		{
			try
			{
				LLGridManager::getInstance()->setGridChoice(grid_name);
			}
			catch (LLInvalidGridName ex)
			{
				LL_WARNS("AppInit") << "Could not set grid '" << ex.name() << "'" << LL_ENDL;
			}
			updateServer();
		}
	}
	sInstance->updateLoginButtons();
	sInstance->addFavoritesToStartLocation();
}

// static
void LLPanelLogin::updateServerCombo()
{
	if (!sInstance) return;
	
	// We add all of the possible values, sorted, and then add a bar and the current value at the top
	LLComboBox* server_choice_combo = sInstance->getChild<LLComboBox>("server_combo");
	server_choice_combo->removeall();

	std::string current_grid = LLGridManager::getInstance()->getGrid();
	std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
	
	for (std::map<std::string, std::string>::iterator grid_choice = known_grids.begin();
		 grid_choice != known_grids.end();
	     ++grid_choice)
	{
		if (!grid_choice->first.empty() && current_grid != grid_choice->first)
		{
			LL_DEBUGS("AppInit") << "adding " << grid_choice->first << LL_ENDL;
			server_choice_combo->add(grid_choice->second, grid_choice->first);
		}
	}
	server_choice_combo->sortByName();
	server_choice_combo->addSeparator(ADD_TOP);
	
	LL_DEBUGS("AppInit") << "adding current " << current_grid << LL_ENDL;
	server_choice_combo->add(LLGridManager::getInstance()->getGridLabel(),
							 current_grid,
							 ADD_TOP);
	server_choice_combo->selectFirstItem();
#if LOGIN_MGR_HELP
	update_grid_help();
#endif
}

// static
std::string LLPanelLogin::credentialName()
{
	std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
	LLStringUtil::trim(username);
	
	size_t arobase = username.find('@');
	if (arobase != std::string::npos && arobase + 1 < username.length())
	{
		username = username.substr(0, arobase);
	}
	LLStringUtil::trim(username);
	
	return username + "@" + LLGridManager::getInstance()->getGrid();
}

// static
void LLPanelLogin::gridListChanged(bool success)
{
	updateServer();
}

// static
bool LLPanelLogin::getShowFavorites()
{
	if (sInstance)
	{
		return sInstance->mShowFavorites;
	}
	else
	{
		return gSavedPerAccountSettings.getBOOL("ShowFavoritesOnLogin");
	}
}

void LLPanelLogin::onUsernameTextChanged()
{
	mUsernameLength = getChild<LLUICtrl>("username_combo")->getValue().asString().length();
	updateLoginButtons();
}

#if SETTINGS_PRESETS
/////////////////////////
//    Mode selector    //
/////////////////////////

void LLPanelLogin::onModeChange(const LLSD& original_value, const LLSD& new_value)
{
	// <FS:AO> make sure toolbar settings are reset on mode change
	if (gSavedSettings.getBOOL("FSToolbarsResetOnModeChange"))
	{
		LL_INFOS() << "Clearing toolbar settings." << LL_ENDL;
		gSavedSettings.setBOOL("ResetToolbarSettings", TRUE);
	}

	if (original_value.asString() != new_value.asString())
	{
		LLNotificationsUtil::add("ModeChange", LLSD(), LLSD(), boost::bind(&LLPanelLogin::onModeChangeConfirm, this, original_value, new_value, _1, _2));
	}
}

void LLPanelLogin::onModeChangeConfirm(const LLSD& original_value, const LLSD& new_value, const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch (option)
	{
		case 0:
			gSavedSettings.getControl("SessionSettingsFile")->set(new_value);
			LLAppViewer::instance()->requestQuit();
			break;
		case 1:
			// revert to original value
			getChild<LLUICtrl>("mode_combo")->setValue(original_value);
			break;
		default:
			break;
	}
}
#endif
