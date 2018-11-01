add_subdirectory(structure)
add_subdirectory(props)
add_subdirectory(magvar)
add_subdirectory(misc)
add_subdirectory(xml)
add_subdirectory(io/iostreams)

set(JSBSIM_SIMGEAR_HDR compiler.h)

install(FILES ${JSBSIM_SIMGEAR_HDR} DESTINATION include/JSBSim/simgear COMPONENT devel)
