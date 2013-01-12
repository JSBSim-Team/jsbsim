set(JSBSIM_SIMGEAR_MAGVAR_SRC coremag.cxx)

set(JSBSIM_SIMGEAR_MAGVAR_HDR coremag.hxx)

add_library(coremag ${JSBSIM_SIMGEAR_MAGVAR_HDR} ${JSBSIM_SIMGEAR_MAGVAR_SRC})

install(TARGETS coremag LIBRARY DESTINATION lib
                        ARCHIVE DESTINATION lib)

