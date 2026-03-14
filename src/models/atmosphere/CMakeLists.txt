set(SOURCES FGMSIS.cpp
            FGStandardAtmosphere.cpp
            FGWinds.cpp
            MSIS/nrlmsise-00.c
            MSIS/nrlmsise-00_data.c)

set(HEADERS FGMSIS.h
            FGStandardAtmosphere.h
            FGWinds.h
            MSIS/nrlmsise-00.h)

add_library(Atmosphere OBJECT ${SOURCES})

if(CXXTEST_FOUND)
    add_coverage(Atmosphere)
endif(CXXTEST_FOUND)

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/atmosphere
                         COMPONENT devel)
