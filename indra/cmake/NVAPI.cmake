# -*- cmake -*-
include(Prebuilt)
include(Variables)

if (LINK_NVAPI)
  if (WINDOWS)
    use_prebuilt_binary(nvapi)
    if (WORD_SIZE EQUAL 32)
      set(NVAPI_LIBRARY nvapi)
    elseif (WORD_SIZE EQUAL 64)
      set(NVAPI_LIBRARY nvapi64)
    endif (WORD_SIZE EQUAL 32)	
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (LINK_NVAPI)
  set(NVAPI_LIBRARY "")
endif (LINK_NVAPI)

