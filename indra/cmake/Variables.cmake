# -*- cmake -*-
#
# Definitions of variables used throughout the Second Life build
# process.
#
# Platform variables:
#
#   DARWIN  - Mac OS X
#   LINUX   - Linux
#   WINDOWS - Windows


# Relative and absolute paths to subtrees.

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

if(NOT DEFINED COMMON_CMAKE_DIR)
    set(COMMON_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")
endif(NOT DEFINED COMMON_CMAKE_DIR)

Include(PVData)

set(LIBS_CLOSED_PREFIX)
set(LIBS_OPEN_PREFIX)
set(SCRIPTS_PREFIX ../scripts)
set(VIEWER_PREFIX)
set(INTEGRATION_TESTS_PREFIX)
set(LL_TESTS OFF CACHE BOOL "Build and run unit and integration tests (disable for build timing runs to reduce variation")

# Compiler and toolchain options
option(INCREMENTAL_LINK "Use incremental linking or incremental LTCG for LTO on win32 builds (enable for faster links on some machines)" OFF)
option(USE_LTO "Enable Whole Program Optimization and related folding and binary reduction routines" OFF)

if(LIBS_CLOSED_DIR)
  file(TO_CMAKE_PATH "${LIBS_CLOSED_DIR}" LIBS_CLOSED_DIR)
else(LIBS_CLOSED_DIR)
  set(LIBS_CLOSED_DIR ${CMAKE_SOURCE_DIR}/${LIBS_CLOSED_PREFIX})
endif(LIBS_CLOSED_DIR)
if(LIBS_COMMON_DIR)
  file(TO_CMAKE_PATH "${LIBS_COMMON_DIR}" LIBS_COMMON_DIR)
else(LIBS_COMMON_DIR)
  set(LIBS_COMMON_DIR ${CMAKE_SOURCE_DIR}/${LIBS_OPEN_PREFIX})
endif(LIBS_COMMON_DIR)
set(LIBS_OPEN_DIR ${LIBS_COMMON_DIR})

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "http://bitbucket.org/lindenlab/master-message-template/raw/tip/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
  if (WORD_SIZE EQUAL 64)
    set(ARCH x86_64 CACHE STRING "Viewer Architecture")
    set(LL_ARCH ${ARCH}_win64)
    set(LL_ARCH_DIR ${ARCH}-win64)
    set(WORD_SIZE 64)
    set(AUTOBUILD_PLATFORM_NAME "windows64")
  else (WORD_SIZE EQUAL 64)
    set(ARCH i686 CACHE STRING "Viewer Architecture")
    set(LL_ARCH ${ARCH}_win32)
    set(LL_ARCH_DIR ${ARCH}-win32)
    set(WORD_SIZE 32)
    set(AUTOBUILD_PLATFORM_NAME "windows")
  endif (WORD_SIZE EQUAL 64)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)

  # If someone has specified a word size, use that to determine the
  # architecture.  Otherwise, let the architecture specify the word size.
  if (WORD_SIZE EQUAL 32)
    #message(STATUS "WORD_SIZE is 32")
    set(ARCH i686)
    set(AUTOBUILD_PLATFORM_NAME "linux")
  elseif (WORD_SIZE EQUAL 64)
    #message(STATUS "WORD_SIZE is 64")
    set(ARCH x86_64)
    set(AUTOBUILD_PLATFORM_NAME "linux64")
  else (WORD_SIZE EQUAL 32)
    #message(STATUS "WORD_SIZE is UNDEFINED")
    execute_process(COMMAND uname -m COMMAND sed s/i.86/i686/
                    OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (ARCH STREQUAL x86_64)
      #message(STATUS "ARCH is detected as 64; ARCH is ${ARCH}")
      set(WORD_SIZE 64)
    else (ARCH STREQUAL x86_64)
      #message(STATUS "ARCH is detected as 32; ARCH is ${ARCH}")
      set(WORD_SIZE 32)
    endif (ARCH STREQUAL x86_64)
  endif (WORD_SIZE EQUAL 32)

  if (WORD_SIZE EQUAL 32)
    set(DEB_ARCHITECTURE i386)
    set(FIND_LIBRARY_USE_LIB64_PATHS OFF)
    set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib32 ${CMAKE_SYSTEM_LIBRARY_PATH})
  else (WORD_SIZE EQUAL 32)
    set(DEB_ARCHITECTURE amd64)
    set(FIND_LIBRARY_USE_LIB64_PATHS ON)
  endif (WORD_SIZE EQUAL 32)

  execute_process(COMMAND dpkg-architecture -a${DEB_ARCHITECTURE} -qDEB_HOST_MULTIARCH 
      RESULT_VARIABLE DPKG_RESULT
      OUTPUT_VARIABLE DPKG_ARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  #message (STATUS "DPKG_RESULT ${DPKG_RESULT}, DPKG_ARCH ${DPKG_ARCH}")
  if (DPKG_RESULT EQUAL 0)
    set(CMAKE_LIBRARY_ARCHITECTURE ${DPKG_ARCH})
    set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib/${DPKG_ARCH} /usr/local/lib/${DPKG_ARCH} ${CMAKE_SYSTEM_LIBRARY_PATH})
  endif (DPKG_RESULT EQUAL 0)

  include(ConfigurePkgConfig)

  set(LL_ARCH ${ARCH}_linux)
  set(LL_ARCH_DIR ${ARCH}-linux)

  if (INSTALL_PROPRIETARY)
    # Only turn on headless if we can find osmesa libraries.
    include(FindPkgConfig)
    #pkg_check_modules(OSMESA osmesa)
    #if (OSMESA_FOUND)
    #  set(BUILD_HEADLESS ON CACHE BOOL "Build headless libraries.")
    #endif (OSMESA_FOUND)
  endif (INSTALL_PROPRIETARY)

endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN 1)
  
  # now we only support Xcode 7.0 using 10.11 (El Capitan), minimum OS 10.7 (Lion)
  set(XCODE_VERSION 7.0)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
  set(CMAKE_OSX_SYSROOT macosx10.11)

  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL 3)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_STRICT_ALIASING NO)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_FAST_MATH NO)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_X86_VECTOR_INSTRUCTIONS ssse3)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++")
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf-with-dsym)

  # Build only for i386 by default, system default on MacOSX 10.6+ is x86_64
  if (NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "i386")
  endif (NOT CMAKE_OSX_ARCHITECTURES)

  set(ARCH ${CMAKE_OSX_ARCHITECTURES})
  set(LL_ARCH ${ARCH}_darwin)
  set(LL_ARCH_DIR universal-darwin)
  set(WORD_SIZE 32)
  set(AUTOBUILD_PLATFORM_NAME "darwin")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(VIEWER_CHANNEL "Polarity Test" CACHE STRING "Viewer Channel Name")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")
set(USESYSTEMLIBS OFF CACHE BOOL "Use libraries from your system rather than Linden-supplied prebuilt libraries.")

set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable use of precompiled header directives where supported.")

option(UNATTENDED "Disable use of uneeded tooling for automated builds" ON)

# <polarity> Our feature list.
# Build process tweaks
set(COMPILER_JOBS "" CACHE STRING "Amount of simultaneous compiler jobs")
option(INTERNAL_BUILD "Build reserved for internal testing" OFF)

if(NOT DEFINED INTERNAL_BUILD)
  set(INTERNAL_BUILD OFF)
endif()

# Third-party libraries
# Audio Engine
option(FMODSTUDIO "Build with support for the FMOD Studio audio engine" ON)
# Mallocs
option(USE_TCMALLOC " Build with Google PerfTools support." OFF)
option(USE_TBBMALLOC "Build the viewer with intel tbbmalloc" OFF)
# APIs
option(NVAPI "Use nvapi driver interface library" OFF)
option(LINK_VLC_PLUGIN "Compile with the LibVLC Plugin. Requires MPEG-LA AVC/H.264 license to distribute." OFF) # Not handled by INSTALL_PROPRIETARY on purpose

## sanity check
if (USE_TCMALLOC AND LINK_TBBMALLOC)
  message(FATAL_ERROR "Only one malloc may be enabled at a time.")
endif (USE_TCMALLOC AND LINK_TBBMALLOC)

# option(INSTALL_PROPRIETARY "Install proprietary binaries" OFF) # Defined in autobuild.xml, don't define here until we have to.
if(INSTALL_PROPRIETARY)
  set(FMODSTUDIO ON)
  set(NVAPI ON)
  set(USE_TBBMALLOC ON)
endif(INSTALL_PROPRIETARY)

if(FMODSTUDIO)
  set(LINK_FMODSTUDIO ON CACHE BOOL "Using FMOD Studio sound library.")
endif(FMODSTUDIO)

if(NVAPI)
  set(LINK_NVAPI ON CACHE BOOL "Using Nvidia API")
endif(NVAPI)

if(USE_TBBMALLOC)
  set(LINK_TBBMALLOC ON CACHE BOOL "Using Intel TBB")
endif(USE_TBBMALLOC)


MESSAGE("")
MESSAGE("======== *Configuration* ========")
MESSAGE("Target Platform        ${AUTOBUILD_PLATFORM_NAME}")
MESSAGE("Incremental Link       ${INCREMENTAL_LINK}")
MESSAGE("Link-Time CodeGen      ${USE_LTO}")
MESSAGE("Internal Build         ${INTERNAL_BUILD}")
MESSAGE("========== *Libraries* ==========")
MESSAGE("FMOD Studio            ${FMODSTUDIO}")
MESSAGE("NVIDIA API             ${NVAPI}")
MESSAGE("Intel Building Blocks  ${USE_TBBMALLOC}")
MESSAGE("Licensed VLC Plugin    ${LINK_VLC_PLUGIN}")
MESSAGE("========== *PVData* ==========")
MESSAGE("PVData System          ${PVDATA_SYSTEM}")
MESSAGE("==============================")
# Add these CMake flags to the C++ preprocessor to toggle code that way, or at least Intellisense to detect them.
add_definitions(
  /DINCREMENTAL_LINK=${INCREMENTAL_LINK}
  /DUSE_LTO=${USE_LTO}
  /DINTERNAL_BUILD=${INTERNAL_BUILD}
  /DLINK_VLC_PLUGIN=${LINK_VLC_PLUGIN}
  )
if(PVDATA_UUID_LOCKDOWN)
  MESSAGE("THIS VIEWER WILL BE LOCKED DOWN TO '${PVDATA_UUID_LOCKTO}'")
endif(PVDATA_UUID_LOCKDOWN)
# </polarity>

source_group("CMake Rules" FILES CMakeLists.txt)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
