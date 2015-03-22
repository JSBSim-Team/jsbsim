include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Define some commons compile flags.                                           #
################################################################################

if(MSVC)
  add_definitions("/D _USE_MATH_DEFINES")
endif()

################################################################################
# Macros definition.                                                           #
################################################################################

macro(add_full_path_name name file_list)
  foreach(file_name ${file_list})
    list(APPEND files_with_path "${CMAKE_CURRENT_SOURCE_DIR}/${file_name}")
  endforeach()
  set(JSBSIM_${name} ${files_with_path} PARENT_SCOPE)
endmacro()

macro(propagate_source_files name subelement)
  set(JSBSIM_${name}_${subelement}_SRC ${JSBSIM_${subelement}_SRC} PARENT_SCOPE)
  set(JSBSIM_${name}_${subelement}_HDR ${JSBSIM_${subelement}_HDR} PARENT_SCOPE)
endmacro()

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

set(STATIC_LIBNAME_SUFFIX)
if(MSVC AND BUILD_SHARED_LIBS)
    set(STATIC_LIBNAME_SUFFIX "static")
endif()

add_subdirectory(initialization)
add_subdirectory(input_output)
add_subdirectory(math)
add_subdirectory(models)
add_subdirectory(simgear)

if(NOT EXPAT_FOUND)
  add_definitions("-DHAVE_EXPAT_CONFIG_H")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/simgear/xml)
else()
  include_directories(${EXPAT_INCLUDE_DIRS})
  set(JSBSIM_LINK_LIBRARIES ${EXPAT_LIBRARIES} ${JSBSIM_LINK_LIBRARIES})
endif()

set(HEADERS FGFDMExec.h
            FGJSBBase.h)
set(SOURCES FGFDMExec.cpp
            FGJSBBase.cpp)

add_library(libJSBSim ${HEADERS} ${SOURCES}
  ${JSBSIM_INITIALISATION_HDR} ${JSBSIM_INITIALISATION_SRC}
  ${JSBSIM_MODELS_ATMOSPHERE_HDR} ${JSBSIM_MODELS_ATMOSPHERE_SRC}
  ${JSBSIM_MODELS_FLIGHT_CONTROL_HDR} ${JSBSIM_MODELS_FLIGHT_CONTROL_SRC}
  ${JSBSIM_MODELS_PROPULSION_HDR} ${JSBSIM_MODELS_PROPULSION_SRC}
  ${JSBSIM_MODELS_HDR} ${JSBSIM_MODELS_SRC}
  ${JSBSIM_MATH_HDR} ${JSBSIM_MATH_SRC}
  ${JSBSIM_INPUT_OUTPUT_HDR} ${JSBSIM_INPUT_OUTPUT_SRC}
  ${JSBSIM_SIMGEAR_PROPS_HDR} ${JSBSIM_SIMGEAR_PROPS_SRC}
  ${JSBSIM_SIMGEAR_XML_HDR} ${JSBSIM_SIMGEAR_XML_SRC}
  ${JSBSIM_SIMGEAR_MAGVAR_HDR} ${JSBSIM_SIMGEAR_MAGVAR_SRC}
  )

set_target_properties (libJSBSim PROPERTIES
                                 OUTPUT_NAME JSBSim${STATIC_LIBNAME_SUFFIX}
                                 VERSION ${LIBRARY_VERSION})

target_link_libraries(libJSBSim ${JSBSIM_LINK_LIBRARIES})

if(BUILD_SHARED_LIBS)
  set_target_properties (libJSBSim PROPERTIES
                                   SOVERSION ${LIBRARY_SOVERSION}
                                   FRAMEWORK ON)
else()
  # If JSBSim is built as a static library on an x86_64 *nix platform, the code
  # must be compiled with the -fPIC flag otherwise the compiler will complain:
  # relocation R_X86_64_32S against `a local symbol' can not be used when making
  # a shared object; recompile with -fPIC
  # See http://www.cmake.org/pipermail/cmake/2007-May/014350.html
  if (UNIX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set_target_properties(libJSBSim PROPERTIES COMPILE_FLAGS "-fPIC")
  endif()
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

