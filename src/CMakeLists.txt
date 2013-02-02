include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Define some commons compile flags.                                           #
################################################################################

if(MSVC)
  add_definitions("/D _USE_MATH_DEFINES")
endif()

################################################################################
# Init the list of libraries that JSBSim links with                            #
################################################################################

if(WIN32 AND (MSVC OR MINGW))
  # not applicable to cygwin
  set(JSBSIM_LINK_LIBRARIES "ws2_32")
elseif(UNIX)
  # not applicable to cygwin
  set(JSBSIM_LINK_LIBRARIES "m")
else()
  set(JSBSIM_LINK_LIBRARIES)
endif()

################################################################################
# Build and install libraries                                                  #
################################################################################

set(STATIC_LIBNAME_SUFFIX)
if(MSVC AND BUILD_SHARED_LIBS)
    set(STATIC_LIBNAME_SUFFIX "static")
endif()

add_subdirectory(initialization)
add_subdirectory(input_output)
add_subdirectory(math)
add_subdirectory(models)
add_subdirectory(simgear)

list(APPEND JSBSIM_LINK_LIBRARIES Init Atmosphere Models FlightControl
                                  Propulsion Math InputOutput Properties Expat
                                  coremag)

set(HEADERS FGFDMExec.h
            FGJSBBase.h)
set(SOURCES FGFDMExec.cpp
            FGJSBBase.cpp)

add_library(libJSBSim ${HEADERS} ${SOURCES})
target_link_libraries(libJSBSim ${JSBSIM_LINK_LIBRARIES})
set_target_properties (libJSBSim PROPERTIES
                                 OUTPUT_NAME JSBSim${STATIC_LIBNAME_SUFFIX}
                                 VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (libJSBSim PROPERTIES
                                   SOVERSION ${LIBRARY_SOVERSION}
                                   FRAMEWORK ON)
endif()

install(TARGETS libJSBSim LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                          RUNTIME DESTINATION lib
                          # For Mac
                          FRAMEWORK DESTINATION "/Library/Frameworks")

################################################################################
# Build and install command line executable                                    #
################################################################################

add_executable(JSBSim JSBSim.cpp)
target_link_libraries(JSBSim libJSBSim)

install(TARGETS JSBSim RUNTIME DESTINATION bin)
install(FILES ${HEADERS} DESTINATION include/JSBSim)

