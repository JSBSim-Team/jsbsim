add_subdirectory(structure)
add_subdirectory(props)
add_subdirectory(magvar)
add_subdirectory(misc)
add_subdirectory(xml)
add_subdirectory(io/iostreams)

set(HEADERS compiler.h)

add_library(Simgear OBJECT ${HEADERS})
set_target_properties(Simgear PROPERTIES
                              TARGET_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                              LINKER_LANGUAGE C)


install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear COMPONENT devel)
