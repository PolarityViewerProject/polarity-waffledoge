if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

## set(PVDATA_SYSTEM TRUE CACHE BOOL "Master setting to compile PVDAta into the viewer" FORCE)
## option(PVDATA_UUID_LOCKDOWN "Lock down the viewer to a specific user" OFF)
## set(PVDATA_UUID_LOCKTO "${PVDATA_UUID_LOCKTO}" CACHE STRING "UUID to lock down to" FORCE)
## 
## if(PVDATA_SYSTEM)
##   add_definitions("-DPVDATA_SYSTEM")
##   if(PVDATA_UUID_LOCKDOWN)
##     add_definitions("-DPVDATA_UUID_LOCKDOWN -DPVDATA_UUID_LOCKTO=${PVDATA_UUID_LOCKTO}")
##   endif(PVDATA_UUID_LOCKDOWN)
## else(NOT PVDATA_SYSTEM)
##   add_definitions("-UPVDATA_SYSTEM -UPVDATA_UUID_LOCKTO -UPVDATA_UUID_LOCKDOWN")
## endif(PVDATA_SYSTEM)
## 
## # <polarity> Our feature list.
## # Build process tweaks
## set(COMPILER_JOBS "" CACHE STRING "Amount of simultaneous compiler jobs")
## option(INTERNAL_BUILD "Build reserved for internal testing" OFF)
## 
## if(NOT DEFINED INTERNAL_BUILD)
##   set(INTERNAL_BUILD OFF)
## endif()
## 
## # option(INSTALL_PROPRIETARY "Install proprietary binaries" OFF) # Defined in autobuild.xml, don't define here until we have to.
## if(INSTALL_PROPRIETARY)
##   set(FMODSTUDIO ON)
##   set(NVAPI ON)
##   # set(USE_TBBMALLOC ON)
## endif(INSTALL_PROPRIETARY)
## 
## 
## if(PVDATA_UUID_LOCKDOWN)
##   MESSAGE(AUTHOR_WARNING "THIS VIEWER WILL BE LOCKED DOWN TO '${PVDATA_UUID_LOCKTO}'")
## endif(PVDATA_UUID_LOCKDOWN)
## # </polarity>

endif()
