set(SOURCES sg_path.cxx strutils.cxx)

set(HEADERS stdint.hxx sg_path.hxx strutils.hxx)

add_library(Misc OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Misc PROPERTIES TARGET_DIRECTORY
                           ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/misc
                         COMPONENT devel)
