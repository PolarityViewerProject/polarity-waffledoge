/**
 * @file pvsearchseparator.h
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
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#pragma once

#ifndef PV_SEARCHSEPARATOR_H
#define PV_SEARCHSEPARATOR_H

#include "llerror.h"
#include "llsingleton.h"

class PVSearchUtil : public LLSingleton <PVSearchUtil>
{
	LOG_CLASS(PVSearchUtil);
	LLSINGLETON_EMPTY_CTOR(PVSearchUtil);

public:
	// refresh from settings
	static U32 getSearchSeparatorFromSettings();
	static void setSearchSeparator(const U32 separator_in_u32);
	// get separator
	static std::string getSearchSeparator();
	std::string getSearchSeparator(const U32 separator_to_get_u32) const;

private:
	enum PVSearchSeparators : U32
	{
		separator_space,
		separator_plus,
		separator_comma,
		separator_pipe,
		separator_semicolon,
		separator_period,
		separator_colon,
	};

	/**
	 * \brief Contains the possible search separators
	 * TODO: Re-write as a map.
	 */
	const std::vector<std::string> PVSearchSeparatorAssociation
	{
		" ",
		"+",
		",",
		"|",
		";",
		".",
		":",
	};

	/**
	* \brief Currently selected search separator
	*/
	static U32 PVSearchSeparatorSelected;
};

extern PVSearchUtil* gPVSearchUtil;
#endif
