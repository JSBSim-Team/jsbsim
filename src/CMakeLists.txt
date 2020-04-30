include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Define some commons compile flags.                                           #
################################################################################

if(MSVC)
  # CMake requires to specify flags with -D even for Windows. The transformation
  # to /D is made by CMake.
  add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
endif()

if(MINGW)
  # Need to force _WIN32_WINNT to a quite recent value for FGfdmSocket to compile due
  # to the usage of the flags AI_ADDRCONFIG and AI_V4MAPPED
  # See https://docs.microsoft.com/fr-fr/windows/win32/winprog/using-the-windows-headers#setting-winver-or-_win32_winnt
  add_definitions(-D_WIN32_WINNT=0x600) # Windows Vista/Windows Server 2008
endif()

if (CMAKE_VERSION VERSION_LESS 3.1)
  if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

################################################################################
# Init the list of libraries that JSBSim links with                            #
################################################################################

if(WIN32 AND (MSVC OR MINGW))
  # not applicable to cygwin
  if(MSVC)
    set(JSBSIM_LINK_LIBRARIES "wsock32.lib" "ws2_32.lib")
  else()
    set(JSBSIM_LINK_LIBRARIES "-lwsock32 -lws2_32")
  endif(MSVC)
elseif(UNIX)
  # not applicable to cygwin
  set(JSBSIM_LINK_LIBRARIES "m")
else()
  set(JSBSIM_LINK_LIBRARIES)
endif()

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

add_definitions(-DJSBSIM_VERSION="${PROJECT_VERSION}${VERSION_MESSAGE}")

set(STATIC_LIBNAME_SUFFIX)
if(MSVC AND BUILD_SHARED_LIBS)
  set(STATIC_LIBNAME_SUFFIX "static")
endif()

add_subdirectory(initialization)
add_subdirectory(input_output)
add_subdirectory(math)
add_subdirectory(models)
add_subdirectory(simgear)

if(EXPAT_FOUND)
  include_directories(${EXPAT_INCLUDE_DIRS})
  set(JSBSIM_LINK_LIBRARIES ${EXPAT_LIBRARIES} ${JSBSIM_LINK_LIBRARIES})
endif()

set(HEADERS FGFDMExec.h
            FGJSBBase.h)
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
  )

set_target_properties (libJSBSim PROPERTIES
                                 OUTPUT_NAME JSBSim${STATIC_LIBNAME_SUFFIX}
                                 VERSION ${LIBRARY_VERSION}
                                 TARGET_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(libJSBSim ${JSBSIM_LINK_LIBRARIES})

if(BUILD_SHARED_LIBS)
  set_target_properties (libJSBSim PROPERTIES
                                   SOVERSION ${LIBRARY_SOVERSION}
                                   FRAMEWORK ON)
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

install(TARGETS JSBSim RUNTIME DESTINATION bin COMPONENT runtime)
install(FILES ${HEADERS} DESTINATION include/JSBSim COMPONENT devel)
