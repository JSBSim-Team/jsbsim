set(SOURCES props.cxx)

set(HEADERS props.hxx)

add_library(Properties OBJECT ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/props
                         COMPONENT devel)
