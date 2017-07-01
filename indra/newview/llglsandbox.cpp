// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llglsandbox.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llglsandbox.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "llvertexbuffer.h"
#include "llviewershadermgr.h"
#include "llviewercontrol.h"

// Height of the yellow selection highlight posts for land


LLGLSLShader	gBenchmarkProgram;

void draw_line_cube(F32 width, const LLVector3& center)
{
	width = 0.5f * width;
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] + width);

	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] - width);

	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] + width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] + width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width, center.mV[VY] - width, center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width, center.mV[VY] - width, center.mV[VZ] - width);
}

F32 gpu_benchmark(bool force_run)
{

	if (!force_run && gSavedSettings.getBOOL("NoHardwareProbe"))
	{
		LL_INFOS() << "Skipping GPU Benchmark due to user preference" << LL_ENDL;
		return -1.f;
	}
	if (!gGLManager.mHasVertexBufferObject || !gGLManager.mHasShaderObjects || !gGLManager.mHasTimerQuery)
	{
		// don't bother benchmarking the fixed function
		// or without vbos
		// or venerable drivers which don't support accurate timing anyway
		// and are likely to be correctly identified by the GPU table already.
		LL_INFOS() << "Skipping GPU Benchmark due missing features" << LL_ENDL;
		return -1.f;
	}
	// <polarity> Don't run GPU benchmark on Intel graphics, they take too long to run (0.117834GB/sec / 11.225GB/sec )
	if (gGLManager.mIsIntel)
	{
		LL_INFOS() << "Skipping GPU Benchmark on Intel graphics" << LL_ENDL;
		return -1.f;
	}
	LL_INFOS() << "Running GPU benchmark..." << LL_ENDL;

	bool old_fixed_func = LLGLSLShader::sNoFixedFunction;

	static bool local_init = false;
	if (gBenchmarkProgram.mProgramObject == 0)
	{
		local_init = true;
		LLGLSLShader::sNoFixedFunction = true;
		LLViewerShaderMgr::instance()->initAttribsAndUniforms();

		gBenchmarkProgram.mName = "Benchmark Shader";
		gBenchmarkProgram.mFeatures.attachNothing = true;
		gBenchmarkProgram.mShaderFiles.clear();
		gBenchmarkProgram.mShaderFiles.push_back(std::make_pair("interface/benchmarkV.glsl", GL_VERTEX_SHADER));
		gBenchmarkProgram.mShaderFiles.push_back(std::make_pair("interface/benchmarkF.glsl", GL_FRAGMENT_SHADER));
		gBenchmarkProgram.mShaderLevel = 1;
		if (!gBenchmarkProgram.createShader(NULL, NULL))
		{
			LL_WARNS() << "GPU Benchmark failed to compose shaders! Benchmark not run." << LL_ENDL;
			return -1.f;
		}
	}

#ifdef GL_ARB_vertex_array_object
	GLuint vao;
	if (local_init)
	{
		if (LLRender::sGLCoreProfile && !LLVertexBuffer::sUseVAO)
		{
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
		}
	}
#endif

	LLGLDisable blend(GL_BLEND);

	//measure memory bandwidth by:
	// - allocating a batch of textures and render targets
	// - rendering those textures to those render targets
	// - recording time taken
	// - taking the median time for a given number of samples

	//resolution of textures/render targets
	const U32 res = 1024;

	//number of textures
	const U32 count = 32;

	//number of samples to take
	const S32 samples = 64;

	//pre-calculated ([resolution of textures/render targets] ^ 2) * number of textures
	constexpr U32 res2_count = (res * res) * count;

	LLGLSLShader::initProfile();

	LLRenderTarget dest[count];
	U32 source[count];
	LLImageGL::generateTextures(count, source);
	std::vector<F32> results;

	//build a random texture
	U8* pixels = new U8[res*res * 4];

	for (U32 i = 0; i < res*res * 4; ++i)
	{
		pixels[i] = (U8)ll_rand(255);
	}


	gGL.setColorMask(true, true);
	LLGLDepthTest depth(GL_FALSE);

	for (U32 i = 0; i < count; ++i)
	{ //allocate render targets and textures
		dest[i].allocate(res, res, GL_RGBA, false, false, LLTexUnit::TT_TEXTURE, true);
		dest[i].bindTarget();
		dest[i].clear();
		dest[i].flush();

		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, source[i]);
		LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_RGBA, res, res, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}

	delete[] pixels;

	//make a dummy triangle to draw with
	LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, GL_STATIC_DRAW_ARB);
	buff->allocateBuffer(3, 0, true);

	LLStrider<LLVector3> v;
	buff->getVertexStrider(v);

	v[0].set(-1, 1, 0);
	v[1].set(-1, -3, 0);
	v[2].set(3, 1, 0);

	buff->flush();

	gBenchmarkProgram.bind();

	bool busted_finish = false;

	buff->setBuffer(LLVertexBuffer::MAP_VERTEX);
	glFinish();

	for (S32 c = -1; c < samples; ++c)
	{
		LLTimer timer;
		timer.start();

		for (U32 i = 0; i < count; ++i)
		{
			dest[i].bindTarget();
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, source[i]);
			buff->drawArrays(LLRender::TRIANGLES, 0, 3);
			dest[i].flush();
		}

		//wait for current batch of copies to finish
		if (busted_finish)
		{
			//read a pixel off the last target since some drivers seem to ignore glFinish
			dest[count - 1].bindTarget();
			U32 pixel = 0;
			glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
			dest[count - 1].flush();
		}
		else
		{
			glFinish();
		}

		F32 time = timer.getElapsedTimeF32();

		if (c >= 4) // <-- ignore the first 5 samples as they tend to be artificially slow // <polarity/>
		{
			//store result in gigabytes per second
			auto gbps = ((res2_count * 8) * 0.000000001) / time; // <polarity/>

			if (!gGLManager.mHasTimerQuery && !busted_finish && gbps > 2048.f) // <polarity/>
			{ //unrealistically high bandwidth for a card without timer queries, glFinish is probably ignored
				busted_finish = true;
				LL_WARNS() << "GPU Benchmark detected GL driver with broken glFinish implementation." << LL_ENDL;
			}
			else
			{
				results.push_back(gbps);
			}
		}
	}

	gBenchmarkProgram.unbind();

	LLGLSLShader::finishProfile(false);

	LLImageGL::deleteTextures(count, source);

	std::sort(results.begin(), results.end());

	F64 gbps = results[results.size() / 2];

	LL_INFOS() << "Memory bandwidth is " << (gbps * 1.9) << "GB/sec according to CPU timers" << LL_ENDL;

	//#if LL_DARWIN
	if (gbps > 2048.0)
	{
		LL_WARNS() << "Memory bandwidth is improbably high and likely incorrect; discarding result." << LL_ENDL;
		//OSX is probably lying, discard result
		gbps = -1.f;
	}
	//#endif

	LL_INFOS() << "gBenchmarkProgram.mTimeElapsed : " << gBenchmarkProgram.mTimeElapsed << LL_ENDL;

	// <polarity> leaner benchmark result math
	//F32 ms = gBenchmarkProgram.mTimeElapsed / 1000000.f;
	//F32 seconds = ms / 1000.f;
	//F64 samples_drawn = res*res*count*samples;
	//F64 samples_sec = (samples_drawn / 1000000000.0) / seconds;
	//gbps = samples_sec * 8;
	gbps = ((res2_count * samples) / (F64)gBenchmarkProgram.mTimeElapsed) * 8;
	// </polarity>

	if (local_init)
	{
		gBenchmarkProgram.unload();
#ifdef GL_ARB_vertex_array_object
		if (LLRender::sGLCoreProfile && !LLVertexBuffer::sUseVAO)
		{
			glBindVertexArray(0);
			glDeleteVertexArrays(1, &vao);
		}
#endif

		LLGLSLShader::sNoFixedFunction = old_fixed_func;
		local_init = false;
	}

	LL_INFOS() << "Memory bandwidth is " << llformat("%.3f", gbps) << "GB/sec according to ARB_timer_query" << LL_ENDL;

	// Turn off subsequent benchmarking.
	gSavedSettings.setBOOL("NoHardwareProbe", TRUE);
	return gbps;
}

