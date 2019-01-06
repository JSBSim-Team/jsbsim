set(SOURCES easyxml.cxx)
set(HEADERS easyxml.hxx)

if(NOT EXPAT_FOUND)
  add_definitions("-DHAVE_EXPAT_CONFIG_H")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  list(APPEND SOURCES xmlparse.c
                      xmltok.c
                      xmlrole.c)
  list(APPEND HEADERS ascii.h
                      asciitab.h
                      expat.h
                      expat_external.h
                      iasciitab.h
                      internal.h
                      latin1tab.h
                      macconfig.h
                      nametab.h
                      utf8tab.h
                      winconfig.h
                      xmlrole.h
                      xmltok.h
                      xmltok_impl.h
                      expat_config.h)
endif()

add_library(Xml OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Xml PROPERTIES TARGET_DIRECTORY
                          ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/xml COMPONENT devel)
