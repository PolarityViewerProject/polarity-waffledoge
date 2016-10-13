/**
 * @file pvomp.h
 * @brief Conditional OpenMP include
 *
 * $LicenseInfo:firstyear=2014&license=plvrlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015, Xenhat Liamano
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#pragma once

#ifndef PV_OMP_H
#define PV_OMP_H

#if defined(OpenMP_Support)
#include <omp.h>
#pragma warning (disable : 4265)
#include <thread>
#endif // OMP_ENABLE

//class PVThreading
namespace PVThreading
{
	//LOG_CLASS(PVThreading); // This adds the class/function tag to the log entry

	static int getCPUCoresAmount()
	{
#if defined(OpenMP_Support)
		return std::thread::hardware_concurrency();
#else // NO OPENMP
		// Fall back to a safe number. Lower to 2 for builds intended for low power machines.
#ifdef LOW_POWER_BUILD
		return 2;
#else // NO LOW POWER BUILD
		return 4;
#endif // LOW_POWER_BUILD
#endif // OMP_ENABLE
	}
	const int CPU_CORES = getCPUCoresAmount();
	//const int mCPUThreadNumber = (CPU_CORES + (CPU_CORES / 2));
#ifdef LOW_POWER_BUILD
	const int mCPUThreadNumber = (CPU_CORES + 1);
#else
	const int mCPUThreadNumber = (CPU_CORES * 4);
#endif // LOW_POWER_BUILD

	inline static void setTheadCount()
	{
#if defined(OpenMP_Support)
#if defined(OpenMP_ManualThreading)
		omp_set_dynamic(false);     // Explicitly disable dynamic teams
		omp_set_num_threads(mCPUThreadNumber); // Use 'n' threads for all consecutive parallel regions
#else
		omp_set_dynamic(true);     // Explicitly enable dynamic teams
#endif // OMP_MANUAL_THREADS
#endif // OMP_ENABLE
	}

	static void initParameters()
	{
		getCPUCoresAmount();
		setTheadCount();
	}
};
#endif // PV_OMP_H
