/** 
 * @file llviewerfloaterreg.cpp
 * @brief LLViewerFloaterReg class registers floaters used in the viewer
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h"

#include "llfloaterreg.h"

#include "llviewerfloaterreg.h"

#include "llcompilequeue.h"
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloateranimpreview.h"
#include "llfloaterauction.h"
#include "llfloateravatarpicker.h"
#include "llfloateravatartextures.h"
#include "llfloaterbeacons.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterbulkpermission.h"
#include "llfloaterbump.h"
#include "llfloatercall.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloaterdaycycle.h"
#include "llfloaterdirectory.h"
#include "llfloaterfirsttimetip.h"
#include "llfloaterenvsettings.h"
#include "llfloaterfonttest.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloatergroups.h"
#include "llfloaterhardwaresettings.h"
#include "llfloaterhtmlcurrency.h"
#include "llfloatermediabrowser.h"
#include "llfloaterhud.h"
#include "llfloaterimagepreview.h"
#include "llimpanel.h"
#include "llfloaterinspect.h"
#include "llfloaterinventory.h"
#include "llfloaterjoystick.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermemleak.h"
#include "llfloatermute.h"
#include "llfloaternamedesc.h"
#include "llfloaternotificationsconsole.h"
#include "llfloateropenobject.h"
#include "llgivemoney.h"
#include "llfloaterparcel.h"
#include "llfloaterperms.h"
#include "llfloaterpostcard.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterproperties.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloatersellland.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersnapshot.h"
#include "llfloatertelehub.h"
#include "llfloatertestlistview.h"
#include "llfloatertools.h"
#include "llfloatertos.h"
#include "llfloatertopobjects.h"
#include "llfloateruipreview.h"
#include "llfloaterurldisplay.h"
#include "llfloatervoicedevicesettings.h"
#include "llfloaterwater.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llmediaremotectrl.h"
#include "llmoveview.h"
#include "llnearbychat.h"

#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llfloaterminiinspector.h"
#include "llsyswellwindow.h"

//class LLLLFloaterObjectIMInfo;

void LLViewerFloaterReg::registerFloaters()
{
	// *NOTE: Please keep these alphabetized for easier merges
	
	LLFloaterReg::add("about_land", "floater_about_land.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLand>);
	LLFloaterReg::add("active_speakers", "floater_active_speakers.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterActiveSpeakers>);
	LLFloaterReg::add("auction", "floater_auction.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAuction>);
	LLFloaterReg::add("avatar_picker", "floater_avatar_picker.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarPicker>);
	LLFloaterReg::add("avatar_textures", "floater_avatar_textures.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarTextures>);

	LLFloaterReg::add("beacons", "floater_beacons.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBeacons>);
	LLFloaterReg::add("bulk_perms", "floater_bulk_perms.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBulkPermission>);
	LLFloaterReg::add("buy_currency", "floater_buy_currency.xml", &LLFloaterBuyCurrency::buildFloater);
	LLFloaterReg::add("buy_land", "floater_buy_land.xml", &LLFloaterBuyLand::buildFloater);
	LLFloaterReg::add("buy_object", "floater_buy_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuy>);
	LLFloaterReg::add("buy_object_contents", "floater_buy_contents.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuyContents>);
	LLFloaterReg::add("build", "floater_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTools>);
	LLFloaterReg::add("build_options", "floater_build_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuildOptions>);
	LLFloaterReg::add("bumps", "floater_bumps.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBump>);

	LLFloaterReg::add("camera", "floater_camera.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCamera>);
	LLFloaterReg::add("chat", "floater_chat_history.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChat>);
	LLFloaterReg::add("nearby_chat", "floater_nearby_chat.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLNearbyChat>);
	LLFloaterReg::add("communicate", "floater_chatterbox.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChatterBox>);
	LLFloaterReg::add("compile_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCompileQueue>);
	LLFloaterReg::add("contacts", "floater_my_friends.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMyFriends>);	

	LLFloaterReg::add("first_time_tip", "floater_first_time_tip.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFirstTimeTip>);
	LLFloaterReg::add("env_day_cycle", "floater_day_cycle_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDayCycle>);
	LLFloaterReg::add("env_post_process", "floater_post_process.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPostProcess>);
	LLFloaterReg::add("env_settings", "floater_env_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterEnvSettings>);
	LLFloaterReg::add("env_water", "floater_water.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWater>);
	LLFloaterReg::add("env_windlight", "floater_windlight_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWindLight>);
	
	LLFloaterReg::add("font_test", "floater_font_test.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFontTest>);

	LLFloaterReg::add("gestures", "floater_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGesture>);
	LLFloaterReg::add("god_tools", "floater_god_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGodTools>);
	LLFloaterReg::add("group_picker", "floater_choose_group.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGroupPicker>);

	LLFloaterReg::add("hud", "floater_hud.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHUD>);
	LLFloaterReg::add("html_simple", "floater_html_simple.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHtmlSimple>);

	LLFloaterReg::add("impanel", "floater_im_session.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIMFloater>);
	LLFloaterReg::add("inventory", "floater_inventory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInventory>);
	LLFloaterReg::add("inspect", "floater_inspect.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInspect>);
	
	LLFloaterReg::add("lagmeter", "floater_lagmeter.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLagMeter>);
	LLFloaterReg::add("land_holdings", "floater_land_holdings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLandHoldings>);
	
	LLFloaterReg::add("mem_leaking", "floater_mem_leaking.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMemLeak>);
	LLFloaterReg::add("media_browser", "floater_media_browser.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaBrowser>);	
	LLFloaterReg::add("message_critical", "floater_critical.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("message_tos", "floater_tos.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("moveview", "floater_moveview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMove>);
	LLFloaterReg::add("mute", "floater_mute.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMute>);
	LLFloaterReg::add("mute_object", "floater_mute_object.xml", &LLFloaterMute::buildFloaterMuteObjectUI);
	LLFloaterReg::add("mini_map", "floater_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMap>);
	LLFloaterReg::add("mini_inspector", "panel_mini_inspector.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMiniInspector>);
	LLFloaterReg::add("syswell_window", "floater_sys_well.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLSysWellWindow>);
	
	LLFloaterReg::add("notifications_console", "floater_notifications_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationConsole>);
	LLFloaterReg::add("nearby_chat", "floater_nearby_chat.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLNearbyChat>);

	LLFloaterReg::add("openobject", "floater_openobject.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOpenObject>);
	
	LLFloaterReg::add("parcel_info", "floater_preview_url.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterParcelInfo>);
	LLFloaterReg::add("pay_resident", "floater_pay.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPay>);
	LLFloaterReg::add("pay_object", "floater_pay_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPay>);

	LLFloaterReg::add("postcard", "floater_postcard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPostcard>);
	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	LLFloaterReg::add("prefs_hardware_settings", "floater_hardware_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHardwareSettings>);
	LLFloaterReg::add("perm_prefs", "floater_perm_prefs.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPerms>);
	LLFloaterReg::add("preview_url", "floater_preview_url.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterURLDisplay>);
	LLFloaterReg::add("pref_joystick", "floater_joystick.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterJoystick>);
	LLFloaterReg::add("pref_voicedevicesettings", "floater_device_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterVoiceDeviceSettings>);
	LLFloaterReg::add("preview_anim", "floater_preview_animation.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewAnim>, "preview");
	LLFloaterReg::add("preview_gesture", "floater_preview_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewGesture>, "preview");
	LLFloaterReg::add("preview_notecard", "floater_preview_notecard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewNotecard>, "preview");
	LLFloaterReg::add("preview_script", "floater_script_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewLSL>, "preview");
	LLFloaterReg::add("preview_scriptedit", "floater_live_lsleditor.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLLiveLSLEditor>, "preview");
	LLFloaterReg::add("preview_sound", "floater_preview_sound.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewSound>, "preview");
	LLFloaterReg::add("preview_texture", "floater_preview_texture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewTexture>, "preview");
	LLFloaterReg::add("properties", "floater_inventory_item_properties.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterProperties>);

	LLFloaterReg::add("telehubs", "floater_telehub.xml",&LLFloaterReg::build<LLFloaterTelehub>);
	LLFloaterReg::add("test_list_view", "floater_test_list_view.xml",&LLFloaterReg::build<LLFloaterTestListView>);
	LLFloaterReg::add("test_widgets", "floater_test_widgets.xml", &LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("top_objects", "floater_top_objects.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTopObjects>);
	
	LLFloaterReg::add("reporter", "floater_report_abuse.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterReporter>);
	LLFloaterReg::add("reset_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterResetQueue>);
	LLFloaterReg::add("region_info", "floater_region_info.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionInfo>);
	
	LLFloaterReg::add("script_debug", "floater_script_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebug>);
	LLFloaterReg::add("script_debug_output", "floater_script_debug_panel.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebugOutput>);
	LLFloaterReg::add("sell_land", "floater_sell_land.xml", &LLFloaterSellLand::buildFloater);
	LLFloaterReg::add("settings_debug", "floater_settings_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSettingsDebug>);
	LLFloaterReg::add("stats", "floater_stats.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("start_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRunQueue>);
	LLFloaterReg::add("stop_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotRunQueue>);
	LLFloaterReg::add("sl_about", "floater_about.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAbout>);
	LLFloaterReg::add("snapshot", "floater_snapshot.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSnapshot>);
	LLFloaterReg::add("search", "floater_directory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDirectory>);
	
	LLFloaterReg::add("ui_preview", "floater_ui_preview.xml", &LLFloaterReg::build<LLFloaterUIPreview>);
	LLFloaterReg::add("upload_anim", "floater_animation_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAnimPreview>, "upload");
	LLFloaterReg::add("upload_image", "floater_image_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterImagePreview>, "upload");
	LLFloaterReg::add("upload_sound", "floater_sound_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSoundPreview>, "upload");
	
	LLFloaterReg::add("voice_call", "floater_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCall>);
	
	LLFloaterReg::add("world_map", "floater_world_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWorldMap>);	

	// *NOTE: Please keep these alphabetized for easier merges
	
	// debug use only
	LLFloaterReg::add("media_remote_ctrl", "floater_media_remote.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaRemoteCtrl>);
	
	// Untested / dangerous - not for release
#if !LL_RELEASE_FOR_DOWNLOAD
	LLFloaterReg::add("buy_currency_html", "floater_html_simple.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHtmlCurrency>);
#endif

	LLFloaterReg::registerControlVariables(); // Make sure visibility and rect controls get preserved when saving
}
