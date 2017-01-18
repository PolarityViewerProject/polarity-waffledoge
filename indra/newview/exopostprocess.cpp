/**
 * @file exopostprocess.h
 * @brief exoPostProcess class definition
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (C) 2011 Geenz Spad
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
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "exopostprocess.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llsky.h"

LLVector3 exoPostProcess::sExodusRenderGamma;
LLVector3 exoPostProcess::sExodusRenderExposure;
LLVector3 exoPostProcess::sExodusRenderOffset;
GLuint exoPostProcess::sExodusRenderColorFormat;
F32 exoPostProcess::sExodusRenderToneExposure;
BOOL exoPostProcess::sExodusRenderToneMapping;
LLVector3 exoPostProcess::sExodusRenderVignette;
S32 exoPostProcess::sExodusRenderToneMappingTech;
S32 exoPostProcess::sExodusRenderColorGradeTech;
LLVector3 exoPostProcess::sExodusRenderToneAdvOptA;
LLVector3 exoPostProcess::sExodusRenderToneAdvOptB;
LLVector3 exoPostProcess::sExodusRenderToneAdvOptC;
F32 exoPostProcess::sExodusRenderGammaCurve;
F32 exoPostProcess::sGreyscaleStrength;
F32 exoPostProcess::sSepiaStrength;
U32 exoPostProcess::sNumColors;
BOOL exoPostProcess::sRenderLensFlare;

exoPostProcess::exoPostProcess()
{

	mExoPostBuffer = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1, 0);
	mExoPostBuffer->allocateBuffer(8, 0, true);

	LLStrider<LLVector3> vert;
	mExoPostBuffer->getVertexStrider(vert);
	LLStrider<LLVector2> tc0;
	LLStrider<LLVector2> tc1;
	mExoPostBuffer->getTexCoord0Strider(tc0);
	mExoPostBuffer->getTexCoord1Strider(tc1);

	vert[0].set(-1,1,0);
	vert[1].set(-1,-3,0);
	vert[2].set(3,1,0);

	sExodusRenderGamma = LLVector3(1.f, 1.f, 1.f);
	sExodusRenderExposure = LLVector3(1.f, 1.f, 1.f);
	sExodusRenderOffset = LLVector3(1.f, 1.f, 1.f);
	sExodusRenderColorFormat = GL_RGBA8;
	sExodusRenderToneExposure = 1.f;
	sExodusRenderToneMapping = FALSE;
	sExodusRenderVignette = LLVector3(0.f, 0.f, 0.f);
	sExodusRenderToneMappingTech = 0;
	sExodusRenderColorGradeTech = 0;
	sExodusRenderToneAdvOptA = LLVector3(1.f,1.f,1.f);
	sExodusRenderToneAdvOptB = LLVector3(1.f, 1.f, 1.f);
	sExodusRenderToneAdvOptC = LLVector3(1.f,1.f,1.f);
	sExodusRenderGammaCurve = 2.2f;
	sGreyscaleStrength = 0.0f;
	sSepiaStrength = 0.0f;
	sNumColors = 0;
}

exoPostProcess::~exoPostProcess()
{
	mExoPostBuffer = NULL;
}

void exoPostProcess::ExodusRenderPostStack(LLRenderTarget *src, LLRenderTarget *dst)
{
	if (mVertexShaderLevel > 0)
	{
		if (sExodusRenderToneMapping)
		{
			if (sExodusRenderToneMappingTech == 1)
				ExodusRenderToneMapping(src, dst, exoPostProcess::EXODUS_RENDER_TONE_LINEAR);
			else if (sExodusRenderToneMappingTech == 2)
				ExodusRenderToneMapping(src, dst, exoPostProcess::EXODUS_RENDER_TONE_REINHARD);
			else if (sExodusRenderToneMappingTech == 0 && LLPipeline::sRenderDeferred)
				ExodusRenderToneMapping(src, dst, exoPostProcess::EXODUS_RENDER_TONE_FILMIC);
			else if (sExodusRenderToneMappingTech == 3 && LLPipeline::sRenderDeferred)
				ExodusRenderToneMapping(src, dst, exoPostProcess::EXODUS_RENDER_TONE_FILMIC_ADV);
		}

		if (sExodusRenderColorGradeTech > -1 && LLPipeline::sRenderDeferred)
		{
			if (sExodusRenderColorGradeTech == 0)
				ExodusRenderColorGrade(src, dst, exoPostProcess::EXODUS_RENDER_COLOR_GRADE_LEGACY);
			else if (sExodusRenderColorGradeTech == 1)
				ExodusRenderColorGrade(src, dst, exoPostProcess::EXODUS_RENDER_COLOR_GRADE);
		} else {
			if (sExodusRenderColorGradeTech > -1)
				ExodusRenderColorGrade(src, dst, exoPostProcess::EXODUS_RENDER_COLOR_GRADE_LEGACY); // Temporary work around: only render legacy color correction in non-deferred.
		}

		if (sExodusRenderVignette.mV[0] > 0 && gPipeline.sRenderDeferred)
			ExodusRenderVignette(src, dst); // Don't render vignette here in non-deferred. Do it in the glow combine shader.

		if(gPipeline.sRenderDeferred)
		{
			if (sRenderLensFlare)
				ExodusRenderLensFlare(src, dst);

			if (sGreyscaleStrength > 0.0f || sNumColors > 2
				|| sSepiaStrength > 0.0f)
				ExodusRenderSpecial(src, dst);
		}
	}
}
void exoPostProcess::ExodusRenderPostSettingsUpdate()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR);
	sExodusRenderGamma = gSavedSettings.getVector3("PVRender_Gamma");
	sExodusRenderExposure = gSavedSettings.getVector3("PVRender_Exposure");
	sExodusRenderOffset = gSavedSettings.getVector3("PVRender_HDRBrightnessOffset");
	sExodusRenderColorFormat = GL_RGBA16F_ARB;
	sExodusRenderToneExposure = gSavedSettings.getF32("PVRender_ToneMappingExposure");
	sExodusRenderToneMapping = gSavedSettings.getBOOL("PVRender_EnableToneMapping");
	sExodusRenderVignette = gSavedSettings.getVector3("PVRender_Vignette");
	sExodusRenderToneMappingTech = gSavedSettings.getS32("PVRender_ToneMappingTech");
	sGreyscaleStrength = gSavedSettings.getF32("PVRender_PostGreyscaleStrength");
	sSepiaStrength = gSavedSettings.getF32("PVRender_PostSepiaStrength");
	sNumColors = gSavedSettings.getU32("PVRender_PostPosterizationSamples");
	sRenderLensFlare = gSavedSettings.getBOOL("PVRender_EnableLensFlare");
	if (mVertexShaderLevel > 0) // Don't even bother with fetching the color grading texture if our vertex shader level isn't above 0.
	{
		LLViewerFetchedTexture::sExodusColorGradeTexp = LLViewerTextureManager::getFetchedTexture(LLUUID(gSavedSettings.getString("PVRender_ColorGradeTexture")), FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
		LLViewerFetchedTexture::sExodusColorGradeTexp->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	sExodusRenderColorGradeTech = gSavedSettings.getS32("PVRender_ColorGradeTech");
	sExodusRenderToneAdvOptA = gSavedSettings.getVector3("PVRender_ToneMappingControlA");
	sExodusRenderToneAdvOptB = gSavedSettings.getVector3("PVRender_ToneMappingControlB");
	sExodusRenderToneAdvOptC = gSavedSettings.getVector3("PVRender_ToneMappingControlC");

	if (sExodusRenderToneMapping)
	{
		if (!gPipeline.RenderDeferred)
		{
			sExodusRenderToneMapping = FALSE;
		}
	}
}
void exoPostProcess::ExodusRenderPostUpdate()
{
	etc1.setVec(0,0);
	etc2.setVec((F32) gPipeline.mScreen.getWidth(),
		(F32) gPipeline.mScreen.getHeight());
	if (!gPipeline.sRenderDeferred)
	{
// Destroy our old buffer, and create a new vertex buffer for the screen (shamelessly ganked from pipeline.cpp).
		mExoPostBuffer = NULL;
		mExoPostBuffer = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1, 0);
		mExoPostBuffer->allocateBuffer(3,0,TRUE);

		LLStrider<LLVector3> v;
		LLStrider<LLVector2> uv1;
		LLStrider<LLVector2> uv2;

		mExoPostBuffer->getVertexStrider(v);
		mExoPostBuffer->getTexCoord0Strider(uv1);
		mExoPostBuffer->getTexCoord1Strider(uv2);

		uv1[0] = LLVector2(0, 0);
		uv1[1] = LLVector2(0, 2);
		uv1[2] = LLVector2(2, 0);

		uv2[0] = LLVector2(0, 0);
		uv2[1] = LLVector2(0, etc2.mV[1]*2.f);
		uv2[2] = LLVector2(etc2.mV[0]*2.f, 0);

		v[0] = LLVector3(-1,-1,0);
		v[1] = LLVector3(-1,3,0);
		v[2] = LLVector3(3,-1,0);

		mExoPostBuffer->flush();
	} else {
		mExoPostBuffer = NULL;
		mExoPostBuffer = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1, 0);
		mExoPostBuffer->allocateBuffer(8, 0, true);

		LLStrider<LLVector3> vert;
		mExoPostBuffer->getVertexStrider(vert);
		LLStrider<LLVector2> tc0;
		LLStrider<LLVector2> tc1;
		mExoPostBuffer->getTexCoord0Strider(tc0);
		mExoPostBuffer->getTexCoord1Strider(tc1);

		vert[0].set(-1,1,0);
		vert[1].set(-1,-3,0);
		vert[2].set(3,1,0);
	}
}

void exoPostProcess::ExodusRenderPost(LLRenderTarget* src, LLRenderTarget* dst, S32 type)
{
	if (type == EXODUS_RENDER_TONE_LINEAR || type == EXODUS_RENDER_TONE_REINHARD || type == EXODUS_RENDER_TONE_FILMIC || type == EXODUS_RENDER_TONE_FILMIC_ADV)
		ExodusRenderToneMapping(src, dst, type);
	else if (type == EXODUS_RENDER_COLOR_GRADE || type == EXODUS_RENDER_COLOR_GRADE_LEGACY)
		ExodusRenderColorGrade(src, dst, type);
	else if (type == EXODUS_RENDER_VIGNETTE_POST)
		ExodusRenderVignette(src, dst);
}

void exoPostProcess::ExodusGenerateLUT()
{
}

void exoPostProcess::ExodusRenderToneMapping(LLRenderTarget *src, LLRenderTarget *dst, S32 type)
{
	src->bindTarget();
	LLGLSLShader *shader;

	if (type == EXODUS_RENDER_TONE_LINEAR)
		shader = &gLinearToneMapping;
	else if (type == EXODUS_RENDER_TONE_REINHARD)
		shader = &gReinhardToneMapping;
	else if (type == EXODUS_RENDER_TONE_FILMIC)
		shader = &gFilmicToneMapping;
	else if (type == EXODUS_RENDER_TONE_FILMIC_ADV)
		shader = &gFilmicToneMappingAdv;
	else
	{
		return; // Not a valid tone mapping mode. Exit early.
	}

	shader->bind();
	mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);
	exoShader::BindRenderTarget(dst, shader, LLShaderMgr::EXO_RENDER_SCREEN, 0);
	shader->uniform1f(LLShaderMgr::EXO_RENDER_EXPOSURE, sExodusRenderToneExposure);
	if (type == EXODUS_RENDER_TONE_FILMIC_ADV)
	{
		shader->uniform3fv(LLShaderMgr::EXO_RENDER_ADV_TONE_UA, 1, sExodusRenderToneAdvOptA.mV);
		shader->uniform3fv(LLShaderMgr::EXO_RENDER_ADV_TONE_UB, 1, sExodusRenderToneAdvOptB.mV);
		shader->uniform3fv(LLShaderMgr::EXO_RENDER_ADV_TONE_UC, 1, sExodusRenderToneAdvOptC.mV);
	}
	mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
	stop_glerror();

	shader->unbind();
	src->flush();
}

void exoPostProcess::ExodusRenderColorGrade(LLRenderTarget *src, LLRenderTarget *dst, S32 type)
{
	if (type == EXODUS_RENDER_COLOR_GRADE) {
		if (LLViewerFetchedTexture::sExodusColorGradeTexp->isFullyLoaded()) //Prevents the "gray screen" while the color grade texture loads.
		{
			src->bindTarget();
			gColorGradePost.bind();
			mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);
			exoShader::BindRenderTarget(dst, &gColorGradePost, LLShaderMgr::EXO_RENDER_SCREEN, 0);
			exoShader::BindTex2D(LLViewerFetchedTexture::sExodusColorGradeTexp, &gColorGradePost, LLShaderMgr::EXO_RENDER_GRADE, 1);

			mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
			stop_glerror();

			gColorGradePost.unbind();
			src->flush();
		}

	} else if (type == EXODUS_RENDER_COLOR_GRADE_LEGACY)
	{
		src->bindTarget();
		gColorGradePostLegacy.bind();
		mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);
		exoShader::BindRenderTarget(dst, &gColorGradePostLegacy, LLShaderMgr::EXO_RENDER_SCREEN, 0);

		gColorGradePostLegacy.uniform3fv(LLShaderMgr::EXO_RENDER_GAMMA, 1, sExodusRenderGamma.mV);
		gColorGradePostLegacy.uniform3fv(LLShaderMgr::EXO_RENDER_EXPOSURE, 1, sExodusRenderExposure.mV);
		gColorGradePostLegacy.uniform3fv(LLShaderMgr::EXO_RENDER_OFFSET, 1, sExodusRenderOffset.mV);
		mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
		stop_glerror();

		gColorGradePostLegacy.unbind();
		src->flush();
	}
}

void exoPostProcess::ExodusRenderVignette(LLRenderTarget* src, LLRenderTarget* dst)
{
	dst->bindTarget();
	LLGLSLShader *shader = &gVignettePost;
	shader->bind();

	mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);

	exoShader::BindRenderTarget(dst, shader, LLShaderMgr::EXO_RENDER_SCREEN);

	shader->uniform3fv(LLShaderMgr::EXO_RENDER_VIGNETTE, 1, sExodusRenderVignette.mV);
	mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
	stop_glerror();

	shader->unbind();
	dst->flush();
}

void exoPostProcess::ExodusRenderSpecial(LLRenderTarget* src, LLRenderTarget* dst)
{
	dst->bindTarget();
	LLGLSLShader *shader = &gSpecialPost;
	shader->bind();

	mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);

	exoShader::BindRenderTarget(dst, shader, LLShaderMgr::EXO_RENDER_SCREEN);

	shader->uniform1i(LLShaderMgr::DEFERRED_NUM_COLORS, sNumColors);
	shader->uniform1f(LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH, sGreyscaleStrength);
	shader->uniform1f(LLShaderMgr::DEFERRED_SEPIA_STRENGTH, sSepiaStrength);
	mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
	stop_glerror();
	shader->unbind();
	dst->flush();
}

void exoPostProcess::ExodusRenderLensFlare(LLRenderTarget* src, LLRenderTarget* dst)
{
	dst->bindTarget();
	LLGLSLShader *shader = &gLensFlare;
	shader->bind();

	mExoPostBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);

	exoShader::BindRenderTarget(dst, shader, LLShaderMgr::EXO_RENDER_SCREEN);

	shader->uniform3fv(LLShaderMgr::DEFERRED_SUN_DIR, 1, gPipeline.mTransformedSunDir.mV);
	shader->uniform4fv(LLShaderMgr::SUNLIGHT_COLOR, 1, gSky.getSunDiffuseColor().mV);
	mExoPostBuffer->drawArrays(LLRender::TRIANGLES, 0, 3);
	stop_glerror();

	shader->unbind();
	dst->flush();
}

void exoShader::BindTex2D(LLTexture *tex2D, LLGLSLShader *shader, S32 uniform, S32 unit, LLTexUnit::eTextureType mode, LLTexUnit::eTextureAddressMode addressMode, LLTexUnit::eTextureFilterOptions filterMode)
{
	if(gPipeline.sRenderDeferred)
	{
		S32 channel = 0;
		channel = shader->enableTexture(uniform);
		if (channel > -1)
		{
			gGL.getTexUnit(channel)->bind(tex2D);
			gGL.getTexUnit(channel)->setTextureFilteringOption(filterMode);
			gGL.getTexUnit(channel)->setTextureAddressMode(addressMode);
		}
	} else {
		gGL.getTexUnit(unit)->bind(tex2D);
	}
}

void exoShader::BindRenderTarget(LLRenderTarget* tgt, LLGLSLShader* shader, S32 uniform, S32 unit, LLTexUnit::eTextureType mode)
{
	if(gPipeline.sRenderDeferred)
	{
		S32 channel = 0;
		channel = shader->enableTexture(uniform, tgt->getUsage());
		if (channel > -1)
		{
			tgt->bindTexture(0,channel);
			gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		}
	} else {
		S32 reftex = shader->enableTexture(uniform, tgt->getUsage());
		if (reftex > -1)
		{
			gGL.getTexUnit(reftex)->activate();
			gGL.getTexUnit(reftex)->bind(tgt);
			gGL.getTexUnit(0)->activate();
		}
	}
	shader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, tgt->getWidth(), tgt->getHeight());
}

