set(SOURCES props.cxx)

set(HEADERS props.hxx)

add_library(Properties OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Properties PROPERTIES TARGET_DIRECTORY
                                 ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/props
                         COMPONENT devel)
