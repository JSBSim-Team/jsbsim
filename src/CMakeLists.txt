include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Define some commons compile flags.                                           #
################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# The object libraries are compiled into libJSBSim and are not part of its
# installed interface: without BUILD_INTERFACE, install(EXPORT) of a static
# libJSBSim would require every one of them in the export set.
set(OBJECT_LIBS $<BUILD_INTERFACE:Atmosphere>
                $<BUILD_INTERFACE:FlightControl>
                $<BUILD_INTERFACE:GeographicLib>
                $<BUILD_INTERFACE:Init>
                $<BUILD_INTERFACE:InputOutput>
                $<BUILD_INTERFACE:IOStreams>
                $<BUILD_INTERFACE:Magvar>
                $<BUILD_INTERFACE:Math>
                $<BUILD_INTERFACE:Misc>
                $<BUILD_INTERFACE:Models>
                $<BUILD_INTERFACE:Properties>
                $<BUILD_INTERFACE:Propulsion>
                $<BUILD_INTERFACE:Xml>)

add_library(libJSBSim ${SOURCES})
target_link_libraries(libJSBSim PRIVATE ${OBJECT_LIBS})

if(SYSTEM_EXPAT AND NOT BUILD_SHARED_LIBS)
  # A static libJSBSim leaves libExpat to its consumers (see JSBSimConfig.cmake).
  target_link_libraries(libJSBSim INTERFACE $<INSTALL_INTERFACE:EXPAT::EXPAT>)
endif()

target_compile_definitions(libJSBSim PUBLIC
                           JSBSIM_VERSION="${PROJECT_VERSION}${VERSION_MESSAGE}")
target_include_directories(libJSBSim PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include/JSBSim>)

if(WIN32)
  target_compile_definitions(libJSBSim PUBLIC _USE_MATH_DEFINES)
  target_link_libraries(libJSBSim PUBLIC wsock32 ws2_32)

  if(BUILD_SHARED_LIBS)
    set(JSBSIM_LIBRARY_TYPE JSBSIM_EXPORT)
    # The flag JSBSIM_EXPORT must be declared PRIVATE to avoid being propagated
    # to other targets such as the executable, the Python module and the unit
    # tests. Otherwise the linking of these targets will fail.
    target_compile_definitions(libJSBSim PRIVATE JSBSIM_EXPORT)
  else()
    set(JSBSIM_LIBRARY_TYPE JSBSIM_STATIC_LINK)
    target_compile_definitions(libJSBSim PUBLIC JSBSIM_STATIC_LINK)
  endif(BUILD_SHARED_LIBS)

  foreach(OBJECT ${OBJECT_LIBS})
    string(REGEX REPLACE "^\\$<BUILD_INTERFACE:(.+)>$" "\\1" OBJECT "${OBJECT}")
    target_compile_definitions(${OBJECT} PRIVATE ${JSBSIM_LIBRARY_TYPE})
  endforeach(OBJECT)
elseif(UNIX)
  target_link_libraries(libJSBSim PUBLIC m)
endif(WIN32)

set_target_properties(libJSBSim PROPERTIES
                                OUTPUT_NAME JSBSim
                                EXPORT_NAME JSBSim
                                VERSION ${LIBRARY_VERSION})

if(BUILD_SHARED_LIBS)
  set_target_properties(libJSBSim PROPERTIES
                            SOVERSION ${LIBRARY_SOVERSION})
  install(TARGETS libJSBSim EXPORT JSBSimTargets
                            LIBRARY DESTINATION lib
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
  install(TARGETS libJSBSim EXPORT JSBSimTargets
                            ARCHIVE DESTINATION lib
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

################################################################################
# CMake package config: find_package(JSBSim CONFIG) -> JSBSim::JSBSim          #
################################################################################

if(NOT SKBUILD)
  include(CMakePackageConfigHelpers)

  set(JSBSIM_CMAKE_INSTALL_DIR lib/cmake/JSBSim)

  install(EXPORT JSBSimTargets
          NAMESPACE JSBSim::
          DESTINATION ${JSBSIM_CMAKE_INSTALL_DIR}
          COMPONENT devel)
  configure_package_config_file(${PROJECT_SOURCE_DIR}/JSBSimConfig.cmake.in
                                ${CMAKE_CURRENT_BINARY_DIR}/JSBSimConfig.cmake
                                INSTALL_DESTINATION ${JSBSIM_CMAKE_INSTALL_DIR})
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/JSBSimConfigVersion.cmake
                                  VERSION ${PROJECT_VERSION}
                                  COMPATIBILITY SameMinorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/JSBSimConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/JSBSimConfigVersion.cmake
          DESTINATION ${JSBSIM_CMAKE_INSTALL_DIR}
          COMPONENT devel)
endif(NOT SKBUILD)
