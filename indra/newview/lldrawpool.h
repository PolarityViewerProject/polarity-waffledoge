/** 
 * @file lldrawpool.h
 * @brief LLDrawPool class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOL_H
#define LL_LLDRAWPOOL_H

#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "llvertexbuffer.h"

class LLFace;
class LLViewerTexture;
class LLViewerFetchedTexture;
class LLSpatialGroup;
class LLDrawInfo;

class LLDrawPool
{
public:
	static S32 sNumDrawPools;

	enum
	{
		// Correspond to LLPipeline render type
		POOL_SIMPLE = 1,
		POOL_GROUND,
		POOL_FULLBRIGHT,
		POOL_BUMP,
		POOL_MATERIALS,
		POOL_TERRAIN,	
		POOL_TREE,
		POOL_SKY,
		POOL_WL_SKY,
		POOL_ALPHA_MASK,
		POOL_FULLBRIGHT_ALPHA_MASK,
		POOL_GRASS,
		POOL_AVATAR,
		POOL_VOIDWATER,
		POOL_WATER,
		POOL_GLOW,
		POOL_ALPHA,
		NUM_POOL_TYPES,
	};
	
	LLDrawPool(const U32 type);
	virtual ~LLDrawPool();

	virtual BOOL isDead() = 0;

	S32 getId() const { return mId; }
	U32 getType() const { return mType; }

	BOOL getSkipRenderFlag() const { return mSkipRender;}
	void setSkipRenderFlag( BOOL flag ) { mSkipRender = flag; }

	virtual LLViewerTexture *getDebugTexture();
	virtual void beginRenderPass( S32 pass );
	virtual void endRenderPass( S32 pass );
	virtual S32	 getNumPasses();
	
	virtual void beginDeferredPass(S32 pass);
	virtual void endDeferredPass(S32 pass);
	virtual S32 getNumDeferredPasses();
	virtual void renderDeferred(S32 pass = 0);

	virtual void beginPostDeferredPass(S32 pass);
	virtual void endPostDeferredPass(S32 pass);
	virtual S32 getNumPostDeferredPasses();
	virtual void renderPostDeferred(S32 pass = 0);

	virtual void beginShadowPass(S32 pass);
	virtual void endShadowPass(S32 pass);
	virtual S32 getNumShadowPasses();
	virtual void renderShadow(S32 pass = 0);

	virtual void render(S32 pass = 0) = 0;
	virtual void prerender() = 0;
	virtual U32 getVertexDataMask() = 0;
	virtual BOOL verify() const { return TRUE; }		// Verify that all data in the draw pool is correct!
	virtual S32 getVertexShaderLevel() const { return mVertexShaderLevel; }
	
	static LLDrawPool* createPool(const U32 type, LLViewerTexture *tex0 = nullptr);
	virtual LLDrawPool *instancePool() = 0;	// Create an empty new instance of the pool.
	virtual LLViewerTexture* getTexture() = 0;
	virtual BOOL isFacePool() { return FALSE; }
	virtual void resetDrawOrders() = 0;

protected:
	S32 mVertexShaderLevel;
	S32	mId;
	U32 mType;				// Type of draw pool
	BOOL mSkipRender;
};

class LLRenderPass : public LLDrawPool
{
public:
	enum
	{
		PASS_SIMPLE = NUM_POOL_TYPES,
		PASS_GRASS,
		PASS_FULLBRIGHT,
		PASS_FULLBRIGHT_SHINY,
		PASS_SHINY,
		PASS_BUMP,
		PASS_POST_BUMP,
		PASS_MATERIAL,
		PASS_MATERIAL_ALPHA,
		PASS_MATERIAL_ALPHA_MASK,
		PASS_MATERIAL_ALPHA_EMISSIVE,
		PASS_SPECMAP,
		PASS_SPECMAP_BLEND,
		PASS_SPECMAP_MASK,
		PASS_SPECMAP_EMISSIVE,
		PASS_NORMMAP,
		PASS_NORMMAP_BLEND,
		PASS_NORMMAP_MASK,
		PASS_NORMMAP_EMISSIVE,
		PASS_NORMSPEC,
		PASS_NORMSPEC_BLEND,
		PASS_NORMSPEC_MASK,
		PASS_NORMSPEC_EMISSIVE,
		PASS_GLOW,
		PASS_ALPHA,
		PASS_ALPHA_MASK,
		PASS_FULLBRIGHT_ALPHA_MASK,
		PASS_ALPHA_INVISIBLE,
		NUM_RENDER_TYPES,
	};

	LLRenderPass(const U32 type);
	virtual ~LLRenderPass();
	/*virtual*/ LLDrawPool* instancePool() override;
	/*virtual*/ LLViewerTexture* getDebugTexture() override { return nullptr; }
	LLViewerTexture* getTexture() override { return nullptr; }
	BOOL isDead() override { return FALSE; }
	void resetDrawOrders() override { }

	static void applyModelMatrix(const LLDrawInfo& params);
	virtual void pushBatches(U32 type, U32 mask, BOOL texture = TRUE, BOOL batch_textures = FALSE);
	virtual void pushMaskBatches(U32 type, U32 mask, BOOL texture = TRUE, BOOL batch_textures = FALSE);
	virtual void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures = FALSE);
	virtual void renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE);
	virtual void renderGroups(U32 type, U32 mask, BOOL texture = TRUE);
	virtual void renderTexture(U32 type, U32 mask);

};

class LLFacePool : public LLDrawPool
{
public:
	typedef std::vector<LLFace*> face_array_t;
	
	enum
	{
		SHADER_LEVEL_SCATTERING = 2
	};

public:
	LLFacePool(const U32 type);
	virtual ~LLFacePool();
	
	BOOL isDead() override { return mReferences.empty(); }

	LLViewerTexture *getTexture() override;
	virtual void dirtyTextures(const std::set<LLViewerFetchedTexture*>& textures);

	virtual void enqueue(LLFace *face);
	virtual BOOL addFace(LLFace *face);
	virtual BOOL removeFace(LLFace *face);

	BOOL verify() const override;		// Verify that all data in the draw pool is correct!

	void resetDrawOrders() override;
	void resetAll();

	void destroy();

	void buildEdges();

	void addFaceReference(LLFace *facep);
	void removeFaceReference(LLFace *facep);

	void printDebugInfo() const;
	
	BOOL isFacePool() override { return TRUE; }

	friend class LLFace;
	friend class LLPipeline;
public:
	face_array_t	mDrawFace;
	face_array_t	mMoveFace;
	face_array_t	mReferences;

public:
	class LLOverrideFaceColor
	{
	public:
		LLOverrideFaceColor(LLDrawPool* pool)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
		}
		LLOverrideFaceColor(LLDrawPool* pool, const LLColor4& color)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(color);
		}
		LLOverrideFaceColor(LLDrawPool* pool, const LLColor4U& color)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(color);
		}
		LLOverrideFaceColor(LLDrawPool* pool, F32 r, F32 g, F32 b, F32 a)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(r, g, b, a);
		}
		~LLOverrideFaceColor()
		{
			sOverrideFaceColor = mOverride;
		}
		void setColor(const LLColor4& color);
		void setColor(const LLColor4U& color);
		void setColor(F32 r, F32 g, F32 b, F32 a);
		BOOL mOverride;
		LLDrawPool* mPool;
		static BOOL sOverrideFaceColor;
	};
};


#endif //LL_LLDRAWPOOL_H
