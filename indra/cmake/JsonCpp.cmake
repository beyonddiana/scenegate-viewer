# -*- cmake -*-

include(Prebuilt)

set(JSONCPP_FIND_QUIETLY ON)
set(JSONCPP_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindJsonCpp)
else (STANDALONE)
  use_prebuilt_binary(jsoncpp)
  if (WINDOWS)
    set(JSONCPP_LIBRARIES 
#      debug json_vc100_libmt
      optimized json_vc100_libmt)
  elseif (DARWIN)
    set(JSONCPP_LIBRARIES libjson_linux-gcc-4.0.1_libmt)
  elseif (LINUX)
    set(JSONCPP_LIBRARIES libjson_linux-gcc-4.3.2_libmt)
  endif (WINDOWS)
  set(JSONCPP_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/json)
endif (STANDALONE)
