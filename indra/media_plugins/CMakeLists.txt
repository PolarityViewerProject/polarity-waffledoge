# -*- cmake -*-

include(Variables)

add_subdirectory(base)

if (ENABLE_MEDIA_PLUGINS)
if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (WINDOWS)
    if(LINK_VLC_PLUGIN)
    add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (WINDOWS)

add_subdirectory(cef)

endif (ENABLE_MEDIA_PLUGINS)
