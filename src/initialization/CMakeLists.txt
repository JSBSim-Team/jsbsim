set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp
            FGSimplexTrim.cpp
            FGTrimmer.cpp
            FGLinearization.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h
            FGSimplexTrim.h
            FGTrimmer.h
            FGLinearization.h)

add_full_path_name(INITIALISATION_SRC "${SOURCES}")
add_full_path_name(INITIALISATION_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization)

