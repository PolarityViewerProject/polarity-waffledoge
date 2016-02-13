# -*- cmake -*-

include(Variables)
include(FreeType)
include(GLM)
include(GLEW)

set(LLRENDER_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llrender
	${GLM_INCLUDE_DIR}
    )

if (BUILD_HEADLESS)
  set(LLRENDER_HEADLESS_LIBRARIES
      llrenderheadless
      )
endif (BUILD_HEADLESS)
set(LLRENDER_LIBRARIES
    llrender
    )

