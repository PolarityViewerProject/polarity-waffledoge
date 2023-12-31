/** 
 * @file llvolumeoctree.h
 * @brief LLVolume octree classes.
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

#ifndef LL_LLVOLUME_OCTREE_H
#define LL_LLVOLUME_OCTREE_H

#include "linden_common.h"
#include "llmemory.h"

#include "lloctree.h"
#include "llvolume.h"
#include "llvector4a.h"

class LLVolumeTriangle : public LLRefCount
{
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	LLVolumeTriangle(): mRadius(0)
	{
		mBinIndex = -1;
	}

	LLVolumeTriangle(const LLVolumeTriangle& rhs)
	{
		*this = rhs;
	}

	const LLVolumeTriangle& operator=(const LLVolumeTriangle& rhs)
	{
		LL_ERRS() << "Illegal operation!" << LL_ENDL;
		return *this;
	}

	~LLVolumeTriangle()
	{
	
	}

	LL_ALIGN_16(LLVector4a mPositionGroup);

	const LLVector4a* mV[3];
	U16 mIndex[3];

	F32 mRadius;
	mutable S32 mBinIndex;


	virtual const LLVector4a& getPositionGroup() const;
	virtual const F32& getBinRadius() const;
	
	S32 getBinIndex() const { return mBinIndex; }
	void setBinIndex(S32 idx) const { mBinIndex = idx; }


};

class LLVolumeOctreeListener : public LLOctreeListener<LLVolumeTriangle>
{
public:
	
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	LLVolumeOctreeListener(LLOctreeNode<LLVolumeTriangle>* node);
	~LLVolumeOctreeListener();
	
	LLVolumeOctreeListener(const LLVolumeOctreeListener& rhs)
	{
		*this = rhs;
	}

	const LLVolumeOctreeListener& operator=(const LLVolumeOctreeListener& rhs)
	{
		LL_ERRS() << "Illegal operation!" << LL_ENDL;
		return *this;
	}

	 //LISTENER FUNCTIONS
	void handleChildAddition(const LLOctreeNode<LLVolumeTriangle>* parent, 
		LLOctreeNode<LLVolumeTriangle>* child) override;
	void handleStateChange(const LLTreeNode<LLVolumeTriangle>* node) override { }

	void handleChildRemoval(const LLOctreeNode<LLVolumeTriangle>* parent, 
			const LLOctreeNode<LLVolumeTriangle>* child) override {	}

	void handleInsertion(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) override { }
	void handleRemoval(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) override { }
	void handleDestruction(const LLTreeNode<LLVolumeTriangle>* node) override { }
	

public:
	LL_ALIGN_16(LLVector4a mBounds[2]); // bounding box (center, size) of this node and all its children (tight fit to objects)
	LL_ALIGN_16(LLVector4a mExtents[2]); // extents (min, max) of this node and all its children
};

class LLOctreeTriangleRayIntersect : public LLOctreeTraveler<LLVolumeTriangle>
{
public:
	const LLVolumeFace* mFace;
	LLVector4a mStart;
	LLVector4a mDir;
	LLVector4a mEnd;
	LLVector4a* mIntersection;
	LLVector2* mTexCoord;
	LLVector4a* mNormal;
	LLVector4a* mTangent;
	F32* mClosestT;
	bool mHitFace;

	LLOctreeTriangleRayIntersect(const LLVector4a& start, const LLVector4a& dir, 
								   const LLVolumeFace* face, F32* closest_t,
								   LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent);

	void traverse(const LLOctreeNode<LLVolumeTriangle>* node) override;

	void visit(const LLOctreeNode<LLVolumeTriangle>* node) override;
    virtual ~LLOctreeTriangleRayIntersect() {}
};

class LLVolumeOctreeValidate : public LLOctreeTraveler<LLVolumeTriangle>
{
	void visit(const LLOctreeNode<LLVolumeTriangle>* branch) override;
    
public:
    virtual ~LLVolumeOctreeValidate() {}
};

#endif
