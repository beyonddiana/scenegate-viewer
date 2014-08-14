# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (LINUX OR NOT FMODEX OR NOT FMODSTUDIO)
  set(OPENAL ON CACHE BOOL "Enable OpenAL")
else (LINUX OR NOT FMODEX OR NOT FMODSTUDIO)
  set(OPENAL OFF CACHE BOOL "Enable OpenAL")
endif (LINUX OR NOT FMODEX OR NOT FMODSTUDIO)

if (OPENAL)
  set(OPENAL_LIB_INCLUDE_DIRS "${LIBS_PREBUILT_DIR}/include/AL")
  if (USESYSTEMLIBS)
    include(FindPkgConfig)
    include(FindOpenAL)
    pkg_check_modules(OPENAL_LIB REQUIRED openal)
    pkg_check_modules(FREEALUT_LIB REQUIRED freealut)
  else (USESYSTEMLIBS)
    use_prebuilt_binary(openal)
  endif (USESYSTEMLIBS)
  if(WINDOWS)
    set(OPENAL_LIBRARIES 
      OpenAL32
      alut
    )
  else()
    set(OPENAL_LIBRARIES 
      openal
      alut
    )
  endif()
endif (OPENAL)

if (OPENAL)
  message(STATUS "Building with OpenAL audio support")
endif (OPENAL)
