set(HEADERS SGReferenced.hxx
            SGSharedPtr.hxx)

add_library(Structure OBJECT ${HEADERS})
set_target_properties(Structure PROPERTIES
                                TARGET_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                                LINKER_LANGUAGE CXX)

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/structure
                         COMPONENT devel)
