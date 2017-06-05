/**
* @file llglsandbox.h
* @brief GL functionality access
*
* $LicenseInfo:firstyear=2003&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

/**
* Contains ALL methods which directly access GL functionality
* except for core rendering engine functionality.
*/

#pragma once
#ifndef LL_GL_SANDBOX_H
#define LL_GL_SANDBOX_H

#include "stdtypes.h"
#include "v3math.h"
#include "llglslshader.h"

extern LLGLSLShader	gBenchmarkProgram;

void draw_line_cube(F32 width, const LLVector3& center);
F32 gpu_benchmark(bool force_run = false);

#endif // LL_GL_SANDBOX_H
