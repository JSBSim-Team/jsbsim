set(SOURCES FGMSIS.cpp
            FGMars.cpp
            FGStandardAtmosphere.cpp
            FGWinds.cpp)

set(HEADERS FGMSIS.h
            FGMars.h
            FGStandardAtmosphere.h
            FGWinds.h
            MSIS/nrlmsise-00.h)

add_library(Atmosphere OBJECT ${SOURCES})
add_library(MSIS OBJECT MSIS/nrlmsise-00.c MSIS/nrlmsise-00_data.c)

if(LCOV_FOUND)
    add_coverage(Atmosphere)
endif()

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/atmosphere
                         COMPONENT devel)
