set(SOURCES props.cxx)

set(HEADERS props.hxx)

add_full_path_name(PROPS_SRC "${SOURCES}")
add_full_path_name(PROPS_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/props)
