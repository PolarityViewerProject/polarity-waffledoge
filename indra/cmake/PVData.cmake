if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

set(PVDATA_SYSTEM TRUE CACHE BOOL "Master setting to compile PVDAta into the viewer" FORCE)
option(PVDATA_UUID_LOCKDOWN "Lock down the viewer to a specific user" OFF)
set(PVDATA_UUID_LOCKTO "${PVDATA_UUID_LOCKTO}" CACHE STRING "UUID to lock down to" FORCE)

if(PVDATA_SYSTEM)
  add_definitions("-DPVDATA_SYSTEM")
  if(PVDATA_UUID_LOCKDOWN)
    add_definitions("-DPVDATA_UUID_LOCKDOWN -DPVDATA_UUID_LOCKTO=${PVDATA_UUID_LOCKTO}")
  endif(PVDATA_UUID_LOCKDOWN)
else(NOT PVDATA_SYSTEM)
  add_definitions("-UPVDATA_SYSTEM -UPVDATA_UUID_LOCKTO -UPVDATA_UUID_LOCKDOWN")
endif(PVDATA_SYSTEM)

# <polarity> Our feature list.
# Build process tweaks
set(COMPILER_JOBS "" CACHE STRING "Amount of simultaneous compiler jobs")
option(INTERNAL_BUILD "Build reserved for internal testing" OFF)

if(NOT DEFINED INTERNAL_BUILD)
  set(INTERNAL_BUILD OFF)
endif()

# option(INSTALL_PROPRIETARY "Install proprietary binaries" OFF) # Defined in autobuild.xml, don't define here until we have to.
if(INSTALL_PROPRIETARY)
  set(FMODSTUDIO ON)
  set(NVAPI ON)
  # set(USE_TBBMALLOC ON)
endif(INSTALL_PROPRIETARY)

# <polarity> automatically get APP_NAME from ROOT_PROJECT_NAME
add_definitions(/DROOT_PROJECT_NAME=${ROOT_PROJECT_NAME})

MESSAGE(STATUS "")
MESSAGE(STATUS "======== *Configuration* ========")
MESSAGE(STATUS "ROOT_PROJECT_NAME      ${ROOT_PROJECT_NAME}")
MESSAGE(STATUS "Build No.              ${BUILD_NUMBER}")
MESSAGE(STATUS "Target Platform        ${AUTOBUILD_PLATFORM_NAME}")
MESSAGE(STATUS "Incremental Link       ${INCREMENTAL_LINK}")
MESSAGE(STATUS "Link-Time CodeGen      ${USE_LTO}")
MESSAGE(STATUS "Internal Build         ${INTERNAL_BUILD}")
MESSAGE(STATUS "========== *Libraries* ==========")
MESSAGE(STATUS "FMOD Studio            ${FMODSTUDIO}")
MESSAGE(STATUS "NVIDIA API             ${NVAPI}")
MESSAGE(STATUS "Intel Building Blocks  ${USE_TBBMALLOC}")
MESSAGE(STATUS "Licensed VLC Plugin    ${LIBVLCPLUGIN}")
MESSAGE(STATUS "========== *PVData* =============")
MESSAGE(STATUS "PVData System          ${PVDATA_SYSTEM}")
MESSAGE(STATUS "=================================")
# Add these CMake flags to the C++ preprocessor to toggle code that way, or at least Intellisense to detect them.
add_definitions(
  /DINCREMENTAL_LINK=${INCREMENTAL_LINK}
  /DUSE_LTO=${USE_LTO}
  /DINTERNAL_BUILD=${INTERNAL_BUILD}
  /DLIBVLCPLUGIN=${LIBVLCPLUGIN}
  /DBUILD_NUMBER=${BUILD_NUMBER}
  /DENABLE_MEDIA_PLUGINS=${ENABLE_MEDIA_PLUGINS}
  )
if(PVDATA_UUID_LOCKDOWN)
  MESSAGE(AUTHOR_WARNING "THIS VIEWER WILL BE LOCKED DOWN TO '${PVDATA_UUID_LOCKTO}'")
endif(PVDATA_UUID_LOCKDOWN)
# </polarity>

endif()
