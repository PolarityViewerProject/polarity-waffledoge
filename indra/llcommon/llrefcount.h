/** 
 * @file llrefcount.h
 * @brief Base class for reference counted objects for use with LLPointer
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
#ifndef LLREFCOUNT_H
#define LLREFCOUNT_H

#include <boost/intrusive_ptr.hpp>
#include "llatomic.h"
#include "llmutex.h"

#define LL_REF_COUNT_DEBUG 0
#if LL_REF_COUNT_DEBUG
class LLMutex ;
#endif

//----------------------------------------------------------------------------
// RefCount objects should generally only be accessed by way of LLPointer<>'s
// see llthread.h for LLThreadSafeRefCount
//----------------------------------------------------------------------------

class LL_COMMON_API LLRefCount
{
protected:
	LLRefCount(const LLRefCount& other);
	LLRefCount& operator=(const LLRefCount&);
	virtual ~LLRefCount(); // use unref()
	
public:
	LLRefCount();

#if LL_REF_COUNT_DEBUG
	void ref() const ;
	S32 unref() const ;
#else
	inline void ref() const
	{ 
		mRef++; 
	} 

	inline S32 unref() const
	{
		llassert(mRef >= 1);
		if (0 == --mRef)
		{
			delete this; 
			return 0;
		}
		return mRef;
	}	
#endif

	//NOTE: when passing around a const LLRefCount object, this can return different results
	// at different types, since mRef is mutable
	S32 getNumRefs() const
	{
		return mRef;
	}

private: 
	mutable S32	mRef; 

#if LL_REF_COUNT_DEBUG
	mutable LLMutex  mMutex ;
	mutable U32  mLockedThreadID ;
	mutable BOOL mCrashAtUnlock ; 
#endif
};


//============================================================================

// see llmemory.h for LLPointer<> definition

class LL_COMMON_API LLThreadSafeRefCount
{
public:
	static void initThreadSafeRefCount(); // creates sMutex
	static void cleanupThreadSafeRefCount(); // destroys sMutex

private:
	static LLMutex* sMutex;

protected:
	virtual ~LLThreadSafeRefCount(); // use unref()

public:
	LLThreadSafeRefCount();
	LLThreadSafeRefCount(const LLThreadSafeRefCount&);
	LLThreadSafeRefCount& operator=(const LLThreadSafeRefCount& ref) 
	{
		mRef = 0;
		return *this;
	}

	void ref()
	{
		++mRef; 
	} 

	void unref()
	{
		llassert(mRef >= 1);
		if ((--mRef) == 0)		// See note at http://en.cppreference.com/w/cpp/atomic/atomic/operator_arith on atomic decrement operator return value.  
		{	
			// If we hit zero, the caller should be the only smart pointer owning the object and we can delete it.
			// It is technically possible for a vanilla pointer to mess this up, or another thread to
			// jump in, find this object, create another smart pointer and end up dangling, but if
			// the code is that bad and not thread-safe, it's trouble already.
			delete this;
		}
	}

	S32 getNumRefs() const
	{
		const S32 currentVal = mRef.load();
		return currentVal;
	}

private: 
	LLAtomic32< S32	> mRef; 
};

/**
 * intrusive pointer support for LLThreadSafeRefCount
 * this allows you to use boost::intrusive_ptr with any LLThreadSafeRefCount-derived type
 */
inline void intrusive_ptr_add_ref(LLThreadSafeRefCount* p) 
{
	p->ref();
}

inline void intrusive_ptr_release(LLThreadSafeRefCount* p) 
{
	p->unref(); 
}

/**
 * intrusive pointer support
 * this allows you to use boost::intrusive_ptr with any LLRefCount-derived type
 */
inline void intrusive_ptr_add_ref(LLRefCount* p) 
{
	p->ref();
}

inline void intrusive_ptr_release(LLRefCount* p) 
{
	p->unref(); 
}

#endif
