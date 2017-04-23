if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

option(PVDATA_SYSTEM "Master setting to compile PVDAta into the viewer" ON)

option(PVDATA_COLORIZER "Color avatar names and various other elements based on their role in the project" ON)
option(PVDATA_MOTD "Use PVData-served Message of the Day" ON)
option(PVDATA_MOTD_CHAT "Display a MOTD in chat at login" ON)
option(PVDATA_PROGRESS_TIPS "Use Progress Tips" ON)
option(PVDATA_UUID_LOCKDOWN "Lock down the viewer to a specific user" OFF)
set(PVDATA_UUID_LOCKTO "${PVDATA_UUID_LOCKTO}" CACHE STRING "UUID to lock down to" FORCE)

if(NOT PVDATA_SYSTEM)
  set(PVDATA_COLORIZER OFF)
  set(PVDATA_MOTD OFF)
  set(PVDATA_MOTD_CHAT OFF)
  set(PVDATA_PROGRESS_TIPS OFF)
  set(PVDATA_UUID_LOCKDOWN OFF)
endif()
# For Intellisense
add_definitions(
  /DPVDATA_COLORIZER=${PVDATA_COLORIZER}
  /DPVDATA_MOTD=${PVDATA_MOTD}
  /DPVDATA_MOTD_CHAT=${PVDATA_MOTD_CHAT}
  /DPVDATA_PROGRESS_TIPS=${PVDATA_PROGRESS_TIPS}
  /DPVDATA_UUID_LOCKDOWN=${PVDATA_UUID_LOCKDOWN}
  /DPVDATA_UUID_LOCKTO="${PVDATA_UUID_LOCKTO}"
)
endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
