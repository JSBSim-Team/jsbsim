add_subdirectory(structure)
add_subdirectory(props)
add_subdirectory(magvar)
add_subdirectory(misc)
add_subdirectory(xml)
add_subdirectory(io/iostreams)

set(JSBSIM_SIMGEAR_HDR compiler.h)

propagate_source_files(SIMGEAR MAGVAR)
propagate_source_files(SIMGEAR MISC)
propagate_source_files(SIMGEAR PROPS)
propagate_source_files(SIMGEAR XML)
propagate_source_files(SIMGEAR IOSTREAMS)

install(FILES ${JSBSIM_SIMGEAR_HDR} DESTINATION include/JSBSim/simgear COMPONENT devel)
