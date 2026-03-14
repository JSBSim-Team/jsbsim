set(SOURCES Geodesic.cpp
            GeodesicLine.cpp
            Math.cpp)
set(HEADERS Config.h
            Constants.hpp
            Geodesic.hpp
            GeodesicLine.hpp
            Math.hpp)

add_library(GeographicLib OBJECT ${SOURCES})
install(FILES ${HEADERS} DESTINATION include/GeographicLib COMPONENT devel)
