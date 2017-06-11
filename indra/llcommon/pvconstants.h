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

// <polarity> This contains the name of the viewer project.
#ifndef ROOT_PROJECT_NAME
 #error "ROOT_PROJECT_NAME is not defined! If you see this error message, re-run CMake or clean your build directory and try again."
#endif
//const std::string APP_NAME = APP_NAME;
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
// Compile-time checks don't appear to work on strings, we're going to have to pray this never ends up empty
const std::string APP_NAME = TOSTRING(ROOT_PROJECT_NAME);

const std::string PROJECT_STRING = "polarityviewer";
const std::string PROJECT_DOMAIN = PROJECT_STRING + ".org";
const std::string PROJECT_HOMEPAGE = "https://www." + PROJECT_DOMAIN;
const std::string PROJECT_UPDATE_URL = "https://update."+PROJECT_DOMAIN+"/update";
#endif
