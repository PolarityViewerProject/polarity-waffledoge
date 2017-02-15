/**
* @file pvtypes.h
* @brief Vendor-specific types.
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Polarity Viewer Source Code
* Copyright (C) 2016 Xenhat Liamano
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

#ifndef PV_TYPES_H
#define PV_TYPES_H

#pragma once

#include <boost/preprocessor.hpp>
#include "stdtypes.h"
#include "lluuid.h"

typedef std::map<LLUUID, LLColor4> pv_pair_uuid_llcolor4;
typedef std::map<LLUUID, unsigned int> pv_pair_uuid_uint;
typedef std::map<LLUUID, signed int> pv_pair_uuid_sint;
typedef std::map<LLUUID, std::string> pv_pair_uuid_string;
typedef std::map<U32, char> pv_pair_u32_char;
typedef std::map<signed int, std::string> pv_pair_int_string;
typedef std::map<std::string, LLSD> pv_pair_string_llsd;

#endif // PV_TYPES_H
