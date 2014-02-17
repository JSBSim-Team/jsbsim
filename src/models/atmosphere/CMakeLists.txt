set(SOURCES FGMSIS.cpp
            FGMSISData.cpp
            FGMars.cpp
            FGStandardAtmosphere.cpp
            FGWinds.cpp)

set(HEADERS FGMSIS.h
            FGMars.h
            FGStandardAtmosphere.h
            FGWinds.h)

add_full_path_name(ATMOSPHERE_SRC "${SOURCES}")
add_full_path_name(ATMOSPHERE_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/atmosphere)

