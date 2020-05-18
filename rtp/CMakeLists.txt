cmake_minimum_required( VERSION 3.5 )

project( rtp )

set ( src_rtp
      firfilter.cpp
      projectrtpchannel.cpp
      projectrtpcodecx.cpp
      projectrtpmain.cpp
      projectrtppacket.cpp
      projectrtpsoundfile.cpp
      projectrtptonegen.cpp
      projetcrtpsoundsoup.cpp
)

add_executable( ${PROJECT_NAME} ${src_rtp} )

target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECTLIB} )

target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} project )
target_link_libraries( ${PROJECT_NAME} spandsp )
target_link_libraries( ${PROJECT_NAME} ilbc )
target_link_libraries( ${PROJECT_NAME} rt )

target_link_libraries( ${PROJECT_NAME} pthread )