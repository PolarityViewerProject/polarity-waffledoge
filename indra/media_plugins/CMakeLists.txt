# -*- cmake -*-

include(Variables)

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (WINDOWS)
    if(LINK_VLC_PLUGIN)
    add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (WINDOWS)

add_subdirectory(cef)

### add_subdirectory(example)
