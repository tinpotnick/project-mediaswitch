cmake_minimum_required( VERSION 3.5 )

project( mediaswitch )

set( CMAKE_CXX_FLAGS " -O2 -std=c++17 " )

include_directories( ${CMAKE_STAGING_PREFIX}/usr/include )
link_directories( ${CMAKE_STAGING_PREFIX}/usr/lib )

# ===================================================================================
# set(Boost_USE_STATIC_LIBS         OFF) # only find static libs
# set(Boost_USE_MULTITHREADED       ON)
# set(Boost_USE_STATIC_RUNTIME      OFF)

# find_package(Boost 1.69.0 REQUIRED COMPONENTS atomic coroutine chrono thread)
find_package(Boost 1.72.0 REQUIRED COMPONENTS chrono thread system random)
if(Boost_FOUND)
  include_directories( ${Boost_INCLUDE_DIRS} )
endif()
# ===================================================================================

set( PROJECTLIB ${CMAKE_SOURCE_DIR}/projectlib )

add_subdirectory( projectlib )
add_subdirectory( sip )
add_subdirectory( rtp )