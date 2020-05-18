cmake_minimum_required( VERSION 3.5 )

project( sip )

set ( src_sip
      projectsipconfig.cpp
      projectsipdialog.cpp
      projectsipdialoginvite.cpp
      projectsipdialogrefer.cpp
      projectsipdirectory.cpp
      projectsipdnssrvresolver.cpp
      projectsipendpoint.cpp
      projectsipeventstate.cpp
      projectsipmain.cpp
      projectsippacket.cpp
      projectsipregistrar.cpp
      projectsipsdp.cpp
      projectsipserver.cpp
      projectsipsm.cpp
      tests.cpp
)

add_executable( ${PROJECT_NAME} ${src_sip} )

target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECTLIB} )

target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} project )
target_link_libraries( ${PROJECT_NAME} resolv )
target_link_libraries( ${PROJECT_NAME} crypto )

target_link_libraries( ${PROJECT_NAME} pthread )