/**
 * @file pvaligntool.h
 * @brief A tool to align in-world objects.
 * Author: Qarl (Karl Stiefvater)
 * Re-implented by Xenhat Liamano
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
 * Portions Copyright (C)
 *  Qarl (Karl Stiefvater)
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

#ifndef PV_ALIGN_TOOL_H
#define PV_ALIGN_TOOL_H

#include "llsingleton.h"
#include "lltool.h"
#include "llbbox.h"

class LLViewerObject;
class LLPickInfo;
class LLToolSelectRect;

class QToolAlign
:	public LLTool, public LLSingleton<QToolAlign>
{
	LLSINGLETON(QToolAlign);
	~QToolAlign() {}
    
public:
	void	handleSelect() override;
	void	handleDeselect() override;
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL    handleHover(S32 x, S32 y, MASK mask) override;
	void	render() override;

	static void pickCallback(const LLPickInfo& pick_info);

private:
	void            align();
	void            computeManipulatorSize();
	void            renderManipulators();
	BOOL            findSelectedManipulator(S32 x, S32 y);
	
	LLBBox          mBBox;
	F32             mManipulatorSize;
	S32             mHighlightedAxis;
	F32             mHighlightedDirection;
	bool            mForce;
};

#endif // Q_QTOOLALIGN_H
