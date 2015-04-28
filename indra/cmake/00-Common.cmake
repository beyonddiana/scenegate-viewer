# -*- cmake -*-
#
# Compilation options shared by all Second Life components.

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(CheckCCompilerFlag)
include(Variables)

# Portable compilation flags.
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -DLL_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE
    "-DLL_RELEASE=1 -DLL_RELEASE_FOR_DOWNLOAD=1 -DNDEBUG") 
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
    "-DLL_RELEASE=1 -DNDEBUG -DLL_RELEASE_WITH_DEBUG_INFO=1")

# Configure crash reporting
set(RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in release builds")
set(NON_RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in developer builds")

if(RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLL_SEND_CRASH_REPORTS=1")
endif()

if(NON_RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLL_SEND_CRASH_REPORTS=1")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLL_SEND_CRASH_REPORTS=1")
endif()  

# Don't bother with a MinSizeRel build.
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release;Debug" CACHE STRING
    "Supported build types." FORCE)


# Platform-specific compilation flags.

if (WINDOWS)
  option(RELEASE_FULL_OPT "Enable Whole Program Optimization and related folding and binary reduction routines" OFF)
  # Don't build DLLs.
  set(BUILD_SHARED_LIBS OFF)

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /MDd /MP -D_SCL_SECURE_NO_WARNINGS=1"
      CACHE STRING "C++ compiler debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od /Zi /MD /MP /Ob0 -D_SECURE_STL=0"
      CACHE STRING "C++ compiler release-with-debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /Zi /Zo /MD /MP /Ob2 -D_SECURE_STL=0 -D_HAS_ITERATOR_DEBUGGING=0"
      CACHE STRING "C++ compiler release options" FORCE)

  if (WORD_SIZE EQUAL 32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  endif (WORD_SIZE EQUAL 32)

  if (RELEASE_FULL_OPT AND NOT INCREMENTAL_LINK)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:REF /OPT:ICF /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /OPT:ICF /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /LTCG")
  elseif (INCREMENTAL_LINK AND NOT RELEASE_FULL_OPT)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL /VERBOSE:INCR")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL /VERBOSE:INCR")
  else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:REF /INCREMENTAL:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /INCREMENTAL:NO")
  endif (RELEASE_FULL_OPT AND NOT INCREMENTAL_LINK)

  set(CMAKE_CXX_STANDARD_LIBRARIES "")
  set(CMAKE_C_STANDARD_LIBRARIES "")

  add_definitions(
      /DLL_WINDOWS=1
      /DNOMINMAX
      /DUNICODE
      /D_UNICODE 
      /GS
      /TP
      /W3
      /c
      /Zc:wchar_t
      /Zc:forScope
      /nologo
      /Oy-
      /fp:fast
      )

  if (RELEASE_FULL_OPT AND NOT INCREMENTAL_LINK)
    add_definitions(
        /GL
        /Gy
        /Gw
        )
  endif (RELEASE_FULL_OPT AND NOT INCREMENTAL_LINK)

  if (USE_AVX)
    add_definitions(/arch:AVX)
  elseif (USE_AVX2)
    add_definitions(/arch:AVX2)
  elseif (WORD_SIZE EQUAL 32)
    add_definitions(/arch:SSE2)
  endif (USE_AVX)

  if (NOT VS_DISABLE_FATAL_WARNINGS)
    add_definitions(/WX)
  endif (NOT VS_DISABLE_FATAL_WARNINGS)

  # configure win32 API for windows vista+ compatibility
  set(WINVER "0x0600" CACHE STRING "Win32 API Target version (see http://msdn.microsoft.com/en-us/library/aa383745%28v=VS.85%29.aspx)")
  add_definitions("/DWINVER=${WINVER}" "/D_WIN32_WINNT=${WINVER}")
endif (WINDOWS)


if (LINUX)
  set(CMAKE_SKIP_RPATH TRUE)
  add_definitions(
      -DLL_LINUX=1
      -DAPPID=secondlife
      -DLL_IGNORE_SIGCHLD
      -D_REENTRANT
      -fvisibility=hidden
      -fexceptions
      -fno-math-errno
      -fno-strict-aliasing
      -fsigned-char
      -std=gnu++11
      -g
      -pthread
      )

  CHECK_C_COMPILER_FLAG(-fstack-protector-strong HAS_STRONG_STACK_PROTECTOR)
  CHECK_C_COMPILER_FLAG(-fstack-protector HAS_STACK_PROTECTOR)

  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(HAS_STRONG_STACK_PROTECTOR)
      add_definitions(-fstack-protector-strong)
    elseif(HAS_STACK_PROTECTOR)
      add_definitions(-fstack-protector)
    endif(HAS_STRONG_STACK_PROTECTOR)
    add_definitions(-D_FORTIFY_SOURCE=2)
  endif (${CMAKE_BUILD_TYPE} STREQUAL "Release")

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    find_program(GXX g++)
    mark_as_advanced(GXX)

    if (GXX)
      execute_process(
          COMMAND ${GXX} --version
          COMMAND sed "s/^[gc+ ]*//"
          COMMAND head -1
          OUTPUT_VARIABLE GXX_VERSION
          OUTPUT_STRIP_TRAILING_WHITESPACE
          )
    else (GXX)
      set(GXX_VERSION x)
    endif (GXX)

    # The quoting hack here is necessary in case we're using distcc or
    # ccache as our compiler.  CMake doesn't pass the command line
    # through the shell by default, so we end up trying to run "distcc"
    # " g++" - notice the leading space.  Ugh.

    execute_process(
        COMMAND sh -c "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} --version"
        COMMAND sed "s/^[gc+ ]*//"
        COMMAND head -1
        OUTPUT_VARIABLE CXX_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Let's actually get a numerical version of gxx's version
    STRING(REGEX REPLACE ".* ([0-9])\\.([0-9])\\.([0-9]).*" "\\1\\2\\3" CXX_VERSION_NUMBER ${CXX_VERSION})

    if (WORD_SIZE EQUAL 32)
      add_definitions(
        -msse2
        -mfpmath=sse
        -march=pentium4)
    endif (WORD_SIZE EQUAL 32)

    if (NOT USESYSTEMLIBS)
      # linking can be very memory-hungry, especially the final viewer link
      set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")
    endif (NOT USESYSTEMLIBS)

    if (${CXX_VERSION_NUMBER} GREATER 479)
      set(CMAKE_CXX_FLAGS_DEBUG "-Og -fno-inline ${CMAKE_CXX_FLAGS_DEBUG}")
    else (${CXX_VERSION_NUMBER} GREATER 479)
      set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline ${CMAKE_CXX_FLAGS_DEBUG}")
    endif (${CXX_VERSION_NUMBER} GREATER 479)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_RELEASE}")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (WORD_SIZE EQUAL 32)
      add_definitions(-msse2 -march=pentium4)
    endif (WORD_SIZE EQUAL 32)

    if (NOT USESYSTEMLIBS)
      # linking can be very memory-hungry, especially the final viewer link
      set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")
    endif (NOT USESYSTEMLIBS)

    set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    if (NOT USESYSTEMLIBS)
      add_definitions(-march=pentium4)
    endif (NOT USESYSTEMLIBS)

    if (NOT USESYSTEMLIBS)
      # linking can be very memory-hungry, especially the final viewer link
      set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")
    endif (NOT USESYSTEMLIBS)

    set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_RELEASE}")
  endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
endif (LINUX)


if (DARWIN)
  add_definitions(-DLL_DARWIN=1)
  set(CMAKE_CXX_LINK_FLAGS "-Wl,-no_compact_unwind -Wl,-headerpad_max_install_names,-search_paths_first")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
  set(DARWIN_extra_cstar_flags "-g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DARWIN_extra_cstar_flags}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${DARWIN_extra_cstar_flags}")
  # NOTE: it's critical that the optimization flag is put in front.
  # NOTE: it's critical to have both CXX_FLAGS and C_FLAGS covered.
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O0 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O0 ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
  set(CMAKE_C_FLAGS_RELEASE "-O3 ${CMAKE_C_FLAGS_RELEASE}")
  set(ENABLE_SIGNING TRUE)
  set(SIGNING_IDENTITY "Developer ID Application: Linden Research, Inc.")
endif (DARWIN)


if (LINUX OR DARWIN)
  if (NOT UNIX_DISABLE_FATAL_WARNINGS)
    set(UNIX_WARNINGS "-Werror")
  endif (NOT UNIX_DISABLE_FATAL_WARNINGS)

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(UNIX_WARNINGS "-Wall -Wno-sign-compare -Wno-trigraphs ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS} -Wno-reorder")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(UNIX_WARNINGS "-Wall -Wno-sign-compare -Wno-trigraphs ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS} -Wno-reorder")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(UNIX_WARNINGS "-w2 -diag-disable remark -wd68 -wd597 -wd780 -wd858 ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS}")
  endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

  set(CMAKE_C_FLAGS "${UNIX_WARNINGS} ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${UNIX_CXX_WARNINGS} ${CMAKE_CXX_FLAGS}")

  if (WORD_SIZE EQUAL 32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  elseif (WORD_SIZE EQUAL 64)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  endif (WORD_SIZE EQUAL 32)
endif (LINUX OR DARWIN)


if (USESYSTEMLIBS)
  add_definitions(-DLL_USESYSTEMLIBS=1)

  if (LINUX AND ${ARCH} STREQUAL "i686")
    add_definitions(-march=pentiumpro)
  endif (LINUX AND ${ARCH} STREQUAL "i686")

else (USESYSTEMLIBS)
  set(${ARCH}_linux_INCLUDES
      ELFIO
      atk-1.0
      gdk-pixbuf-2.0
      glib-2.0
      gstreamer-0.10
      gtk-2.0
      pango-1.0
      )
endif (USESYSTEMLIBS)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
