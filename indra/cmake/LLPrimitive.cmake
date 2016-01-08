# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)
include(ColladaDOM)


use_prebuilt_binary(libxml2)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )
if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        ${COLLADADOM_LIBRARIES}
        libxml2_a
        ${BOOST_SYSTEM_LIBRARIES}
        )
elseif (DARWIN)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        ${COLLADADOM_LIBRARIES}
        minizip
        xml2
        iconv           # Required by libxml2
        )
elseif (LINUX)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        ${COLLADADOM_LIBRARIES}
        minizip
        xml2
        )
endif (WINDOWS)

