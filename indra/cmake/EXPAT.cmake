# -*- cmake -*-
include(Prebuilt)

set(EXPAT_FIND_QUIETLY ON)
set(EXPAT_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindEXPAT)
else (USESYSTEMLIBS)
    use_prebuilt_binary(expat)
    if (WINDOWS)
        if(WORD_SIZE EQUAL 32)
          set(EXPAT_LIBRARIES libexpatMT)
        elseif(WORD_SIZE EQUAL 64)
          set(EXPAT_LIBRARIES expat)
        endif(WORD_SIZE EQUAL 32)
    else (WINDOWS)
        set(EXPAT_LIBRARIES expat)
    endif (WINDOWS)
    set(EXPAT_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)
