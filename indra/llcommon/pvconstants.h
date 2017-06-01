/**
 * @file pvconstants.h
 * @brief Vendor-specific constants.
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

#pragma once

#ifndef PV_CONSTANTS_H
#define PV_CONSTANTS_H
#if (LL_WINDOWS)
#include "llwin32headers.h"
#endif
//#include "stdtypes.h" // for std::string
#include <string>

// <polarity> This contains the name of the viewer.
const std::string APP_NAME = "Polarity";
const std::string CAPITALIZED_APP_NAME = "POLARITY";

#if (LL_WINDOWS)
const std::wstring LAPP_NAME = L"Polarity";
const LPCWSTR LAPP_NAME_LPCWSTR = LAPP_NAME.c_str();
#endif

const std::string PROJECT_STRING = "polarityviewer";
const std::string PROJECT_DOMAIN = PROJECT_STRING + ".org";
const std::string PROJECT_HOMEPAGE = "https://www." + PROJECT_DOMAIN;
const std::string PROJECT_UPDATE_URL = "https://update."+PROJECT_DOMAIN+"/update";

#endif
