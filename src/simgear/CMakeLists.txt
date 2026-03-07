add_subdirectory(structure)
add_subdirectory(props)
add_subdirectory(magvar)
add_subdirectory(misc)
add_subdirectory(xml)
add_subdirectory(io/iostreams)

set(HEADERS compiler.h)

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear COMPONENT devel)
