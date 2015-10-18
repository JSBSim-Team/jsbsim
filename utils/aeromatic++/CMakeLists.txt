
ENABLE_LANGUAGE(CXX)

CONFIGURE_FILE(
     "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
     "${CMAKE_CURRENT_BINARY_DIR}/config.h"
   )

INCLUDE_DIRECTORIES(
     "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_BINARY_DIR}"
   )

SET(LIBAEROMATIC3 Aeromatic3)

SET(AEROMATIC_HDRS 
     types.h
     Aircraft.h
     Systems/Controls.h
     Systems/Propulsion.h
     Systems/Systems.h
     Systems/Thruster.h
   )

SET(AEROMATIC_OBJS
     types.cpp
     Aircraft.cpp
     AeroLightGA.cpp
     AeroFighterJet.cpp
     AeroHighPerformance.cpp
     AeroJetTransport.cpp
     AeroPropTransport.cpp
     Systems/Controls.cpp
     Systems/Flaps.cpp
     Systems/LandingGear.cpp
     Systems/Propulsion.cpp
     Systems/Speedbrake.cpp
     Systems/Spoilers.cpp
     Systems/Thruster.cpp
   )

ADD_LIBRARY( ${LIBAEROMATIC3} STATIC ${AEROMATIC_OBJS} )

ADD_EXECUTABLE( aeromatic aeromatic.cpp )
TARGET_LINK_LIBRARIES( aeromatic ${LIBAEROMATIC3} )
