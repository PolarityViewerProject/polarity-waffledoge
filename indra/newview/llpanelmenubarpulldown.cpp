/*
* @file llpanelmenubarpulldown.cpp
* @brief Base for menu bar flyouts
*
* Copyright (c) 2017, Cinder Roxley <cinder@sdf.org>
*
* Permission is hereby granted, free of charge, to any person or organization
* obtaining a copy of the software and accompanying documentation covered by
* this license (the "Software") to use, reproduce, display, distribute,
* execute, and transmit the Software, and to prepare derivative works of the
* Software, and to permit third-parties to whom the Software is furnished to
* do so, all subject to the following:
*
* The copyright notices in the Software and this entire statement, including
* the above license grant, this restriction and the following disclaimer,
* must be included in all copies of the Software, in whole or in part, and
* all derivative works of the Software, unless such copies or derivative
* works are solely in the form of machine-executable object code generated by
* a source language processor.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*/

#include "llviewerprecompiledheaders.h"
#include "llpanelmenubarpulldown.h"

#include "llframetimer.h"

LLPanelMenuBarPulldown::LLPanelMenuBarPulldown()
{
	mHoverTimer.stop();
}

void LLPanelMenuBarPulldown::draw()
{
	F32 alpha = mHoverTimer.getStarted()
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), AUTO_CLOSE_FADE_START_TIME_SEC,
			AUTO_CLOSE_TOTAL_TIME_SEC, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}

	LLPanel::draw();
}

void LLPanelMenuBarPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x, y, mask);
}

void LLPanelMenuBarPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x, y, mask);
}

void LLPanelMenuBarPulldown::onTopLost()
{
	setVisible(FALSE);
}

void LLPanelMenuBarPulldown::onVisibilityChange(BOOL new_visibility)
{
	new_visibility ? mHoverTimer.start() : mHoverTimer.stop();
}
