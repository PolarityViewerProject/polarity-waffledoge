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

#ifndef PV_CONSTANTS_H
#define PV_CONSTANTS_H
#include <string>
// Cannot use STRINGIZE because defined for something else in stringize.h
#define DEF_TO_STRING(x) #x
#define TOSTRING(x) DEF_TO_STRING(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define APP_NAME "Polarity"
#define PROJECT_STRING "polarityviewer"
#define PROJECT_DOMAIN "polarityviewer.org"
#define PROJECT_HOMEPAGE "https://www.polarityviewer.org"
#define PROJECT_UPDATE_URL "https://update.polarityviewer.org/update"
#endif
