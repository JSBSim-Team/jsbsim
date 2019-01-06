set(SOURCES sgstream.cxx)

set(HEADERS sgstream.hxx)

add_library(IOStreams OBJECT ${HEADERS} ${SOURCES})
set_target_properties(IOStreams PROPERTIES TARGET_DIRECTORY
                                ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/io/iostreams
                         COMPONENT devel)
