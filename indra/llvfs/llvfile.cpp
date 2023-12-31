// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llvfile.cpp
 * @brief Implementation of virtual file
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

#include "linden_common.h"

#include "llvfile.h"

#include "llerror.h"
#include "llthread.h"
#include "lltimer.h"
#include "llfasttimer.h"
#include "llmemory.h"
#include "llvfs.h"

static LLTrace::BlockTimerStatHandle FTM_VFILE_WAIT("VFile Wait");

//----------------------------------------------------------------------------
LLVFSThread* LLVFile::sVFSThread = nullptr;
BOOL LLVFile::sAllocdVFSThread = FALSE;
//----------------------------------------------------------------------------

//============================================================================

LLVFile::LLVFile(LLVFS *vfs, const LLUUID &file_id, const LLAssetType::EType file_type, S32 mode)
{
	mFileType =	file_type;

	mFileID =	file_id;
	mPosition = 0;
	mMode =		mode;
	mVFS =		vfs;

	mBytesRead = 0;
	mHandle = LLVFSThread::nullHandle();
	mPriority = 128.f;

	mVFS->incLock(mFileID, mFileType, VFSLOCK_OPEN);
}

LLVFile::~LLVFile()
{
	if (!isReadComplete())
	{
		if (mHandle != LLVFSThread::nullHandle())
		{
			if (!(mMode & LLVFile::WRITE))
			{
				//LL_WARNS() << "Destroying LLVFile with pending async read/write, aborting..." << LL_ENDL;
				sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_ABORT);
			}
			else // WRITE
			{
				sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE);
			}
		}
	}
	mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
}

BOOL LLVFile::read(U8 *buffer, S32 bytes, BOOL async, F32 priority)
{
	if (! (mMode & READ))
	{
		LL_WARNS() << "Attempt to read from file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
		return FALSE;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		LL_WARNS() << "Attempt to read from vfile object " << mFileID << " with pending async operation" << LL_ENDL;
		return FALSE;
	}
	mPriority = priority;
	
	BOOL success = TRUE;

	// We can't do a read while there are pending async writes
	waitForLock(VFSLOCK_APPEND);
	
	// *FIX: (?)
	if (async)
	{
		mHandle = sVFSThread->read(mVFS, mFileID, mFileType, buffer, mPosition, bytes, threadPri());
	}
	else
	{
		// We can't do a read while there are pending async writes on this file
		mBytesRead = sVFSThread->readImmediate(mVFS, mFileID, mFileType, buffer, mPosition, bytes);
		mPosition += mBytesRead;
		if (! mBytesRead)
		{
			success = FALSE;
		}
	}

	return success;
}

//static
U8* LLVFile::readFile(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, S32* bytes_read)
{
	U8 *data;
	LLVFile file(vfs, uuid, type, LLVFile::READ);
	S32 file_size = file.getSize();
	if (file_size == 0)
	{
		// File is empty.
		data = nullptr;
	}
	else
	{		
		data = (U8*) ll_aligned_malloc_16(file_size);
		file.read(data, file_size);	/* Flawfinder: ignore */ 
		
		if (file.getLastBytesRead() != (S32)file_size)
		{
			ll_aligned_free_16(data);
			data = nullptr;
			file_size = 0;
		}
	}
	if (bytes_read)
	{
		*bytes_read = file_size;
	}
	return data;
}
	
void LLVFile::setReadPriority(const F32 priority)
{
	mPriority = priority;
	if (mHandle != LLVFSThread::nullHandle())
	{
		sVFSThread->setPriority(mHandle, threadPri());
	}
}

BOOL LLVFile::isReadComplete()
{
	BOOL res = TRUE;
	if (mHandle != LLVFSThread::nullHandle())
	{
		LLVFSThread::Request* req = (LLVFSThread::Request*)sVFSThread->getRequest(mHandle);
		LLVFSThread::status_t status = req->getStatus();
		if (status == LLVFSThread::STATUS_COMPLETE)
		{
			mBytesRead = req->getBytesRead();
			mPosition += mBytesRead;
			sVFSThread->completeRequest(mHandle);
			mHandle = LLVFSThread::nullHandle();
		}
		else
		{
			res = FALSE;
		}
	}
	return res;
}

S32 LLVFile::getLastBytesRead()
{
	return mBytesRead;
}

BOOL LLVFile::eof()
{
	return mPosition >= getSize();
}

BOOL LLVFile::write(const U8 *buffer, S32 bytes)
{
	if (! (mMode & WRITE))
	{
		LL_WARNS() << "Attempt to write to file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
	}
	if (mHandle != LLVFSThread::nullHandle())
	{
		LL_ERRS() << "Attempt to write to vfile object " << mFileID << " with pending async operation" << LL_ENDL;
		return FALSE;
	}
	BOOL success = TRUE;
	
	// *FIX: allow async writes? potential problem wit mPosition...
	if (mMode == APPEND) // all appends are async (but WRITEs are not)
	{	
		U8* writebuf = new U8[bytes];
		memcpy(writebuf, buffer, bytes);
		S32 offset = -1;
		mHandle = sVFSThread->write(mVFS, mFileID, mFileType,
									writebuf, offset, bytes,
									LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_AUTO_DELETE);
		mHandle = LLVFSThread::nullHandle(); // FLAG_AUTO_COMPLETE means we don't track this
	}
	else
	{
		// We can't do a write while there are pending reads or writes on this file
		waitForLock(VFSLOCK_READ);
		waitForLock(VFSLOCK_APPEND);

		S32 pos = (mMode & APPEND) == APPEND ? -1 : mPosition;

		S32 wrote = sVFSThread->writeImmediate(mVFS, mFileID, mFileType, (U8*)buffer, pos, bytes);

		mPosition += wrote;
		
		if (wrote < bytes)
		{	
			LL_WARNS() << "Tried to write " << bytes << " bytes, actually wrote " << wrote << LL_ENDL;

			success = FALSE;
		}
	}
	return success;
}

//static
BOOL LLVFile::writeFile(const U8 *buffer, S32 bytes, LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type, LLVFile::WRITE);
	file.setMaxSize(bytes);
	return file.write(buffer, bytes);
}

BOOL LLVFile::seek(S32 offset, S32 origin)
{
	if (mMode == APPEND)
	{
		LL_WARNS() << "Attempt to seek on append-only file" << LL_ENDL;
		return FALSE;
	}

	if (-1 == origin)
	{
		origin = mPosition;
	}

	S32 new_pos = origin + offset;

	S32 size = getSize(); // Calls waitForLock(VFSLOCK_APPEND)

	if (new_pos > size)
	{
		LL_WARNS() << "Attempt to seek past end of file" << LL_ENDL;

		mPosition = size;
		return FALSE;
	}
	else if (new_pos < 0)
	{
		LL_WARNS() << "Attempt to seek past beginning of file" << LL_ENDL;

		mPosition = 0;
		return FALSE;
	}

	mPosition = new_pos;
	return TRUE;
}

S32 LLVFile::tell() const
{
	return mPosition;
}

S32 LLVFile::getSize()
{
	waitForLock(VFSLOCK_APPEND);
	S32 size = mVFS->getSize(mFileID, mFileType);

	return size;
}

S32 LLVFile::getMaxSize()
{
	S32 size = mVFS->getMaxSize(mFileID, mFileType);

	return size;
}

BOOL LLVFile::setMaxSize(S32 size)
{
	if (! (mMode & WRITE))
	{
		LL_WARNS() << "Attempt to change size of file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;

		return FALSE;
	}

	if (!mVFS->checkAvailable(size))
	{
		//LL_RECORD_BLOCK_TIME(FTM_VFILE_WAIT);
		S32 count = 0;
		while (sVFSThread->getPending() > 1000)
		{
			if (count % 100 == 0)
			{
				LL_INFOS() << "VFS catching up... Pending: " << sVFSThread->getPending() << LL_ENDL;
			}
			if (sVFSThread->isPaused())
			{
				sVFSThread->update(0);
			}
			ms_sleep(10);
		}
	}
	return mVFS->setMaxSize(mFileID, mFileType, size);
}

BOOL LLVFile::rename(const LLUUID &new_id, const LLAssetType::EType new_type)
{
	if (! (mMode & WRITE))
	{
		LL_WARNS() << "Attempt to rename file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;

		return FALSE;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		LL_WARNS() << "Renaming file with pending async read" << LL_ENDL;
	}

	waitForLock(VFSLOCK_READ);
	waitForLock(VFSLOCK_APPEND);

	// we need to release / replace our own lock
	// since the renamed file will inherit locks from the new name
	mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
	mVFS->renameFile(mFileID, mFileType, new_id, new_type);
	mVFS->incLock(new_id, new_type, VFSLOCK_OPEN);
	
	mFileID = new_id;
	mFileType = new_type;

	return TRUE;
}

BOOL LLVFile::remove()
{
// 	LL_INFOS() << "Removing file " << mFileID << LL_ENDL;
	
	if (! (mMode & WRITE))
	{
		// Leaving paranoia warning just because this should be a very infrequent
		// operation.
		LL_WARNS() << "Remove file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		LL_WARNS() << "Removing file with pending async read" << LL_ENDL;
	}
	
	// why not seek back to the beginning of the file too?
	mPosition = 0;

	waitForLock(VFSLOCK_READ);
	waitForLock(VFSLOCK_APPEND);
	mVFS->removeFile(mFileID, mFileType);

	return TRUE;
}

// static
void LLVFile::initClass(LLVFSThread* vfsthread)
{
	if (!vfsthread)
	{
		if (LLVFSThread::sLocal != nullptr)
		{
			vfsthread = LLVFSThread::sLocal;
		}
		else
		{
			vfsthread = new LLVFSThread();
			sAllocdVFSThread = TRUE;
		}
	}
	sVFSThread = vfsthread;
}

// static
void LLVFile::cleanupClass()
{
	if (sAllocdVFSThread)
	{
		delete sVFSThread;
	}
	sVFSThread = nullptr;
}

bool LLVFile::isLocked(EVFSLock lock)
{
	return mVFS->isLocked(mFileID, mFileType, lock) ? true : false;
}

void LLVFile::waitForLock(EVFSLock lock)
{
	//LL_RECORD_BLOCK_TIME(FTM_VFILE_WAIT);
	// spin until the lock clears
	while (isLocked(lock))
	{
		if (sVFSThread->isPaused())
		{
			sVFSThread->update(0);
		}
		ms_sleep(1);
	}
}
