SET(CMAKE_CXX_STANDARD 11)

SET(LIBAEROMATIC3 Aeromatic++)

SET(AEROMATIC_HDRS
     types.h
     Aircraft.h
	   config.h
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
TARGET_INCLUDE_DIRECTORIES(${LIBAEROMATIC3} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

IF(WIN32)
   SET(EXTRA_LIBS "shlwapi.lib")
   IF(MSVC)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
   ENDIF(MSVC)
ENDIF(WIN32)

ADD_EXECUTABLE( aeromatic aeromatic.cpp )
TARGET_LINK_LIBRARIES( aeromatic ${LIBAEROMATIC3} ${EXTRA_LIBS})

INSTALL(TARGETS aeromatic RUNTIME DESTINATION bin COMPONENT runtime)
INSTALL(TARGETS ${LIBAEROMATIC3} ARCHIVE DESTINATION lib COMPONENT devel)
