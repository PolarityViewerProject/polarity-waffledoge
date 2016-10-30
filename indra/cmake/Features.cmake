# <polarity> Our feature list. Included in Variables.cmake, please do not include manually.

# Make sure it doesn't happen anyway.
if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

MESSAGE("======== *FEATURES* ========")

option(PVDATA_COLORIZER "Color avatar names and various other elements based on their role in the project" OFF)
option(PVDATA_MOTD "Use PVData-served Message of the Day" OFF)
option(PVDATA_MOTD_CHAT "Display a MOTD in chat at login" OFF)
option(PVDATA_PROGRESS_TIPS "Use Progress Tips" OFF)
option(PVDATA_UUID_LOCKDOWN "Lock down the viewer to a specific user" OFF)
set(PVDATA_UUID_LOCKTO "${PVDATA_UUID_LOCKTO}" CACHE STRING "UUID to lock down to" FORCE)

# Audio Engine
option(FMODSTUDIO "Build with support for the FMOD Studio audio engine" OFF)

# Workarounds
option(ENABLE_MESH_UPLOAD "Enable the Mesh Uploader menu items" OFF)

# Experimental viewer features
option(GL_TRANSFORM_FEEDBACK_BUFFER "Use OpenGL Transform Feedback Buffer" OFF)

# Build process tweaks
set(COMPILER_JOBS "" CACHE STRING "Amount of simultaneous compiler jobs")

# Optimizations
option(USE_AVX "[GLOBAL]Use AVX Instrinsics whenever possible" OFF)
option(USE_SSE3 "[GLM]Use SSE3 Instrinsics whenever possible" OFF)
option(RESTRICT_SSE2 "[GLM]Restrict to SSE2 Instrinsics" OFF)
option(RESTRICT_PURE "[GLM]Do not use SIMD Intrinsics at all" OFF)

# Libraries
option(USE_TCMALLOC " Build with Google PerfTools support." OFF)

option(RELEASE_BUILD "Used to help configure release binaries" OFF)
option(DEVEL_BUILD "Development build. May include slow debugging code" OFF)

if(RELEASE_BUILD)
  add_definitions(/DRELEASE_BUILD=TRUE)
  unset(DEVEL_BUILD)
else(DEVEL_BUILD)
  add_definitions(/DDEVEL_BUILD=TRUE)
  unset(RELEASE_BUILD)
endif()

# Add these CMake flags to the C++ preprocessor to toggle code that way
add_definitions(
  /DENABLE_MESH_UPLOAD=${ENABLE_MESH_UPLOAD}
  /DINCREMENTAL_LINK=${INCREMENTAL_LINK}
  /DPVDATA_COLORIZER=${PVDATA_COLORIZER}
  /DPVDATA_MOTD=${PVDATA_MOTD}
  /DPVDATA_MOTD_CHAT=${PVDATA_MOTD_CHAT}
  /DPVDATA_PROGRESS_TIPS=${PVDATA_PROGRESS_TIPS}
  /DPVDATA_UUID_LOCKDOWN=${PVDATA_UUID_LOCKDOWN}
  /DPVDATA_UUID_LOCKTO="${PVDATA_UUID_LOCKTO}"
  /DUSE_LTO=${USE_LTO}
  )

MESSAGE("ENABLE_MESH_UPLOAD                 ${ENABLE_MESH_UPLOAD}")
MESSAGE("INCREMENTAL_LINK                   ${INCREMENTAL_LINK}")
MESSAGE("PVDATA_COLORIZER                   ${PVDATA_COLORIZER}")
MESSAGE("PVDATA_MOTD                        ${PVDATA_MOTD}")
MESSAGE("PVDATA_MOTD_CHAT                   ${PVDATA_MOTD_CHAT}")
MESSAGE("PVDATA_PROGRESS_TIPS               ${PVDATA_PROGRESS_TIPS}")
MESSAGE("USE_LTO                            ${USE_LTO}")

if(USE_AVX)
  add_definitions(
    /DGLM_FORCE_AVX=TRUE
    )
  MESSAGE("Preferred SIMD intrinsics:         AVX")
  unset(GLM_FORCE_SSE3)
  unset(GLM_FORCE_SSE2)
  unset(GLM_FORCE_PURE)
endif()
if(USE_SSE3)
  add_definitions(
    /DGLM_FORCE_SSE3=TRUE
    )
  MESSAGE("Preferred SIMD intrinsics:         SSE3")
  unset(GLM_FORCE_AVX)
  unset(GLM_FORCE_SSE2)
  unset(GLM_FORCE_PURE)
endif()
if(RESTRICT_SSE2)
  add_definitions(
    /DGLM_FORCE_SSE2=TRUE
    )
  MESSAGE("Preferred SIMD intrinsics:         SSE2")
  unset(GLM_FORCE_AVX)
  unset(GLM_FORCE_SSE3)
  unset(GLM_FORCE_PURE)
endif()
if(RESTRICT_PURE)
  add_definitions(
    /DGLM_FORCE_PURE=TRUE
    )
  MESSAGE("Preferred SIMD intrinsics:         PURE")
  unset(GLM_FORCE_AVX)
  unset(GLM_FORCE_SSE3)
  unset(GLM_FORCE_SSE2)
endif()

if(RELEASE_BUILD)
MESSAGE("THIS IS A RELEASE BUILD: ANYONE NOT BANNED CAN USE IT")
else(DEVEL_BUILD)
MESSAGE("THIS IS A DEVELOPMENT BUILD: ONLY DEVELOPERS CAN USE IT")
endif()
if(PVDATA_UUID_LOCKDOWN)
  MESSAGE("THIS VIEWER WILL BE LOCKED DOWN TO '${PVDATA_UUID_LOCKTO}'")
endif(PVDATA_UUID_LOCKDOWN)
MESSAGE("============================")
# </polarity>

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
