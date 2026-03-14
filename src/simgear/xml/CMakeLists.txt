set(SOURCES easyxml.cxx)
set(HEADERS easyxml.hxx)

if(NOT EXPAT_FOUND)
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
                      xmltok_impl.c
                      xmltok_ns.c
                      expat_config.h
                      siphash.h)
  set_property(SOURCE xmltok_impl.c PROPERTY HEADER_FILE_ONLY)
  set_property(SOURCE xmltok_ns.c PROPERTY HEADER_FILE_ONLY)
endif(NOT EXPAT_FOUND)

add_library(Xml OBJECT ${SOURCES})
target_include_directories(Xml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32 AND NOT EXPAT_FOUND)
  target_compile_definitions(Xml PRIVATE XML_STATIC)
endif()

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/xml COMPONENT devel)
