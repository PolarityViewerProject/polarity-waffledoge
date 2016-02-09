# -*- cmake -*-

include(Variables)
include(FreeType)
include(GLH)
include(GLM)
include(GLEW)

set(LLRENDER_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llrender
    ${GLH_INCLUDE_DIR}
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

