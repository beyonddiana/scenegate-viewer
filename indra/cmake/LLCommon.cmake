# -*- cmake -*-

include(APR)
include(Boost)
include(EXPAT)
include(ZLIB)
include(GooglePerfTools)

if (LINUX)
    # In order to support using ld.gold on linux, we need to explicitely
    # specify all libraries that llcommon uses.
    # llcommon uses `clock_gettime' which is provided by librt on linux.
    set(LLCOMMON_LIBRARIES llcommon 
        ${BOOST_COROUTINE_LIBRARY} 
        ${BOOST_CONTEXT_LIBRARY} 
        ${BOOST_THREAD_LIBRARY} 
        ${BOOST_SYSTEM_LIBRARY} 
        rt
        )
else (LINUX)
    set(LLCOMMON_LIBRARIES llcommon
        ${BOOST_COROUTINE_LIBRARY} 
        ${BOOST_CONTEXT_LIBRARY} 
        ${BOOST_THREAD_LIBRARY} 
        ${BOOST_SYSTEM_LIBRARY} )
endif (LINUX)

add_definitions(${TCMALLOC_FLAG})

set(LLCOMMON_LINK_SHARED OFF CACHE BOOL "Build the llcommon target as a static library.")
if(LLCOMMON_LINK_SHARED)
  add_definitions(-DLL_COMMON_LINK_SHARED=1)
endif(LLCOMMON_LINK_SHARED)
