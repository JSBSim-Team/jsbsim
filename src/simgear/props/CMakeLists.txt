set(SOURCES props.cxx propertyObject.cxx)

set(HEADERS props.hxx propertyObject.hxx)

add_library(Properties OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/props COMPONENT devel)
