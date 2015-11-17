
ENABLE_LANGUAGE(CXX)

# read 'version' file into a variable (stripping any newlines or spaces)
FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/version versionFile)
STRING(STRIP ${versionFile} AEROMATIC_VERSION)
SET(VERSION AEROMATIC_VERSION)

# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
STRING(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CPACK_PACKAGE_VERSION ${AEROMATIC_VERSION})

SET(AEROMATIC_MAJOR_VERSION ${CMAKE_MATCH_1})
SET(AEROMATIC_MINOR_VERSION ${CMAKE_MATCH_2})
SET(AEROMATIC_MICRO_VERSION ${CMAKE_MATCH_3})

message("version: ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message("version: ${AEROMATIC_MAJOR_VERSION}.${AEROMATIC_MINOR_VERSION}.${AEROMATIC_MICRO_VERSION}")

CONFIGURE_FILE(
     "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
     "${CMAKE_CURRENT_BINARY_DIR}/config.h"
   )

INCLUDE_DIRECTORIES(
     "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_BINARY_DIR}"
   )

SET(LIBAEROMATIC3 Aeromatic++)

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
     Systems/Chute.cpp
     Systems/ArrestorHook.cpp
     Systems/Spoilers.cpp
     Systems/Thruster.cpp
     Systems/ThrustReverse.cpp
     Systems/Catapult.cpp
   )

ADD_LIBRARY( ${LIBAEROMATIC3} STATIC ${AEROMATIC_OBJS} )

IF(WIN32)
   SET(EXTRA_LIBS "shlwapi.lib")
   IF(MSVC)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
   ENDIF(MSVC)
ENDIF(WIN32)

ADD_EXECUTABLE( aeromatic aeromatic.cpp )
TARGET_LINK_LIBRARIES( aeromatic ${LIBAEROMATIC3} ${EXTRA_LIBS})
