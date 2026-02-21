include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Define some commons compile flags.                                           #
################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################################################################################
# Init the list of libraries that JSBSim links with                            #
################################################################################

# MSVC and MINGW linked libraries
set(WINDOWS_LINK_LIBRARIES wsock32 ws2_32)
# Unix linked libraries
set(UNIX_LINK_LIBRARIES m)

################################################################################
# Build and install libraries                                                  #
################################################################################

set(VERSION_MESSAGE)
if(DEFINED ENV{TRAVIS})
  set(VERSION_MESSAGE " [Travis build $ENV{TRAVIS_BUILD_NUMBER}/commit $ENV{TRAVIS_COMMIT}]")
elseif(DEFINED ENV{APPVEYOR})
  set(VERSION_MESSAGE " [AppVeyor build $ENV{APPVEYOR_BUILD_NUMBER}/commit $ENV{APPVEYOR_REPO_COMMIT}]")
elseif(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(VERSION_MESSAGE " [GitHub build $ENV{GITHUB_RUN_NUMBER}/commit $ENV{GITHUB_SHA}]")
endif()

add_subdirectory(initialization)
add_subdirectory(input_output)
add_subdirectory(math)
add_subdirectory(models)
add_subdirectory(simgear)
add_subdirectory(GeographicLib)

set(HEADERS FGFDMExec.h
            FGJSBBase.h
            JSBSim_API.h)
set(SOURCES FGFDMExec.cpp
            FGJSBBase.cpp)

add_library(libJSBSim ${HEADERS} ${SOURCES}
  $<TARGET_OBJECTS:Init>
  $<TARGET_OBJECTS:Atmosphere>
  $<TARGET_OBJECTS:FlightControl>
  $<TARGET_OBJECTS:Propulsion>
  $<TARGET_OBJECTS:Models>
  $<TARGET_OBJECTS:Math>
  $<TARGET_OBJECTS:InputOutput>
  $<TARGET_OBJECTS:Properties>
  $<TARGET_OBJECTS:Xml>
  $<TARGET_OBJECTS:Magvar>
  $<TARGET_OBJECTS:Misc>
  $<TARGET_OBJECTS:IOStreams>
  $<TARGET_OBJECTS:Structure>
  $<TARGET_OBJECTS:Simgear>
  $<TARGET_OBJECTS:GeographicLib>
  )
target_compile_definitions(libJSBSim PUBLIC
                           JSBSIM_VERSION="${PROJECT_VERSION}${VERSION_MESSAGE}")
target_include_directories(libJSBSim PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if(EXPAT_FOUND)
  target_include_directories(libJSBSim PRIVATE ${EXPAT_INCLUDE_DIRS})
  if (PKG_CONFIG_FOUND)
    target_link_libraries(libJSBSim ${PC_EXPAT_LIBRARIES})
    set(ALL_LINK_LIBRARIES ${PC_EXPAT_LIBRARIES})
  else()
    target_link_libraries(libJSBSim ${EXPAT_LIBRARIES})
    set(ALL_LINK_LIBRARIES ${EXPAT_LIBRARIES})
  endif()
endif(EXPAT_FOUND)

if(WIN32)
  target_compile_definitions(libJSBSim PUBLIC _USE_MATH_DEFINES)
  if(BUILD_SHARED_LIBS)
    set(JSBSIM_LIBRARY_TYPE JSBSIM_EXPORT)
    # The flag JSBSIM_EXPORT must be declared PRIVATE to avoid propagating it
    # to other targets such as the executable, the Python module and the unit
    # tests. Otherwise the linking of these targets will fail.
    target_compile_definitions(libJSBSim PRIVATE JSBSIM_EXPORT)
  else()
    set(JSBSIM_LIBRARY_TYPE JSBSIM_STATIC_LINK)
    target_compile_definitions(libJSBSim PUBLIC JSBSIM_STATIC_LINK)
  endif(BUILD_SHARED_LIBS)

  get_target_property(libJSBSim_SOURCE_FILES libJSBSim SOURCES)
  foreach(OBJECT ${libJSBSim_SOURCE_FILES})
    if (${OBJECT} MATCHES "TARGET_OBJECTS:([^ >]+)")
      target_compile_definitions(${CMAKE_MATCH_1} PRIVATE ${JSBSIM_LIBRARY_TYPE})
    endif()
  endforeach(OBJECT)
endif(WIN32)

set_target_properties(libJSBSim PROPERTIES
                                OUTPUT_NAME JSBSim
                                VERSION ${LIBRARY_VERSION}
                                TARGET_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Manage link libraries
set(JSBSIM_LINK_LIBRARIES ${ALL_LINK_LIBRARIES} PARENT_SCOPE)
set(JSBSIM_WINDOWS_LINK_LIBRARIES ${WINDOWS_LINK_LIBRARIES} PARENT_SCOPE)
set(JSBSIM_UNIX_LINK_LIBRARIES ${UNIX_LINK_LIBRARIES} PARENT_SCOPE)

if(WIN32 AND (MSVC OR MINGW))
  foreach(LINK_LIBRARY ${WINDOWS_LINK_LIBRARIES})
    target_link_libraries(libJSBSim ${LINK_LIBRARY})
  endforeach()
elseif(UNIX)
  foreach(LINK_LIBRARY ${UNIX_LINK_LIBRARIES})
    target_link_libraries(libJSBSim ${LINK_LIBRARY})
  endforeach()
endif()

if(BUILD_SHARED_LIBS)
  set_target_properties(libJSBSim PROPERTIES
                            SOVERSION ${LIBRARY_SOVERSION})
  install(TARGETS libJSBSim LIBRARY DESTINATION lib
                            NAMELINK_SKIP
                            COMPONENT runtime
                            # For Mac
                            FRAMEWORK DESTINATION "/Library/Frameworks")
  install(TARGETS libJSBSim LIBRARY DESTINATION lib
                            NAMELINK_ONLY
                            COMPONENT devel
                            # For Mac
                            FRAMEWORK DESTINATION "/Library/Frameworks")
else()
  install(TARGETS libJSBSim ARCHIVE DESTINATION lib
                            COMPONENT devel
                            # For Mac
                            FRAMEWORK DESTINATION "/Library/Frameworks")
endif()

################################################################################
# Build and install command line executable                                    #
################################################################################

add_executable(JSBSim JSBSim.cpp)
target_link_libraries(JSBSim libJSBSim)

if(CXXTEST_FOUND)
  add_coverage(libJSBSim)
  # Add JSBSim to the coverage analysis to avoid linking errors
  add_coverage(JSBSim)
endif(CXXTEST_FOUND)

if(MSVC AND BUILD_SHARED_LIBS)
  # Under Windows, during the linking process, executables must have a
  # different name than the DLLs to which they will be linked otherwise MSVC
  # complains with an error LNK1149:
  # "LINK : fatal error LNK1149: output filename matches input filename"
  #
  # To workaround this limitation the target output name is set to
  # JSBSim-bin.exe which is copied to JSBSim.exe afterwards. That way the
  # executable name is the same whether it has been statically linked or
  # dynamically linked. And we still use the legacy name JSBSim.exe
  set_target_properties(JSBSim PROPERTIES OUTPUT_NAME JSBSim-bin)
  add_custom_command(TARGET JSBSim POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:JSBSim>
                      $<TARGET_FILE_DIR:JSBSim>/JSBSim.exe)
endif(MSVC AND BUILD_SHARED_LIBS)

install(TARGETS JSBSim RUNTIME DESTINATION bin COMPONENT runtime)
install(FILES ${HEADERS} DESTINATION include/JSBSim COMPONENT devel)
