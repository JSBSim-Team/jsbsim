include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(initialization)
add_subdirectory(input_output)
add_subdirectory(math)
add_subdirectory(models)
add_subdirectory(simgear)

set(HEADERS FGFDMExec.h
            FGJSBBase.h)
set(SOURCES FGFDMExec.cpp
            FGJSBBase.cpp)

add_executable(JSBSim JSBSim.cpp ${HEADERS} ${SOURCES})
target_link_libraries(JSBSim
                      Init Atmosphere Models FlightControl Propulsion
                      Math InputOutput Properties Expat coremag)

install(TARGETS JSBSim RUNTIME DESTINATION bin)
install(FILES ${HEADERS} DESTINATION include/JSBSim)

