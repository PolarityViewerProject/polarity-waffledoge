# -*- cmake -*-
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindPkgConfig)

  pkg_check_modules(COLLADADOM REQUIRED colladadom)
else (USESYSTEMLIBS)
  use_prebuilt_binary(colladadom)
  set(COLLADADOM_INCLUDE_DIRS 
      ${LIBS_PREBUILT_DIR}/include/collada    
      ${LIBS_PREBUILT_DIR}/include/collada/1.4
      )
  if (WINDOWS)
    set(COLLADADOM_LIBRARIES
        debug libcollada14dom23-sd
        optimized libcollada14dom23-s
        )
  else(WINDOWS)
    set(COLLADADOM_LIBRARIES
        debug collada14dom-d
        optimized collada14dom
        minizip
        )
  endif (WINDOWS)
endif (USESYSTEMLIBS)