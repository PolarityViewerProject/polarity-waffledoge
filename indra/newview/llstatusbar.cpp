/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llstatusbar.h"

// viewer includes
#include "llagent.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llbuycurrencyhtml.h"
#include "llpanelnearbymedia.h"
#include "ospanelquicksettingspulldown.h"
#include "llpanelvolumepulldown.h"
#include "llhints.h"
#include "llmenugl.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "llrootview.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llstatgraph.h"
#include "llviewermedia.h"
#include "llviewermenu.h"	// for gMenuBarView
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"
#include "llappviewer.h"
#include "llweb.h"

// library includes
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

// system includes
#include "llwindowwin32.h" // for refresh rate and such
#include "llfloaterreg.h"

//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const S32 SIM_STAT_WIDTH = 8;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY		= 3.f; // How long the balance and health icons should flash after a change.

static void onClickVolume(void* data);

// initialize static member to avoid unresolved external symbols. Ah, C++...
LLColor4 LLStatusBar::gFPSColor = LLUIColorTable::instance().getColor("EmphasisColor", LLColor4::green); // is that even legal?

LLStatusBar::LLStatusBar(const LLRect& rect)
:	LLPanel(),
	mTextTime(nullptr),
	mFPSCount(nullptr), // <polarity> FPS Counter in the status bar
	mSGBandwidth(nullptr),
	mSGPacketLoss(nullptr),
	mBandwidthButton(NULL), // <FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window
	mBtnStats(nullptr),
	mBtnQuickSettings(nullptr),
	mBtnVolume(nullptr),
	mBoxBalance(nullptr),
	mBalance(0),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0)
{
	LLView::setRect(rect);
	
	// status bar can possible overlay menus?
	setMouseOpaque(FALSE);

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();
	gSavedSettings.getControl("ShowNetStats")->getSignal()->connect(boost::bind(&LLStatusBar::updateNetstatVisibility, this, _2));

	buildFromFile("panel_status_bar.xml");
}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	refresh();
	LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	show_navbar_context_menu(this,x,y);
	return TRUE;
}

BOOL LLStatusBar::postBuild()
{
	gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

	mTextTime = getChild<LLTextBox>("TimeText" );
	
	// <polarity> FPS Meter in status bar. Inspired by NiranV Dean's initial implementation in Black Dragon
	mFPSCount = getChild<LLTextBox>("FPS_count");

	getChild<LLUICtrl>("buyL")->setCommitCallback(boost::bind(&LLWeb::loadURLExternal, "https://secondlife.com/my/lindex/buy.php"));

	getChild<LLUICtrl>("goShop")->setCommitCallback(boost::bind(&LLWeb::loadURL, gSavedSettings.getString("MarketplaceURL"), LLStringUtil::null, LLStringUtil::null));

	mBoxBalance = getChild<LLTextBox>("balance");
	mBoxBalance->setClickedCallback( &LLStatusBar::onClickBalance, this );
	
	mBtnStats = getChildView("stat_btn");

	mBtnQuickSettings = getChild<LLButton>("quick_settings_btn");
	mBtnQuickSettings->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterQuickSettings, this));

	mBtnVolume = getChild<LLButton>( "volume_btn" );
	mBtnVolume->setClickedCallback( onClickVolume, this );
	mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));

	mMediaToggle = getChild<LLButton>("media_toggle_btn");
	mMediaToggle->setClickedCallback( &LLStatusBar::onClickMediaToggle, this );
	mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));

	LLHints::registerHintTarget("linden_balance", getChild<LLView>("balance_bg")->getHandle());

	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));

	// Adding Net Stat Graph
	S32 x = getRect().getWidth() - 2;
	S32 y = 0;
	LLRect r;
	
	// <FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window
	r.set( x-((SIM_STAT_WIDTH*2)+2), y+MENU_BAR_HEIGHT-1, x, y+1);
	LLButton::Params BandwidthButton;
	BandwidthButton.name(std::string("BandwidthGraphButton"));
	BandwidthButton.label("");
	BandwidthButton.rect(r);
	BandwidthButton.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	BandwidthButton.click_callback.function(boost::bind(&LLStatusBar::onBandwidthGraphButtonClicked, this));
	mBandwidthButton = LLUICtrlFactory::create<LLButton>(BandwidthButton);
	addChild(mBandwidthButton);
	LLColor4 BandwidthButtonOpacity;
	BandwidthButtonOpacity.setAlpha(0);
	mBandwidthButton->setColor(BandwidthButtonOpacity);
	// </FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window
	
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	LLStatGraph::Params sgp;
	sgp.name("BandwidthGraph");
	sgp.rect(r);
	sgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	sgp.mouse_opaque(false);
	sgp.stat.count_stat_float(&LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED);
	sgp.units("Kbps");
	sgp.precision(0);
	sgp.per_sec(true);
	mSGBandwidth = LLUICtrlFactory::create<LLStatGraph>(sgp);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;

	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	//these don't seem to like being reused
	LLStatGraph::Params pgp;
	pgp.name("PacketLossPercent");
	pgp.rect(r);
	pgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	pgp.mouse_opaque(false);
	pgp.stat.sample_stat_float(&LLStatViewer::PACKETS_LOST_PERCENT);
	pgp.units("%");
	pgp.min(0.f);
	pgp.max(5.f);
	pgp.precision(1);
	pgp.per_sec(false);
	LLStatGraph::Thresholds thresholds;
	thresholds.threshold.add(LLStatGraph::ThresholdParams().value(0.1f).color(LLColor4::green))
						.add(LLStatGraph::ThresholdParams().value(0.25f).color(LLColor4::yellow))
						.add(LLStatGraph::ThresholdParams().value(0.6f).color(LLColor4::red));

	pgp.thresholds(thresholds);

	mSGPacketLoss = LLUICtrlFactory::create<LLStatGraph>(pgp);
	addChild(mSGPacketLoss);

	mPanelQuickSettingsPulldown = new OSPanelQuickSettingsPulldown();
	addChild(mPanelQuickSettingsPulldown);
	mPanelQuickSettingsPulldown->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
	mPanelQuickSettingsPulldown->setVisible(FALSE);

	mPanelVolumePulldown = new LLPanelVolumePulldown();
	addChild(mPanelVolumePulldown);
	mPanelVolumePulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelVolumePulldown->setVisible(FALSE);

	mPanelNearByMedia = new LLPanelNearByMedia();
	addChild(mPanelNearByMedia);
	mPanelNearByMedia->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelNearByMedia->setVisible(FALSE);

	mScriptOut = getChildView("scriptout");

	mRefreshRate = LLWindowWin32::getRefreshRate();
	static LLCachedControl<bool> show_net_stats(gSavedSettings, "ShowNetStats", false);
	if (!show_net_stats)
	{
		updateNetstatVisibility(LLSD(FALSE));
	}

	return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	static LLCachedControl<bool> show_net_stats(gSavedSettings, "ShowNetStats", false);
	bool net_stats_visible = show_net_stats;

	if (net_stats_visible)
	{
		// Adding Net Stat Meter back in
		F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
		mSGBandwidth->setMin(0.f);
		mSGBandwidth->setMax(bwtotal*1.25f);
		//mSGBandwidth->setThreshold(0, bwtotal*0.75f);
		//mSGBandwidth->setThreshold(1, bwtotal);
		//mSGBandwidth->setThreshold(2, bwtotal);
	}

	const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();

	// reshape menu bar to its content's width
	if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
	{
		gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
	}

	// update the master volume button state
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	mBtnVolume->setToggleState(mute_audio);
	
	// Disable media toggle if there's no media, parcel media, and no parcel audio
	// (or if media is disabled)
	bool button_enabled = (gSavedSettings.getBOOL("AudioStreamingMusic")||gSavedSettings.getBOOL("AudioStreamingMedia")) && 
						  (LLViewerMedia::hasInWorldMedia() || LLViewerMedia::hasParcelMedia() || LLViewerMedia::hasParcelAudio());
	mMediaToggle->setEnabled(button_enabled);
	// Note the "sense" of the toggle is opposite whether media is playing or not
	bool any_media_playing = (LLViewerMedia::isAnyMediaShowing() || 
							  LLViewerMedia::isParcelMediaPlaying() ||
							  LLViewerMedia::isParcelAudioPlaying());
	mMediaToggle->setValue(!any_media_playing);

	if (mClockUpdateTimer.getElapsedTimeF32() < 0.25f)
	{
		return;
	}

	mClockUpdateTimer.reset();
	// Get current UTC time, adjusted for the user's clock being off.
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr;
	// <polarity> PLVR-4 24-hour clock mode
	static LLCachedControl<bool> mShowSeconds(gSavedSettings, "PVUI_ClockShowSeconds", true);
	static LLCachedControl<bool> use_24h_clock(gSavedSettings, "PVUI_ClockUse24hFormat", false);
	if (use_24h_clock)
	{
		timeStr = getString(mShowSeconds ? "time24Precise" : "time24");
	}
	else
	{
		timeStr = getString(mShowSeconds ? "timePrecise" : "time");
	}
	// </polarity>
	LLSD substitution;
	substitution["datetime"] = static_cast<S32>(utc_time);
	LLStringUtil::format(timeStr, substitution);
	mTextTime->setText(timeStr);
	// set the tooltip to have the date
	std::string dtStr = getString("timeTooltip");
	LLStringUtil::format(dtStr, substitution);
	mTextTime->setToolTip(dtStr);

	// <polarity> FPS Meter in status bar. Inspired by NiranV Dean's work
	if (mFPSCount == NULL)
	{
		return;
	}
	// Throttle a bit to avoid making faster FPS heavier to process
	if (mFPSCountTimer.getElapsedTimeF32() > 0.1)
	{
		mFPSCountTimer.reset(); // Reset the FPS timer so that we can count again

	// Update the FPS count value from the statistics system (This is the normalized value, like in the statitics floater)
		LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording(); // capture sample of the frame recording, I think.
		auto current_fps_normalized = frame_recording.getPeriodMeanPerSec(LLStatViewer::FPS); // current fps showed to the user

		F32 current_fps_sampled = frame_recording.getPeriodMeanPerSec(LLStatViewer::FPS, 2);

		// Cap the amount of decimals we return
		if (current_fps_normalized > 100.f)
		{
			mFPSCount->setValue(llformat("%.0f", current_fps_normalized) + "/" + std::to_string(mRefreshRate));
		}
		else if (current_fps_normalized > 10.f)
		{
			mFPSCount->setValue(llformat("%.1f", current_fps_normalized) + "/" + std::to_string(mRefreshRate));
		}
		else
		{
			mFPSCount->setValue(llformat("%.2f", current_fps_normalized) + "/" + std::to_string(mRefreshRate));
		}
		// </polarity>

		static LLUIColor color_fps_default = LLUIColorTable::instance().getColor("EmphasisColor");

		// Quick and Dirty FPS counter colors. Idea is from NiranV, which never got finished.
		static LLUIColor color_critical = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Critical", LLColor4::red);
		static LLUIColor color_low = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Low", LLColor4::orange);
		static LLUIColor color_medium = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Medium", LLColor4::yellow);
		static LLUIColor color_high = LLUIColorTable::instance().getColor("PVUI_FPSCounter_High", LLColor4::green);
		static LLUIColor color_outstanding = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Outstanding", LLColor4::cyan);

		static LLUIColor color_vsync = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Vsync", LLColor4::blue2);
		static LLUIColor color_limited = LLUIColorTable::instance().getColor("PVUI_FPSCounter_Limited", LLColor4::purple);

		/*static*/ LLCachedControl<U32> fps_critical(gSavedSettings, "PVUI_FPSCounter_Critical", 10);
		/*static*/ LLCachedControl<U32> fps_low(gSavedSettings, "PVUI_FPSCounter_Low", 20);
		/*static*/ LLCachedControl<U32> fps_medium(gSavedSettings, "PVUI_FPSCounter_Medium", 40);
		/*static*/ LLCachedControl<U32> fps_high(gSavedSettings, "PVUI_FPSCounter_High", 50);
		/*static*/ LLCachedControl<U32> fps_outstanding(gSavedSettings, "PVUI_FPSCounter_Outstanding", 120);

		static LLCachedControl<bool> fps_limited(gSavedSettings, "PVRender_FPSLimiterEnabled", false);
		static LLCachedControl<F32> fps_limit_target(gSavedSettings, "PVRender_FPSLimiterTarget", 60.f);

		// TODO: Add a "status indicator" textbox or two somewhere in the top bar AND the statistics floater
		// to show vsync'd and limited statuses.
		// e.g.
		//_______________________________
		// FPS Limited Vsync          72 |
		//-------------------------------|
		// FPS BAR HERE .    | .        ||
		// FPS BAR HERE  .   |     .    ||
		// FPS BAR HERE    . |  .       ||
		//нннннн-------------------------------|
		// ~/~

		//U32 vsync_mode = gSavedSettings.getU32("PVRender_VsyncMode");
		static LLCachedControl<U32> vsync_mode(gSavedSettings, "PVRender_VsyncMode");
		if (fps_limited && (current_fps_normalized <= (fps_limit_target + 1) && current_fps_normalized >= (fps_limit_target - 1)))
		{
			gFPSColor = color_limited;
		}
		else if ((vsync_mode == 1 || vsync_mode == 2) && (current_fps_normalized <= (mRefreshRate + 1) && current_fps_normalized >= (mRefreshRate - 1)))
		{
			gFPSColor = color_vsync;
		}
		else if (current_fps_sampled <= fps_critical)
		{
			gFPSColor = color_critical;
		}
		else if (current_fps_sampled >= fps_critical && (current_fps_sampled < fps_medium))
		{
			gFPSColor = color_low;
		}
		else if (current_fps_sampled >= fps_low && (current_fps_sampled < fps_high))
		{
			gFPSColor = color_medium;
		}
		else if (current_fps_sampled >= fps_medium && (current_fps_sampled < fps_outstanding))
		{
			gFPSColor = color_high;
		}
		else if (current_fps_sampled >= fps_outstanding)
		{
			gFPSColor = color_outstanding;
		}
		else
		{
			// all else fails, fallback to default color to prevent blackness
			gFPSColor = color_fps_default;
		}

		// Note: We can't/shouldn't use the color table because we do alpha math on that color in the stats floater later. The lookups are probably pretty costy.
		mFPSCount->setColor(gFPSColor);
	}
	// </polarity>
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
	mTextTime->setVisible(visible);
	getChild<LLUICtrl>("balance_bg")->setVisible(visible && gSavedSettings.getBOOL("PVUI_ShowCurrencyBalanceInStatusBar"));
	mBoxBalance->setVisible(visible);
	mBtnQuickSettings->setVisible(visible);
	mBtnVolume->setVisible(visible);
	mMediaToggle->setVisible(visible);
	mSGBandwidth->setVisible(visible);
	mSGPacketLoss->setVisible(visible);
	setBackgroundVisible(visible);
}

void LLStatusBar::debitBalance(S32 debit)
{
	setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
	setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
	if (balance > getBalance() && getBalance() != 0)
	{
		LLFirstUse::receiveLindens();
	}

	std::string money_str = LLResMgr::getInstance()->getMonetaryString( balance );

	LLStringUtil::format_map_t string_args;
	string_args["[AMT]"] = llformat("%s", money_str.c_str());
	std::string label_str = getString("buycurrencylabel", string_args);
	mBoxBalance->setValue(label_str);

	// Resize the L$ balance background to be wide enough for your balance plus the buy button
	{
		const S32 HPAD = 24;
		LLRect balance_rect = mBoxBalance->getTextBoundingRect();
		LLRect buy_rect = getChildView("buyL")->getRect();
		LLRect shop_rect = getChildView("goShop")->getRect();
		LLView* balance_bg_view = getChildView("balance_bg");
		LLRect balance_bg_rect = balance_bg_view->getRect();
		balance_bg_rect.mLeft = balance_bg_rect.mRight - (buy_rect.getWidth() + shop_rect.getWidth() + balance_rect.getWidth() + HPAD);
		balance_bg_view->setShape(balance_bg_rect);
	}

	if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
	{
		if (mBalance > balance)
			make_ui_sound("UISndMoneyChangeDown");
		else
			make_ui_sound("UISndMoneyChangeUp");
	}

	if( balance != mBalance )
	{
		mBalanceTimer->reset();
		mBalanceTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
		mBalance = balance;
	}
}


// static
void LLStatusBar::sendMoneyBalanceRequest()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
	gAgent.sendReliableMessage();
}


void LLStatusBar::setHealth(S32 health)
{
	//LL_INFOS() << "Setting health to: " << buffer << LL_ENDL;
	if( mHealth > health )
	{
		if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
		{
			if (isAgentAvatarValid())
			{
				if (gAgentAvatarp->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}

		mHealthTimer->reset();
		mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
	}

	mHealth = health;
}

// <polarity> PLVR-7 Hide currency balance in snapshots
void LLStatusBar::showBalance(bool show)
{
	mBoxBalance->setVisible(show);
}
// </polarity>

S32 LLStatusBar::getBalance() const
{
	return mBalance;
}


S32 LLStatusBar::getHealth() const
{
	return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}

void LLStatusBar::onClickBuyCurrency()
{
	// open a currency floater - actual one open depends on 
	// value specified in settings.xml
	LLBuyCurrencyHTML::openCurrencyFloater();
	LLFirstUse::receiveLindens(false);
}

void LLStatusBar::onMouseEnterQuickSettings()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLRect qs_rect = mPanelQuickSettingsPulldown->getRect();
	LLRect qs_btn_rect = mBtnQuickSettings->getRect();
	qs_rect.setLeftTopAndSize(qs_btn_rect.mLeft -
		(qs_rect.getWidth() - qs_btn_rect.getWidth()) / 2,
		qs_btn_rect.mBottom,
		qs_rect.getWidth(),
		qs_rect.getHeight());
	// force onscreen
	qs_rect.translate(popup_holder->getRect().getWidth() - qs_rect.mRight, 0);

	// show the master volume pull-down
	mPanelQuickSettingsPulldown->setShape(qs_rect);
	LLUI::clearPopups();
	LLUI::addPopup(mPanelQuickSettingsPulldown);

	mPanelNearByMedia->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(FALSE);
	mPanelQuickSettingsPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterVolume()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLButton* volbtn =  getChild<LLButton>( "volume_btn" );
	LLRect vol_btn_rect = volbtn->getRect();
	LLRect volume_pulldown_rect = mPanelVolumePulldown->getRect();
	volume_pulldown_rect.setLeftTopAndSize(vol_btn_rect.mLeft -
	     (volume_pulldown_rect.getWidth() - vol_btn_rect.getWidth()),
			       vol_btn_rect.mBottom,
			       volume_pulldown_rect.getWidth(),
			       volume_pulldown_rect.getHeight());

	volume_pulldown_rect.translate(popup_holder->getRect().getWidth() - volume_pulldown_rect.mRight, 0);
	mPanelVolumePulldown->setShape(volume_pulldown_rect);


	// show the master volume pull-down
	LLUI::clearPopups();
	LLUI::addPopup(mPanelVolumePulldown);
	mPanelNearByMedia->setVisible(FALSE);
	mPanelQuickSettingsPulldown->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterNearbyMedia() const
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLRect nearby_media_rect = mPanelNearByMedia->getRect();
	LLButton* nearby_media_btn =  getChild<LLButton>( "media_toggle_btn" );
	LLRect nearby_media_btn_rect = nearby_media_btn->getRect();
	nearby_media_rect.setLeftTopAndSize(nearby_media_btn_rect.mLeft - 
										(nearby_media_rect.getWidth() - nearby_media_btn_rect.getWidth())/2,
										nearby_media_btn_rect.mBottom,
										nearby_media_rect.getWidth(),
										nearby_media_rect.getHeight());
	// force onscreen
	nearby_media_rect.translate(popup_holder->getRect().getWidth() - nearby_media_rect.mRight, 0);
	
	// show the master volume pull-down
	mPanelNearByMedia->setShape(nearby_media_rect);
	LLUI::clearPopups();
	LLUI::addPopup(mPanelNearByMedia);

	mPanelQuickSettingsPulldown->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(FALSE);
	mPanelNearByMedia->setVisible(TRUE);
}


static void onClickVolume(void* data)
{
	// toggle the master mute setting
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	LLAppViewer::instance()->setMasterSystemAudioMute(!mute_audio);	
}

//static 
void LLStatusBar::onClickBalance(void* )
{
	// Force a balance request message:
	LLStatusBar::sendMoneyBalanceRequest();
	// The refresh of the display (call to setBalance()) will be done by process_money_balance_reply()
}

//static 
void LLStatusBar::onClickMediaToggle(void* data)
{
	LLStatusBar *status_bar = (LLStatusBar*)data;
	// "Selected" means it was showing the "play" icon (so media was playing), and now it shows "pause", so turn off media
	bool enable = ! status_bar->mMediaToggle->getValue();
	LLViewerMedia::setAllMediaEnabled(enable);
}

void LLStatusBar::updateNetstatVisibility(const LLSD& data)
{
	const S32 NETSTAT_WIDTH = (SIM_STAT_WIDTH + 2) * 2;
	BOOL showNetStat = data.asBoolean();
	//S32 translateFactor = (showNetStat ? -1 : 1);

	mSGBandwidth->setVisible(showNetStat);
	mSGPacketLoss->setVisible(showNetStat);
	mBandwidthButton->setVisible(showNetStat); // <FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window

	//LLRect rect = mFPSCount->getRect();
	//rect.translate(NETSTAT_WIDTH * translateFactor, 0);
	//mFPSCount->setRect(rect);

	//rect = mBalancePanel->getRect();
	//rect.translate(NETSTAT_WIDTH * translateFactor, 0);
	//mBalancePanel->setRect(rect);
}

BOOL can_afford_transaction(S32 cost)
{
	return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
	refresh();
}

// <FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window
void LLStatusBar::onBandwidthGraphButtonClicked()
{
	if (gSavedSettings.getBOOL("FSUseStatsInsteadOfLagMeter"))
	{
		LLFloaterReg::toggleInstance("stats");
	}
	else
	{
		LLFloaterReg::toggleInstance("lagmeter");
	}
}
// </FS:PP> FIRE-6287: Clicking on traffic indicator toggles Lag Meter window

// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
	// Requires "trusted" browser/URL source
	LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1
			&& tokens[0].asString() == "request")
		{
			LLStatusBar::sendMoneyBalanceRequest();
			return true;
		}
		return false;
	}
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
