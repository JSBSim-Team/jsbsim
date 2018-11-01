set(SOURCES sg_path.cxx strutils.cxx)

set(HEADERS stdint.hxx sg_path.hxx strutils.hxx)

add_library(Misc OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/misc COMPONENT devel)
