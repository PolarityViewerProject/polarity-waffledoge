// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file pvsearchseparator.cpp
 * @brief Configurable inventory search separator
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2017 Xenhat Liamano
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#include "pvsearchseparator.h"
#include "llviewercontrol.h"

PVSearchUtil*		gPVSearchUtil = NULL;

U32 PVSearchUtil::PVSearchSeparatorSelected = gPVSearchUtil->separator_space;

U32 PVSearchUtil::getSearchSeparatorFromSettings()
{
	static LLCachedControl<U32> settings_separator(gSavedSettings, "PVUI_SubstringSearchSeparator", separator_space);
	LL_DEBUGS() << "Search separator index from settings: '" << settings_separator << "'" << LL_ENDL;
	return settings_separator;
}

void PVSearchUtil::setSearchSeparator(const U32 separator_in_u32)
{
	PVSearchSeparatorSelected = separator_in_u32;
	LL_DEBUGS() << "Setting search separator to '" << separator_in_u32 << "'" << "('" << getSearchSeparator() << "')" << LL_ENDL;
	gSavedSettings.setU32("PVUI_SubstringSearchSeparator", separator_in_u32);
}

std::string PVSearchUtil::getSearchSeparator()
{
	auto separator = gPVSearchUtil->PVSearchSeparatorAssociation[PVSearchSeparatorSelected];
	LL_DEBUGS() << "Search separator from runtime: '" << separator << "'" << LL_ENDL;
	return separator;
}

std::string PVSearchUtil::getSearchSeparator(const U32 separator_to_get_u32) const
{
	PVSearchSeparatorSelected = separator_to_get_u32;
	return getSearchSeparator();
}
