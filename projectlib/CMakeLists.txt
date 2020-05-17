cmake_minimum_required( VERSION 3.5 )

project( project )

set ( src_lib
      json.cpp
      projectdaemon.cpp
      projecthttpclient.cpp
      projecthttpserver.cpp
      projectsipstring.cpp
      projectwebdocument.cpp
)

add_library( ${PROJECT_NAME} ${src_lib} )

target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECTLIB} )

target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} )

target_link_libraries( ${PROJECT_NAME} pthread )