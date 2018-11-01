set(SOURCES sgstream.cxx)

set(HEADERS sgstream.hxx)

add_library(IOStreams OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/io/iostreams COMPONENT devel)
