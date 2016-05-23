# <polarity> Our feature list. Included in Variables.cmake, please do not include manually.

# Make sure it doesn't happen anyway.
if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

# PVData Master Switch. Does not turn off PVData, but allows to easily disable all non-essential features.
option(PVDATA "PVData master switch for all non-essential components." OFF)
if (PVDATA)
  set(PVDATA_COLORIZER ${PVDATA_COLORIZER} CACHE BOOL "Color avatar names and various other elements based on their role in the project" FORCE)
  set(PVDATA_MOTD ${PVDATA_MOTD} CACHE BOOL "Use PVData-served Message of the Day" FORCE)
  set(PVDATA_MOTD_CHAT ${PVDATA_MOTD_CHAT} CACHE BOOL "Display a MOTD in chat at login" FORCE)
  set(PVDATA_PROGRESS_TIPS ${PVDATA_PROGRESS_TIPS} CACHE BOOL "Use Progress Tips" FORCE)
  set(PVDATA_UUID_LOCKDOWN ${PVDATA_UUID_LOCKDOWN} CACHE BOOL "Lock down the viewer to a specific user" FORCE)
  set(PVDATA_UUID_LOCKTO "${PVDATA_UUID_LOCKTO}" CACHE STRING "UUID to lock down to" FORCE)
elseif (NOT PVDATA)
  set(PVDATA_COLORIZER OFF CACHE BOOL "Color avatar names and various other elements based on their role in the project" FORCE)
  set(PVDATA_MOTD OFF CACHE BOOL "Use PVData-served Message of the Day" FORCE)
  set(PVDATA_MOTD_CHAT OFF CACHE BOOL "Display a MOTD in chat at login" FORCE)
  set(PVDATA_PROGRESS_TIPS OFF CACHE BOOL "Use Progress Tips" FORCE)
  set(PVDATA_UUID_LOCKDOWN OFF CACHE BOOL "Lock down the viewer to a specific user" FORCE)
  set(PVDATA_UUID_LOCKTO "" CACHE STRING "UUID to lock down to" FORCE)
endif(PVDATA)

# Audio Engine
option(FMODEX "Build with support for the FMOD Ex audio engine" ON)

# Workarounds
option(ENABLE_MESH_UPLOAD "Enable the Mesh Uploader menu items" OFF)

# Build process tweaks
set(COMPILER_JOBS ${COMPILER_JOBS} CACHE STRING "Amount of simultaneous compiler jobs" FORCE)

# Optimizations
option(USE_AVX "Use the AVX Instruction Set whenever possible" OFF)
option(USE_SSE3 "Use the SSE3 Instruction Set whenever possible" ON)

# Multi-threading
option(OMP_ENABLE "Multi-threading using OpenMP (up to v2.0 on Windows)" OFF)
option(OMP_IMAGEWORKER "Multi-thread the image worker process independently of the decode thread" ${OMP_IMAGEWORKER})
option(OMP_MANUAL_THREADS "Use Hard-coded amount of threads." OFF)

if(USE_AVX)
  set(USE_SSE3 OFF CACHE BOOL "Use the SSE3 Instruction Set whenever possible" FORCE)
endif(USE_AVX)

option(RELEASE_BUILD "Used to help configure release binaries" OFF)
# </polarity>

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
