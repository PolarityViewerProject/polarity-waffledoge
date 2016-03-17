/**
 * @file pvloadinganim.h
 * @brief Configurable loading animation widget.
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (C) 2016 Doug Falta
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

#ifndef PV_PVLOADINGANIM_H
#define PV_PVLOADINGANIM_H

#include "llview.h"

class PVLoadingAnim : public LLView
{
public:

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<S32> number_sprites;
		Optional<LLUIImage*> sprite_image;
		Optional<LLUIColor> color;

		Params();
	};
	PVLoadingAnim(const Params&);
	~PVLoadingAnim();

	virtual void draw();

private:
	S32 mNumSprites;
	S32 mSpriteDiameter;
	S32 mSpriteSeparation;
	LLUIColor mColor;
	LLPointer<LLUIImage> mSpriteImage;
};

#endif
