if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

# set(PVDATA_SYSTEM TRUE CACHE BOOL "Master setting to compile PVDAta into the viewer" FORCE)
# if(PVDATA_SYSTEM)
  # add_definitions("-DPVDATA_SYSTEM")
# else(NOT PVDATA_SYSTEM)
  # add_definitions("-UPVDATA_SYSTEM")
# endif(PVDATA_SYSTEM)

# Build process tweaks
set(COMPILER_JOBS "" CACHE STRING "Amount of simultaneous compiler jobs")
endif()
