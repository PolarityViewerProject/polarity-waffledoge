# <polarity> This is how you define preprocessor definitions...
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
option(USE_SSE3 "Use the SSE3 Instruction Set whenever possible" OFF)

if(USE_AVX)
  set(USE_SSE3 OFF CACHE BOOL "Use the SSE3 Instruction Set whenever possible" FORCE)
endif(USE_AVX)

# <Polarity> Make sure our feature flags are passed on to the preprocessor/compiler...
add_definitions(
  /DENABLE_MESH_UPLOAD=${ENABLE_MESH_UPLOAD}
  /DINCREMENTAL_LINK=${INCREMENTAL_LINK}
  /DPVDATA_COLORIZER=${PVDATA_COLORIZER}
  /DPVDATA_MOTD_CHAT=${PVDATA_MOTD_CHAT}
  /DPVDATA_MOTD=${PVDATA_MOTD}
  /DPVDATA_PROGRESS_TIPS=${PVDATA_PROGRESS_TIPS}
  /DPVDATA_UUID_LOCKDOWN=${PVDATA_UUID_LOCKDOWN}
  /DPVDATA_UUID_LOCKTO="${PVDATA_UUID_LOCKTO}"
  /DUSE_LTO=${USE_LTO}
  /DUSE_AVX=${USE_AVX}
  /DUSE_SSE=${USE_SSE3}
  )

MESSAGE("======== *FEATURES* ========")
MESSAGE("ENABLE_MESH_UPLOAD                 ${ENABLE_MESH_UPLOAD}")
MESSAGE("INCREMENTAL_LINK                   ${INCREMENTAL_LINK}")
MESSAGE("PVDATA_COLORIZER                   ${PVDATA_COLORIZER}")
MESSAGE("PVDATA_COLORIZER                   ${PVDATA_COLORIZER}")
MESSAGE("PVDATA_MOTD                        ${PVDATA_MOTD}")
MESSAGE("PVDATA_MOTD_CHAT                   ${PVDATA_MOTD_CHAT}")
MESSAGE("PVDATA_PROGRESS_TIPS               ${PVDATA_PROGRESS_TIPS}")
MESSAGE("USE_LTO                            ${USE_LTO}")
if(USE_AVX)
MESSAGE("Minimum Optimization:              AVX")
else(USE_SSE3)
MESSAGE("Minimum Optimization:              SSE3")
else()
MESSAGE("Minimum Optimization:              SSE2")
endif(USE_AVX)
if(PVDATA_UUID_LOCKDOWN)
  MESSAGE("THIS VIEWER WILL BE LOCKED DOWN TO 'secondlife:///app/agent/${PVDATA_UUID_LOCKTO}/about'")
endif(PVDATA_UUID_LOCKDOWN)
MESSAGE("============================")
# </polarity>
