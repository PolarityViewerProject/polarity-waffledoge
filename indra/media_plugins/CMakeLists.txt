# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
    add_subdirectory(libvlc)
endif (LINUX)

if (DARWIN)
    add_subdirectory(quicktime)
    add_subdirectory(cef)
endif (DARWIN)

    add_subdirectory(cef)
    add_subdirectory(libvlc)
# add_subdirectory(example)
