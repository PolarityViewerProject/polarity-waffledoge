// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file lltoolselectrect.cpp
 * @brief A tool to select multiple objects with a screen-space rectangle.
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

// File includes
#include "lltoolselectrect.h"

// Library includes
#include "llgl.h"
#include "llrender.h"
#include "llui.h"
#include "llworld.h"

// Viewer includes

#include "llagent.h"
#include "lldrawable.h"
#include "llglheaders.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

// [RLVa:KB] - Checked: 2010-04-11 (RLVa-1.2.0e)
#include "rlvactions.h"
#include "rlvhandler.h"
#include "rlvhelper.h"
// [/RLVa:KB]

// Globals
const S32 SLOP_RADIUS = 5;


//
// Member functions
//

LLToolSelectRect::LLToolSelectRect( LLToolComposite* composite )
	:
	LLToolSelect( composite ),
	mDragStartX(0),
	mDragStartY(0),
	mDragEndX(0),
	mDragEndY(0),
	mDragLastWidth(0),
	mDragLastHeight(0),
	mMouseOutsideSlop(FALSE)

{ }


void dialog_refresh_all(void);

BOOL LLToolSelectRect::handleMouseDown(S32 x, S32 y, MASK mask)
{
	handlePick(gViewerWindow->pickImmediate(x, y, TRUE, FALSE));

	LLTool::handleMouseDown(x, y, mask);

	return mPick.getObject().notNull();
}

void LLToolSelectRect::handlePick(const LLPickInfo& pick)
{
	mPick = pick;

	// start dragging rectangle
	setMouseCapture( TRUE );

	mDragStartX = pick.mMousePt.mX;
	mDragStartY = pick.mMousePt.mY;
	mDragEndX = pick.mMousePt.mX;
	mDragEndY = pick.mMousePt.mY;

	mMouseOutsideSlop = FALSE;
}


BOOL LLToolSelectRect::handleMouseUp(S32 x, S32 y, MASK mask)
{
	setMouseCapture( FALSE );

	if(	mMouseOutsideSlop )
	{
		mDragLastWidth = 0;
		mDragLastHeight = 0;

		mMouseOutsideSlop = FALSE;
		
		if (mask == MASK_CONTROL)
		{
			LLSelectMgr::getInstance()->deselectHighlightedObjects();
		}
		else
		{
			LLSelectMgr::getInstance()->selectHighlightedObjects();
		}
		return TRUE;
	}
	else
	{
		return LLToolSelect::handleMouseUp(x, y, mask);
	}
}


BOOL LLToolSelectRect::handleHover(S32 x, S32 y, MASK mask)
{
	if(	hasMouseCapture() )
	{
		if (mMouseOutsideSlop || outsideSlop(x, y, mDragStartX, mDragStartY))
		{
			if (!mMouseOutsideSlop && !(mask & MASK_SHIFT) && !(mask & MASK_CONTROL))
			{
				// just started rect select, and not adding to current selection
				LLSelectMgr::getInstance()->deselectAll();
			}
			mMouseOutsideSlop = TRUE;
			mDragEndX = x;
			mDragEndY = y;

			handleRectangleSelection(x, y, mask);
		}
		else
		{
			return LLToolSelect::handleHover(x, y, mask);
		}

		LL_DEBUGS("UserInput") << "hover handled by LLToolSelectRect (active)" << LL_ENDL;		
	}
	else
	{
		LL_DEBUGS("UserInput") << "hover handled by LLToolSelectRect (inactive)" << LL_ENDL;		
	}

	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}


void LLToolSelectRect::draw()
{
	if(	hasMouseCapture() && mMouseOutsideSlop)
	{
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			gGL.color4f(1.f, 0.f, 0.f, 1.f);
		}
		else
		{
			gGL.color4f(1.f, 1.f, 0.f, 1.f);
		}
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gl_rect_2d(
			llmin(mDragStartX, mDragEndX),
			llmax(mDragStartY, mDragEndY),
			llmax(mDragStartX, mDragEndX),
			llmin(mDragStartY, mDragEndY),
			FALSE);
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			gGL.color4f(1.f, 0.f, 0.f, 0.1f);
		}
		else
		{
			gGL.color4f(1.f, 1.f, 0.f, 0.1f);
		}
		gl_rect_2d(
			llmin(mDragStartX, mDragEndX),
			llmax(mDragStartY, mDragEndY),
			llmax(mDragStartX, mDragEndX),
			llmin(mDragStartY, mDragEndY));
	}
}

// true if x,y outside small box around start_x,start_y
BOOL LLToolSelectRect::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;

	return (dx <= -SLOP_RADIUS || SLOP_RADIUS <= dx || dy <= -SLOP_RADIUS || SLOP_RADIUS <= dy);
}

// Returns true if you got at least one object
void LLToolSelectRect::handleRectangleSelection(S32 x, S32 y, MASK mask)
{
	// [RLVa:KB] - Checked: 2010-11-29 (RLVa-1.3.0c) | Modified: RLVa-1.3.0c
	// Block rectangle selection if:
	//   - prevented from editing and no exceptions are set (see below for the case where exceptions are set)
	//   - prevented from interacting at all
	if ((rlv_handler_t::isEnabled()) &&
		(((gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) && (!gRlvHandler.hasException(RLV_BHVR_EDIT))) ||
		(gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT))))
	{
		return;
	}
	// [/RLVa:KB]

	LLVector3 av_pos = gAgent.getPositionAgent();
	F32 select_dist_squared = gSavedSettings.getF32("MaxSelectDistance");
	select_dist_squared = select_dist_squared * select_dist_squared;

	BOOL deselect = (mask == MASK_CONTROL);
	S32 left = llmin(x, mDragStartX);
	S32 right = llmax(x, mDragStartX);
	S32 top = llmax(y, mDragStartY);
	S32 bottom = llmin(y, mDragStartY);

	left = ll_round((F32)left * LLUI::getScaleFactor().mV[VX]);
	right = ll_round((F32)right * LLUI::getScaleFactor().mV[VX]);
	top = ll_round((F32)top * LLUI::getScaleFactor().mV[VY]);
	bottom = ll_round((F32)bottom * LLUI::getScaleFactor().mV[VY]);

	F32 old_far_plane = LLViewerCamera::getInstance()->getFar();
	F32 old_near_plane = LLViewerCamera::getInstance()->getNear();

	S32 width = right - left + 1;
	S32 height = top - bottom + 1;

	BOOL grow_selection = FALSE;
	BOOL shrink_selection = FALSE;

	if (height > mDragLastHeight || width > mDragLastWidth)
	{
		grow_selection = TRUE;
	}
	if (height < mDragLastHeight || width < mDragLastWidth)
	{
		shrink_selection = TRUE;
	}

	if (!grow_selection && !shrink_selection)
	{
		// nothing to do
		return;
	}

	mDragLastHeight = height;
	mDragLastWidth = width;

	S32 center_x = (left + right) / 2;
	S32 center_y = (top + bottom) / 2;

	// save drawing mode
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();

	BOOL limit_select_distance = gSavedSettings.getBOOL("LimitSelectDistance");
	if (limit_select_distance)
	{
		// ...select distance from control
		LLVector3 relative_av_pos = av_pos;
		relative_av_pos -= LLViewerCamera::getInstance()->getOrigin();

		F32 new_far = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() + gSavedSettings.getF32("MaxSelectDistance");
		F32 new_near = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() - gSavedSettings.getF32("MaxSelectDistance");

		new_near = llmax(new_near, 0.1f);

		LLViewerCamera::getInstance()->setFar(new_far);
		LLViewerCamera::getInstance()->setNear(new_near);
	}
	// [RLVa:KB] - Checked: 2010-04-11 (RLVa-1.2.0e) | Modified: RLVa-1.0.0g
	if (gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH))
	{
		static RlvCachedBehaviourModifier<float> s_nFartouchDist(RLV_MODIFIER_FARTOUCHDIST);

		// We'll allow drag selection under fartouch, but only within the fartouch range
		// (just copy/paste the code above us to make that work, thank you Lindens!)
		LLVector3 relative_av_pos = av_pos;
		relative_av_pos -= LLViewerCamera::getInstance()->getOrigin();

		F32 new_far = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() + s_nFartouchDist;
		F32 new_near = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() - s_nFartouchDist;

		new_near = llmax(new_near, 0.1f);

		LLViewerCamera::getInstance()->setFar(new_far);
		LLViewerCamera::getInstance()->setNear(new_near);

		// Usurp these two
		limit_select_distance = TRUE;
		select_dist_squared = s_nFartouchDist * s_nFartouchDist;
	}
	// [/RLVa:KB]
	LLViewerCamera::getInstance()->setPerspective(FOR_SELECTION,
		center_x - width / 2, center_y - height / 2, width, height,
		limit_select_distance);

	if (shrink_selection)
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* vobjp)
			{
				LLDrawable* drawable = vobjp->mDrawable;
				if (!drawable || vobjp->getPCode() != LL_PCODE_VOLUME || vobjp->isAttachment())
				{
					return true;
				}
				S32 result = LLViewerCamera::getInstance()->sphereInFrustum(drawable->getPositionAgent(), drawable->getRadius());
				switch (result)
				{
				case 0:
					LLSelectMgr::getInstance()->unhighlightObjectOnly(vobjp);
					break;
				case 1:
					// check vertices
					if (!LLViewerCamera::getInstance()->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						LLSelectMgr::getInstance()->unhighlightObjectOnly(vobjp);
					}
					break;
				default:
					break;
				}
				return true;
			}
		} func;
		LLSelectMgr::getInstance()->getHighlightedObjects()->applyToObjects(&func);
	}

	if (grow_selection)
	{
		std::vector<LLDrawable*> potentials;

		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* region = *iter;
			for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
			{
				LLSpatialPartition* part = region->getSpatialPartition(i);
				if (part)
				{
					part->cull(*LLViewerCamera::getInstance(), &potentials, TRUE);
				}
			}
		}

		for (std::vector<LLDrawable*>::iterator iter = potentials.begin();
			iter != potentials.end(); iter++)
		{
			LLDrawable* drawable = *iter;
			// <FS:Ansariel> FIRE-15206: Crash fixing
			if (!drawable)
			{
				continue;
			}
			// </FS:Ansariel>
			LLViewerObject* vobjp = drawable->getVObj();

			// <FS:Ansariel> FIRE-15206: Crash fixing
			//if (!drawable || !vobjp ||
			if (!vobjp ||
				// </FS:Ansariel>
				vobjp->getPCode() != LL_PCODE_VOLUME ||
				vobjp->isAttachment() ||
				(deselect && !vobjp->isSelected()))
			{
				continue;
			}

			if (limit_select_distance && dist_vec_squared(drawable->getWorldPosition(), av_pos) > select_dist_squared)
			{
				continue;
			}

			// [RLVa:KB] - Checked: 2010-11-29 (RLVa-1.3.0c) | Added: RLVa-1.3.0c
			if ((RlvActions::isRlvEnabled()) && (!RlvActions::canEdit(vobjp)))
			{
				continue;
			}
			// [/RLVa:KB]

			S32 result = LLViewerCamera::getInstance()->sphereInFrustum(drawable->getPositionAgent(), drawable->getRadius());
			if (result)
			{
				switch (result)
				{
				case 1:
					// check vertices
					if (LLViewerCamera::getInstance()->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						LLSelectMgr::getInstance()->highlightObjectOnly(vobjp);
					}
					break;
				case 2:
					LLSelectMgr::getInstance()->highlightObjectOnly(vobjp);
					break;
				default:
					break;
				}
			}
		}
	}

	// restore drawing mode
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	// restore camera
	LLViewerCamera::getInstance()->setFar(old_far_plane);
	LLViewerCamera::getInstance()->setNear(old_near_plane);
	gViewerWindow->setup3DRender();
}

//
// Static functions
//
