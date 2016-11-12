# -*- cmake -*-
add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
    add_subdirectory(cef)
    if(LINK_VLC_PLUGIN)
        add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (LINUX)

if (DARWIN)
    add_subdirectory(quicktime)
    add_subdirectory(cef)
endif (DARWIN)

if (WINDOWS)
    add_subdirectory(cef)
    if(LINK_VLC_PLUGIN)
        add_subdirectory(libvlc)
    endif(LINK_VLC_PLUGIN)
endif (WINDOWS)

# add_subdirectory(example)

#if (WINDOWS OR LINUX)
#    add_subdirectory(gstreamer010)
#endif (WINDOWS OR LINUX)

