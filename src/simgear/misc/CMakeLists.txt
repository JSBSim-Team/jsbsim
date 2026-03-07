set(SOURCES sg_path.cxx strutils.cxx)

set(HEADERS stdint.hxx sg_path.hxx strutils.hxx)

add_library(Misc OBJECT ${SOURCES})

if(WIN32)
    target_compile_definitions(Misc PRIVATE NOMINMAX)
endif()

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/misc
                         COMPONENT devel)
