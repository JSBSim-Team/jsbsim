set(JSBSIM_SIMGEAR_PROPS_SRC props.cxx)

set(JSBSIM_SIMGEAR_PROPS_HDR props.hxx)

add_library(Properties ${JSBSIM_SIMGEAR_PROPS_HDR} ${JSBSIM_SIMGEAR_PROPS_SRC})

install(TARGETS Properties LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib)

