set(SOURCES coremag.cxx)

set(HEADERS coremag.hxx)

add_library(Magvar OBJECT ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/magvar
                         COMPONENT devel)
