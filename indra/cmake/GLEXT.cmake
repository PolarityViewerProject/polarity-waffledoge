# -*- cmake -*-
include(Prebuilt)

if (NOT USESYSTEMLIBS)
  if (WINDOWS OR LINUX)
    use_prebuilt_binary(glext)
  endif (WINDOWS OR LINUX)
  set(GLEXT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (NOT USESYSTEMLIBS)
