set(SOURCES coremag.cxx)

set(HEADERS coremag.hxx)

add_library(Magvar OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Magvar PROPERTIES TARGET_DIRECTORY
                             ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/magvar
                         COMPONENT devel)
