/**
 * @file pvaligntool.h
 * @brief A tool to align in-world objects.
 * Author: Qarl (Karl Stiefvater)
 * Re-implented by Xenhat Liamano
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
 * Portions Copyright (C)
 *  2011 Wolfspirit Magi
 *  2011-2013 Techwolf Lupindo
 *  2012 Ansariel Hiller @ Second Life
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

#pragma once

#include "lltool.h"
#include "llbbox.h"

class LLViewerObject;
class LLPickInfo;
class LLToolSelectRect;

class QToolAlign
:	public LLTool, public LLSingleton<QToolAlign>
{
public:
	QToolAlign();
	virtual ~QToolAlign();

	virtual void	handleSelect();
	virtual void	handleDeselect();
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	render();

	static void pickCallback(const LLPickInfo& pick_info);

private:
	void			align();
	void			computeManipulatorSize();
	void			renderManipulators();
	BOOL			findSelectedManipulator(S32 x, S32 y);

	LLBBox		mBBox;
	F32			 mManipulatorSize;
	S32			 mHighlightedAxis;
	F32			 mHighlightedDirection;
	BOOL			mForce;
};
