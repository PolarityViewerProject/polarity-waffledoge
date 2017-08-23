/**
 * @file pvrandom.h
 * @brief functions related to getting random things
 * Based on FSData by Techwolf Lupindo
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
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

#ifndef PV_RANDOM_H
#define PV_RANDOM_H

#pragma once

#include "llsingleton.h"

class PVRandom: public LLSingleton<PVRandom>
{
	LLSINGLETON_EMPTY_CTOR(PVRandom);
public:
	// get random string from a std::vector<std::string>
	std::string getRandomElement(const std::vector<std::string>& vector) const;
};

#endif // PV_RANDOM_H
