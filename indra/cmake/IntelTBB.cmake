# -*- cmake -*-
include(Prebuilt)

if(TBBMALLOC_ENABLE)
  if(STANDALONE)
  else(STANDALONE)
    use_prebuilt_binary(inteltbb)
    if(WINDOWS)
      set(TBBMALLOC_LIBRARIES 
          debug tbbmalloc_proxy_debug
          optimized tbbmalloc_proxy
          )
      if(WORD_SIZE EQUAL 64)
         set(TBBMALLOC_LINK_FLAGS  "/INCLUDE:__TBB_malloc_proxy")
      else(WORD_SIZE EQUAL 64) 
         set(TBBMALLOC_LINK_FLAGS  "/INCLUDE:___TBB_malloc_proxy")
      endif(WORD_SIZE EQUAL 64)
    elseif(LINUX)
      set(TBBMALLOC_LIBRARIES 
          debug tbbmalloc_proxy_debug
          optimized tbbmalloc_proxy
          )
    endif(WINDOWS)
  endif(STANDALONE)
else(TBBMALLOC_ENABLE)
  set(TBBMALLOC_LIBRARIES "")
  set(TBBMALLOC_LINK_FLAGS "")
endif(TBBMALLOC_ENABLE)
