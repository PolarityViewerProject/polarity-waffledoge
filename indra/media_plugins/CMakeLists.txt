# -*- cmake -*-

include(Variables)

add_subdirectory(base)

if (ENABLE_MEDIA_PLUGINS)
if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (WINDOWS)
    if(LIBVLCPLUGIN)
    add_subdirectory(libvlc)
    endif(LIBVLCPLUGIN)
endif (WINDOWS)

add_subdirectory(cef)

endif (ENABLE_MEDIA_PLUGINS)
