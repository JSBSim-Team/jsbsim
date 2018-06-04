set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h)

add_full_path_name(INITIALISATION_SRC "${SOURCES}")
add_full_path_name(INITIALISATION_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization COMPONENT devel)
