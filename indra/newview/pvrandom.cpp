/**
* @file pvrandom.cpp
* @brief functions related to getting random things
* Based on FSData by Techwolf Lupindo
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "pvrandom.h"

std::string PVRandom::getRandomElement(const std::vector<std::string>& vector) const
{
	size_t r = ll_rand(vector.size());
	return (r) ? vector[r] : "";
}
