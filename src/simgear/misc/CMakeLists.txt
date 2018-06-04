set(SOURCES sg_path.cxx strutils.cxx)

set(HEADERS stdint.hxx sg_path.hxx strutils.hxx)

add_full_path_name(MISC_SRC "${SOURCES}")
add_full_path_name(MISC_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/misc COMPONENT devel)
