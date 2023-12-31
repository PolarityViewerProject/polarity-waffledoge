// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
* @file llpathfindingobjectlist.cpp
* @brief Implementation of llpathfindingobjectlist
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "llpathfindingobjectlist.h"
#include "llpathfindingobject.h"

//---------------------------------------------------------------------------
// LLPathfindingObjectList
//---------------------------------------------------------------------------

LLPathfindingObjectList::LLPathfindingObjectList()
	: mObjectMap()
{
}

LLPathfindingObjectList::~LLPathfindingObjectList()
{
	clear();
}

bool LLPathfindingObjectList::isEmpty() const
{
	return mObjectMap.empty();
}

void LLPathfindingObjectList::clear()
{
	for (LLPathfindingObjectMap::iterator objectIter = mObjectMap.begin(); objectIter != mObjectMap.end(); ++objectIter)
	{
		objectIter->second.reset();
	}
	mObjectMap.clear();
}

void LLPathfindingObjectList::update(LLPathfindingObjectPtr pUpdateObjectPtr)
{
	if (pUpdateObjectPtr != nullptr)
	{
		std::string updateObjectId = pUpdateObjectPtr->getUUID().asString();

		LLPathfindingObjectMap::iterator foundObjectIter = mObjectMap.find(updateObjectId);
		if (foundObjectIter == mObjectMap.end())
		{
			mObjectMap.insert(std::pair<std::string, LLPathfindingObjectPtr>(updateObjectId, pUpdateObjectPtr));
		}
		else
		{
			foundObjectIter->second = pUpdateObjectPtr;
		}
	}
}

void LLPathfindingObjectList::update(LLPathfindingObjectListPtr pUpdateObjectListPtr)
{
	if ((pUpdateObjectListPtr != nullptr) && !pUpdateObjectListPtr->isEmpty())
	{
		for (LLPathfindingObjectMap::const_iterator updateObjectIter = pUpdateObjectListPtr->begin();
			updateObjectIter != pUpdateObjectListPtr->end(); ++updateObjectIter)
		{
			const LLPathfindingObjectPtr updateObjectPtr = updateObjectIter->second;
			update(updateObjectPtr);
		}
	}
}

LLPathfindingObjectPtr LLPathfindingObjectList::find(const std::string &pObjectId) const
{
	LLPathfindingObjectPtr objectPtr;

	LLPathfindingObjectMap::const_iterator objectIter = mObjectMap.find(pObjectId);
	if (objectIter != mObjectMap.end())
	{
		objectPtr = objectIter->second;
	}

	return objectPtr;
}

LLPathfindingObjectList::const_iterator LLPathfindingObjectList::begin() const
{
	return mObjectMap.begin();
}

LLPathfindingObjectList::const_iterator LLPathfindingObjectList::end() const
{
	return mObjectMap.end();
}

LLPathfindingObjectMap &LLPathfindingObjectList::getObjectMap()
{
	return mObjectMap;
}
