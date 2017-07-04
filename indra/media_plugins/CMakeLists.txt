# -*- cmake -*-
include(Variables)
add_subdirectory(base)

if (USE_MEDIA_PLUGINS)
if (LINUX)
    add_subdirectory(gstreamer010)
    if(LINK_VLC_PLUGIN)
    add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (LINUX)

if (DARWIN)
    add_subdirectory(cef)
endif (DARWIN)

if (WINDOWS)
    add_subdirectory(cef)
    if(LINK_VLC_PLUGIN)
    add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (WINDOWS)
endif (USE_MEDIA_PLUGINS)
