set(SOURCES coremag.cxx)

set(HEADERS coremag.hxx)

add_full_path_name(MAGVAR_SRC "${SOURCES}")
add_full_path_name(MAGVAR_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/magvar)

