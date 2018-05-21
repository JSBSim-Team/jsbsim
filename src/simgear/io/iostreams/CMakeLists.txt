set(SOURCES sgstream.cxx)

set(HEADERS sgstream.hxx)

add_full_path_name(IOSTREAMS_SRC "${SOURCES}")
add_full_path_name(IOSTREAMS_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/io/iostreams COMPONENT devel)
