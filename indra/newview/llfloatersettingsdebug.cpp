// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llfloatersettingsdebug.cpp
 * @brief floater for debugging internal viewer settings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llfloatersettingsdebug.h"
#include "llfloater.h"
//#include "llfirstuse.h"
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "llscrolllistctrl.h"
#include "llstring.h"
#include "llwindow.h"
#include "sanitycheck.h"
#include "llsearcheditor.h" // for search box
#include "lltrans.h"

LLFloaterSettingsDebug::LLFloaterSettingsDebug(const LLSD& key) 
// Polarity> Ensure the debug list is populated when opening through the toolbar button
//: LLFloater(key)
// TODO: Add a dropdown for this in the floater
:  LLFloater(key.asString().empty() ? LLSD("all") : key)
{
	mCommitCallbackRegistrar.add("SettingSelect",  boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));
	mCommitCallbackRegistrar.add("CommitSettings",	boost::bind(&LLFloaterSettingsDebug::onCommitSettings, this));
	mCommitCallbackRegistrar.add("ClickDefault",	boost::bind(&LLFloaterSettingsDebug::onClickDefault, this));
	mCommitCallbackRegistrar.add("UpdateFilter",	boost::bind(&LLFloaterSettingsDebug::onUpdateFilter, this));
	mCommitCallbackRegistrar.add("ClickCopy",	 boost::bind(&LLFloaterSettingsDebug::onCopyToClipboard, this));
	mCommitCallbackRegistrar.add("ClickSanityIcon", boost::bind(&LLFloaterSettingsDebug::onSanityCheck, this));

	// make sure that the first filter update succeeds
	mOldSearchTerm = std::string("---");
	// proper initialisation
	mCurrentControlVariable = nullptr;
	mOldControlVariable = nullptr;
}

LLFloaterSettingsDebug::~LLFloaterSettingsDebug()
{
	if (mOldControlVariable)
	{
		mOldControlVariable->getCommitSignal()->disconnect(boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));
	}
}

void LLFloaterSettingsDebug::onUpdateFilter()
{
	std::string searchTerm = mSearchSettingsInput->getValue();
	// make sure not to reselect the first item in the list on focus restore
	if (searchTerm == mOldSearchTerm)
	{
		return;
	}
	mOldSearchTerm = searchTerm;
	mSettingsScrollList->deleteAllItems();
	settings_map_t::iterator it;
	for (it = mSettingsMap.begin(); it != mSettingsMap.end(); ++it)
	{
		bool addItem = false;
		if (searchTerm.empty())
		{
			addItem = true;
		}
		else
		{
			std::string itemValue = it->second->getName();
			std::string itemComment = it->second->getComment();
			LLStringUtil::toLower(searchTerm);
			LLStringUtil::toLower(itemValue);
			if (itemValue.find(searchTerm, 0) != std::string::npos)
			{
				addItem = true;
			}
			else	// performance: broken out to save toLower calls on comments
			{
				LLStringUtil::toLower(itemComment);
				if (itemComment.find(searchTerm, 0) != std::string::npos)
				{
					addItem = true;
				}
			}
		}
		if (addItem)
		{
			LLSD item;
			item["columns"][0]["value"] = it->second->getName();
			mSettingsScrollList->addElement(item, ADD_BOTTOM, it->second);
		}
	}
	// if at least one match was found, highlight and select the topmost entry in the list
	// but only if actually a search term was given
	if (mSettingsScrollList->getItemCount() && !searchTerm.empty())
	{
		mSettingsScrollList->sortByColumnIndex(0, TRUE);
		mSettingsScrollList->selectFirstItem();
	}
	onSettingSelect();
}
BOOL LLFloaterSettingsDebug::postBuild()
{
	mSearchSettingsInput = getChild<LLSearchEditor>("search_settings_input");
	mSettingsScrollList = getChild<LLScrollListCtrl>("settings_scroll_list");
	mComment = getChild<LLTextEditor>("comment_text");
	mSpinner1 = getChild<LLSpinCtrl>("val_spinner_1");
	mSpinner2 = getChild<LLSpinCtrl>("val_spinner_2");
	mSpinner3 = getChild<LLSpinCtrl>("val_spinner_3");
	mSpinner4 = getChild<LLSpinCtrl>("val_spinner_4");
	mColorSwatch = getChild<LLColorSwatchCtrl>("val_color_swatch");
	mValText = getChild<LLLineEditor>("val_text");
	mBooleanCombo = getChild<LLRadioGroup>("boolean_combo");
	mCopyButton = getChild<LLButton>("copy_btn");
	mDefaultButton = getChild<LLButton>("default_btn");
	mSanityButton = getChild<LLButton>("sanity_warning_btn");

	mSearchSettingsInput->setFocus(TRUE); // <FS:LO> Make search textbox focused on opening
	// tried to make this an XUI callback, but keystroke_callback doesn't
	// seem to work as hoped, so build the callback manually :/ -Zi
	mSearchSettingsInput->setKeystrokeCallback(boost::bind(&LLFloaterSettingsDebug::onUpdateFilter, this));
	struct f : public LLControlGroup::ApplyFunctor
	{
		std::map<std::string, LLControlVariable*>* map;
		f(settings_map_t* m) : map(m) {}
		virtual void apply(const std::string& name, LLControlVariable* control)
		{
			if (!control->isHiddenFromSettingsEditor())
			{
				(*map)[name]=control;
			}
		}
	} func(&mSettingsMap);

	std::string key = getKey().asString();
	if (key == "all" || key == "base")
	{
		gSavedSettings.applyToAll(&func);
	}
	if (key == "all" || key == "account")
	{
		gSavedPerAccountSettings.applyToAll(&func);
	}

	onUpdateFilter();
	mSettingsScrollList->sortByColumnIndex(0,TRUE);

	if (!gSavedSettings.getBOOL("7bd9b379-962b-407f-9dd3-2d5ce9eba39a"))
	{
		LLSD args;
		args["MESSAGE"] = LLTrans::getString("DebugSettingsWarning");
		LLNotificationsUtil::add("SystemMessageTip", args);
		gSavedSettings.setBOOL("7bd9b379-962b-407f-9dd3-2d5ce9eba39a", TRUE);
	}

	return TRUE;
}

LLControlVariable* LLFloaterSettingsDebug::getControlVariable() const
{
	LLControlVariable* controlp = nullptr;
	LLScrollListItem* item = mSettingsScrollList->getFirstSelected();
	if (item)
	{
		controlp = static_cast<LLControlVariable*>(item->getUserdata());
	}
	return controlp;
}
void LLFloaterSettingsDebug::onSettingSelect()
{
	mCurrentControlVariable = getControlVariable();
	if (mOldControlVariable != mCurrentControlVariable)
	{
		// unbind change control signal from previously selected control
		if (mOldControlVariable)
		{
			mOldControlVariable->getCommitSignal()->disconnect(boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));
		}
		// bind change control signal, so we can see updates to the current control in realtime
		if (mCurrentControlVariable)
		{
			mCurrentControlVariable->getCommitSignal()->connect(boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));
		}
		mOldControlVariable = mCurrentControlVariable;
	}
	updateControl();
}
void LLFloaterSettingsDebug::draw()
{
	if (mCurrentControlVariable)
	{
		// check for changes in control visibility, like RLVa does
		if (mCurrentControlVariable->isHiddenFromSettingsEditor() != mOldVisibility)
		{
			updateControl();
		}
	}
	LLFloater::draw();
}

void LLFloaterSettingsDebug::onSanityCheck() const
{
	SanityCheck::instance().onSanity(mCurrentControlVariable);
}

void LLFloaterSettingsDebug::onCommitSettings() const
{
	if (!mCurrentControlVariable)
		return;

	LLVector3 vector;
	LLVector3d vectord;
	LLVector4 vector4; // <Black Dragon:NiranV> Vector4
	LLRect rect;
	LLColor4 col4;
	LLColor3 col3;

	switch(mCurrentControlVariable->type())
	{		
	  case TYPE_U32:
		mCurrentControlVariable->set(mSpinner1->getValue());
		break;
	  case TYPE_S32:
		mCurrentControlVariable->set(mSpinner1->getValue());
		break;
	  case TYPE_F32:
		mCurrentControlVariable->set(LLSD(mSpinner1->getValue().asReal()));
		break;
	  case TYPE_BOOLEAN:
		mCurrentControlVariable->set(mBooleanCombo->getValue());
		break;
	  case TYPE_STRING:
		mCurrentControlVariable->set(LLSD(mValText->getValue().asString()));
		break;
	  case TYPE_VEC3:
		vector.mV[VX] = static_cast<F32>(mSpinner1->getValue().asReal());
		vector.mV[VY] = static_cast<F32>(mSpinner2->getValue().asReal());
		vector.mV[VZ] = static_cast<F32>(mSpinner3->getValue().asReal());
		mCurrentControlVariable->set(vector.getValue());
		break;
	  case TYPE_VEC3D:
		vectord.mdV[VX] = mSpinner1->getValue().asReal();
		vectord.mdV[VY] = mSpinner2->getValue().asReal();
		vectord.mdV[VZ] = mSpinner3->getValue().asReal();
		mCurrentControlVariable->set(vectord.getValue());
		break;
	  case TYPE_RECT:
		rect.mLeft = mSpinner1->getValue().asInteger();
		rect.mRight = mSpinner2->getValue().asInteger();
		rect.mBottom = mSpinner3->getValue().asInteger();
		rect.mTop = mSpinner4->getValue().asInteger();
		mCurrentControlVariable->set(rect.getValue());
		break;
	  case TYPE_COL4:
		col3.setValue(mColorSwatch->getValue());
		col4 = LLColor4(col3, static_cast<F32>(mSpinner4->getValue().asReal()));
		mCurrentControlVariable->set(col4.getValue());
		break;
	  case TYPE_COL3:
		mCurrentControlVariable->set(mColorSwatch->getValue());
		break;
	  // <Black Dragon:NiranV> Vector4
	  case TYPE_VEC4:
		vector4.mV[VX] = (F32)getChild<LLUICtrl>("val_spinner_1")->getValue().asReal();
		vector4.mV[VY] = (F32)getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
		vector4.mV[VZ] = (F32)getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
		vector4.mV[VW] = (F32)getChild<LLUICtrl>("val_spinner_4")->getValue().asReal();
		mCurrentControlVariable->set(vector4.getValue());
	  default:
		break;
	}
	if (!mCurrentControlVariable->isSane())
	{
		onSanityCheck();
}
}

void LLFloaterSettingsDebug::onClickDefault()
{
	if (mCurrentControlVariable)
	{
		mCurrentControlVariable->resetToDefault(true);
		updateControl();
	}
}

void LLFloaterSettingsDebug::onCopyToClipboard() const
{
	if (mCurrentControlVariable)
	{
		getWindow()->copyTextToClipboard(utf8str_to_wstring(mCurrentControlVariable->getName()));
		LLNotificationsUtil::add("ControlNameCopiedToClipboard");
	}
}

// we've switched controls, so update spinners, etc.
void LLFloaterSettingsDebug::updateControl()
{
	if (!mSpinner1 || !mSpinner2 || !mSpinner3 || !mSpinner4 || !mColorSwatch)
	{
		LL_WARNS() << "Could not find all desired controls by name"
			<< LL_ENDL;
		return;
	}

	mSpinner1->setVisible(FALSE);
	mSpinner2->setVisible(FALSE);
	mSpinner3->setVisible(FALSE);
	mSpinner4->setVisible(FALSE);
	mColorSwatch->setVisible(FALSE);
	mValText->setVisible( FALSE);
	mComment->setText(LLStringUtil::null);
	mCopyButton->setEnabled(FALSE);
	mDefaultButton->setEnabled(FALSE);
	mBooleanCombo->setVisible(FALSE);
	mSanityButton->setVisible(FALSE);

	if (mCurrentControlVariable)
	{
	
		mCopyButton->setEnabled(TRUE);
		mSanityButton->setVisible(!mCurrentControlVariable->isSane());

		eControlType type=mCurrentControlVariable->type();

		mComment->setText(mCurrentControlVariable->getName()+std::string(": ")+mCurrentControlVariable->getComment());
		mSpinner1->setMaxValue(F32_MAX);
		mSpinner2->setMaxValue(F32_MAX);
		mSpinner3->setMaxValue(F32_MAX);
		mSpinner4->setMaxValue(F32_MAX);
		mSpinner1->setMinValue(-F32_MAX);
		mSpinner2->setMinValue(-F32_MAX);
		mSpinner3->setMinValue(-F32_MAX);
		mSpinner4->setMinValue(-F32_MAX);
		if (!mSpinner1->hasFocus())
		{
			mSpinner1->setIncrement(0.1f);
		}
		if (!mSpinner2->hasFocus())
		{
			mSpinner2->setIncrement(0.1f);
		}
		if (!mSpinner3->hasFocus())
		{
			mSpinner3->setIncrement(0.1f);
		}
		if (!mSpinner4->hasFocus())
		{
			mSpinner4->setIncrement(0.1f);
		}

		LLSD sd = mCurrentControlVariable->get();
		switch(type)
		{
		  case TYPE_U32:
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setValue(sd);
				mSpinner1->setMinValue(static_cast<F32>(U32_MIN));
				mSpinner1->setMaxValue(static_cast<F32>(U32_MAX));
				mSpinner1->setIncrement(1.f);
				mSpinner1->setPrecision(0);
			}
			break;
		  case TYPE_S32:
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setValue(sd);
				mSpinner1->setMinValue(static_cast<F32>(S32_MIN));
				mSpinner1->setMaxValue(static_cast<F32>(S32_MAX));
				mSpinner1->setIncrement(1.f);
				mSpinner1->setPrecision(0);
			}
			break;
		  case TYPE_F32:
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setPrecision(3);
				mSpinner1->setValue(sd);
			}
			break;
		  case TYPE_BOOLEAN:
			mBooleanCombo->setVisible(TRUE);
			if (!mBooleanCombo->hasFocus())
			{
				if (sd.asBoolean())
				{
					mBooleanCombo->setValue(LLSD("true"));
				}
				else
				{
					mBooleanCombo->setValue(LLSD(""));
				}
			}
			break;
		  case TYPE_STRING:
			mValText->setVisible( TRUE);
			if (!mValText->hasFocus())
			{
				mValText->setValue(sd);
			}
			break;
		  case TYPE_VEC3:
		  {
			LLVector3 v;
			v.setValue(sd);
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("X"));
			mSpinner2->setVisible(TRUE);
			mSpinner2->setLabel(std::string("Y"));
			mSpinner3->setVisible(TRUE);
			mSpinner3->setLabel(std::string("Z"));
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setPrecision(3);
				mSpinner1->setValue(v[VX]);
			}
			if (!mSpinner2->hasFocus())
			{
				mSpinner2->setPrecision(3);
				mSpinner2->setValue(v[VY]);
			}
			if (!mSpinner3->hasFocus())
			{
				mSpinner3->setPrecision(3);
				mSpinner3->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_VEC3D:
		  {
			LLVector3d v;
			v.setValue(sd);
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("X"));
			mSpinner2->setVisible(TRUE);
			mSpinner2->setLabel(std::string("Y"));
			mSpinner3->setVisible(TRUE);
			mSpinner3->setLabel(std::string("Z"));
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setPrecision(3);
				mSpinner1->setValue(v[VX]);
			}
			if (!mSpinner2->hasFocus())
			{
				mSpinner2->setPrecision(3);
				mSpinner2->setValue(v[VY]);
			}
			if (!mSpinner3->hasFocus())
			{
				mSpinner3->setPrecision(3);
				mSpinner3->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_RECT:
		  {
			LLRect r;
			r.setValue(sd);
			mSpinner1->setVisible(TRUE);
			mSpinner1->setLabel(std::string("Left"));
			mSpinner2->setVisible(TRUE);
			mSpinner2->setLabel(std::string("Right"));
			mSpinner3->setVisible(TRUE);
			mSpinner3->setLabel(std::string("Bottom"));
			mSpinner4->setVisible(TRUE);
			mSpinner4->setLabel(std::string("Top"));
			if (!mSpinner1->hasFocus())
			{
				mSpinner1->setPrecision(0);
				mSpinner1->setValue(r.mLeft);
			}
			if (!mSpinner2->hasFocus())
			{
				mSpinner2->setPrecision(0);
				mSpinner2->setValue(r.mRight);
			}
			if (!mSpinner3->hasFocus())
			{
				mSpinner3->setPrecision(0);
				mSpinner3->setValue(r.mBottom);
			}
			if (!mSpinner4->hasFocus())
			{
				mSpinner4->setPrecision(0);
				mSpinner4->setValue(r.mTop);
			}

			mSpinner1->setMinValue(static_cast<F32>(S32_MIN));
			mSpinner1->setMaxValue(static_cast<F32>(S32_MAX));
			mSpinner1->setIncrement(1.f);

			mSpinner2->setMinValue(static_cast<F32>(S32_MIN));
			mSpinner2->setMaxValue(static_cast<F32>(S32_MAX));
			mSpinner2->setIncrement(1.f);

			mSpinner3->setMinValue(static_cast<F32>(S32_MIN));
			mSpinner3->setMaxValue(static_cast<F32>(S32_MAX));
			mSpinner3->setIncrement(1.f);

			mSpinner4->setMinValue(static_cast<F32>(S32_MIN));
			mSpinner4->setMaxValue(static_cast<F32>(S32_MAX));
			mSpinner4->setIncrement(1.f);
			break;
		  }
		  case TYPE_COL4:
		  {
			LLColor4 clr;
			clr.setValue(sd);
			mColorSwatch->setVisible(TRUE);
			// only set if changed so color picker doesn't update
			if(clr != LLColor4(mColorSwatch->getValue()))
			{
				mColorSwatch->set(LLColor4(sd), TRUE, FALSE);
			}
			mSpinner4->setVisible(TRUE);
			mSpinner4->setLabel(std::string("Alpha"));
			if (!mSpinner4->hasFocus())
			{
				mSpinner4->setPrecision(3);
				mSpinner4->setMinValue(0.0);
				mSpinner4->setMaxValue(1.f);
				mSpinner4->setValue(clr.mV[VALPHA]);
			}
			break;
		  }
		  case TYPE_COL3:
		  {
			LLColor3 clr;
			clr.setValue(sd);
			mColorSwatch->setVisible(TRUE);
			mColorSwatch->setValue(sd);
			break;
		  }
		  // <Black Dragon:NiranV> Vector4
		  case TYPE_VEC4:
		  {
			  LLVector4 v;
			  v.setValue(sd);
			  mSpinner1->setVisible(TRUE);
			  mSpinner1->setLabel(std::string("X"));
			  mSpinner2->setVisible(TRUE);
			  mSpinner2->setLabel(std::string("Y"));
			  mSpinner3->setVisible(TRUE);
			  mSpinner3->setLabel(std::string("Z"));
			  mSpinner4->setVisible(TRUE);
			  mSpinner4->setLabel(std::string("W"));
			  if (!mSpinner1->hasFocus())
			  {
				  mSpinner1->setPrecision(3);
				  mSpinner1->setValue(v[VX]);
			  }
			  if (!mSpinner2->hasFocus())
			  {
				  mSpinner2->setPrecision(3);
				  mSpinner2->setValue(v[VY]);
			  }
			  if (!mSpinner3->hasFocus())
			  {
				  mSpinner3->setPrecision(3);
				  mSpinner3->setValue(v[VZ]);
			  }
			  if (!mSpinner4->hasFocus())
			  {
				  mSpinner4->setPrecision(3);
				  mSpinner4->setValue(v[VW]);
			  }
			  break;
		  }
		  default:
			mComment->setText(std::string("unknown"));
			break;
		}
	}
}
