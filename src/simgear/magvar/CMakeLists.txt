set(SOURCES coremag.cxx)

set(HEADERS coremag.hxx)

add_library(Magvar OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/magvar COMPONENT devel)
