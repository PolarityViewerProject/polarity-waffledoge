/**
 * @file pvfloaterprogressview.h
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

#ifndef PV_FLOATERPROGRESSVIEW_H
#define PV_FLOATERPROGRESSVIEW_H

#pragma once

#include "llfloater.h"

class LLButton;
class LLProgressBar;
class LLTextBase;

class LLFloaterProgressView : public LLFloater
{
public:
	LLFloaterProgressView(const LLSD& key);
	BOOL postBuild() override;
	void setProgressCancelButtonVisible(BOOL visible, const std::string& label = LLStringUtil::null);
	void setProgressText(const std::string& text);
	void setProgressPercent(const F32 percent);
	void setRegion(const std::string& region = LLStringUtil::null);
	
private:
	~LLFloaterProgressView();
	void onCommitCancel(LLUICtrl* ctrl);
	
	LLProgressBar* mProgressBar;
	LLTextBase* mProgressText;
	LLTextBase* mLocationText;
	LLButton* mCancelBtn;
};

#endif // LL_FLOATERPROGRESSVIEW_H
