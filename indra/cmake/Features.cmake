# <polarity> Our feature list. Included in Variables.cmake, please do not include manually.

# Make sure it doesn't happen anyway.
if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

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
set(COMPILER_JOBS "8" CACHE STRING "Amount of simultaneous compiler jobs")

# Optimizations
# Note: Default is to use any available. Performance may vary on different CPUs.
option(USE_AVX "[GLOBAL]Use AVX Instrinsics whenever possible" OFF)
option(USE_SSE3 "[GLM]Use SSE3 Instrinsics whenever possible" OFF)
option(RESTRICT_SSE2 "[GLM]Restrict to SSE2 Instrinsics" OFF)
option(RESTRICT_PURE "[GLM]Do not use SIMD Intrinsics at all" OFF)

# Libraries
option(USE_TCMALLOC " Build with Google PerfTools support." OFF)

option(RELEASE_BUILD "Used to help configure release binaries" OFF)
# </polarity>

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
