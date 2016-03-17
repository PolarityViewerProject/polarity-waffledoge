/**
 * @file pvloadinganim.cpp
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

#include "linden_common.h"

#include "pvloadinganim.h"

#include "llmath.h"
#include "llui.h"

PVLoadingAnim::Params::Params()
:	number_sprites("number_sprites"),
	sprite_image("sprite_image"),
	color("color")
{
	changeDefault(follows.flags, FOLLOWS_TOP | FOLLOWS_LEFT);
}

///////////////////////////////////////////////////////////////////////////////////

PVLoadingAnim::PVLoadingAnim(const Params& p)
:	LLView(p),
	mNumSprites(p.number_sprites),
	mSpriteImage(p.sprite_image),
	mColor(p.color())
{
	mSpriteSeparation = getRect().getWidth() / mNumSprites;
	mSpriteDiameter = static_cast<S32>(mSpriteSeparation / 5.0f * 4.0f);
}

PVLoadingAnim::~PVLoadingAnim()
{
	mSpriteImage = NULL;
}

void PVLoadingAnim::draw()
{
	static LLTimer timer;
	// <Polarity> PLVR-221: Port Prismata's public domain loading animation
	F32 half_height = (getRect().getHeight() - mSpriteDiameter) / 2;
	U64 time_step = floor(timer.getElapsedTimeF64() * 60);
	for (size_t i = 0; i < mNumSprites; i++)
	{
		const F32 sprite_y = half_height + half_height * sin(fmod(time_step * (i / 500.0 + 0.02), F_TWO_PI));
		mSpriteImage->draw(i * mSpriteSeparation, sprite_y, mSpriteDiameter, mSpriteDiameter, mColor.get());
	}
	// </Polarity> PLVR-221

	LLView::draw();
}

static LLDefaultChildRegistry::Register<PVLoadingAnim> r("loading_anim");
