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

#if OMP_ENABLE
#include <omp.h>
#pragma warning (disable : 4265)
#include <thread> // <polarity/>
#endif OMP_ENABLE

//class PVOpenMP
namespace PVOpenMP
{
	//LOG_CLASS(PVOpenMP); // This adds the class/function tag to the log entry

	static int getCPUCoresAmount()
	{
#if OMP_ENABLE
		return std::thread::hardware_concurrency();
#else // NO OPENMP
		// Fall back to a safe number. Lower to 2 for builds intended for low power machines.
#if LOW_POWER_BUILD
		return 2;
#else // NO LOW POWER BUILD
		return 4;
#endif // LOW_POWER_BUILD
#endif // OMP_ENABLE
	}
	const int CPU_CORES = PVOpenMP::getCPUCoresAmount();
	//const int PVInternalsMaxThreads = (CPU_CORES + (CPU_CORES / 2));
#if LOW_POWER_BUILD
	const int PVInternalsMaxThreads = (CPU_CORES + 1);
#else
	const int PVInternalsMaxThreads = (CPU_CORES * 4);
#endif // LOW_POWER_BUILD

	inline static void setOpenMPThreadsCount()
	{
#if OMP_ENABLE
#if OMP_MANUAL_THREADS
		omp_set_dynamic(false);     // Explicitly disable dynamic teams
		omp_set_num_threads(PVInternalsMaxThreads); // Use 'n' threads for all consecutive parallel regions
#else
		omp_set_dynamic(true);     // Explicitly enable dynamic teams
#endif // OMP_MANUAL_THREADS
#endif
	}

	static void initThreadingParameters()
	{
		getCPUCoresAmount();
		setOpenMPThreadsCount();
	}
};
#endif // PV_OMP_H
