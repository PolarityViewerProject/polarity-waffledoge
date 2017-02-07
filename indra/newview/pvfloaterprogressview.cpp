// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvfloaterprogressview.cpp
 * @brief custom progress view implementation based on Alchemy Viewer's llfloaterprogressview
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2016 Xenhat Liamano
 * Portions Copyright (C)
 *  2014, Cinder Roxley <cinder@sdf.org>
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

#include "llviewerprecompiledheaders.h"
#include "pvfloaterprogressview.h"

#include "llfloaterreg.h"
#include "llprogressbar.h"
#include "lltextbase.h"

#include "llagent.h"

LLFloaterProgressView::LLFloaterProgressView(const LLSD& key)
:	LLFloater(key)
,	mProgressBar(nullptr)
,	mProgressText(nullptr)
,	mLocationText(nullptr)
,	mCancelBtn(nullptr)
{
	mCommitCallbackRegistrar.add("cancel", boost::bind(&LLFloaterProgressView::onCommitCancel, this, _1));
}

LLFloaterProgressView::~LLFloaterProgressView()
{
}

BOOL LLFloaterProgressView::postBuild()
{
	mProgressBar = getChild<LLProgressBar>("progress_bar");
	mProgressText = getChild<LLTextBase>("progress_text");
	mLocationText = getChild<LLTextBase>("location");
	mCancelBtn = getChild<LLButton>("cancel_btn");
	return TRUE;
}

void LLFloaterProgressView::setRegion(const std::string& region)
{
	if (region.empty())
		mLocationText->setText(getString("teleporting"));
	else
	{
		LLStringUtil::format_map_t arg;
		arg["REGION"] = region;
		mLocationText->setText(getString("loc_fmt", arg));
	}
}

void LLFloaterProgressView::setProgressText(const std::string& text)
{
	mProgressText->setValue(text);
}

void LLFloaterProgressView::setProgressPercent(const F32 percent)
{
	mProgressBar->setValue(percent);
}

void LLFloaterProgressView::setProgressCancelButtonVisible(BOOL visible, const std::string& label)
{
	mCancelBtn->setVisible(visible);
	mCancelBtn->setEnabled(visible);
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}

void LLFloaterProgressView::onCommitCancel(LLUICtrl* ctrl)
{
	gAgent.teleportCancel();
	ctrl->setEnabled(FALSE);
}
