set(SOURCES easyxml.cxx)
set(HEADERS easyxml.hxx)

if(NOT SYSTEM_EXPAT)
  list(APPEND SOURCES xmlparse.c
                      xmltok.c
                      xmlrole.c)
  list(APPEND HEADERS ascii.h
                      asciitab.h
                      expat.h
                      expat_external.h
                      fallthrough.h
                      hash_table.h
                      iasciitab.h
                      internal.h
                      latin1tab.h
                      nametab.h
                      utf8tab.h
                      winconfig.h
                      xcsinc.c
                      xmlrole.h
                      xmltok.h
                      xmltok_impl.h
                      xmltok_impl.c
                      xmltok_ns.c
                      expat_config.h
                      siphash.h)

  set_property(SOURCE xcsinc.c PROPERTY HEADER_FILE_ONLY)
  set_property(SOURCE xmltok_impl.c PROPERTY HEADER_FILE_ONLY)
  set_property(SOURCE xmltok_ns.c PROPERTY HEADER_FILE_ONLY)

  if(WIN32)
    list(APPEND SOURCES random_rand_s.c)
    list(APPEND HEADERS random_rand_s.h)
  else()
    list(APPEND SOURCES random_dev_urandom.c)
    list(APPEND HEADERS random_dev_urandom.h)
  endif(WIN32)
endif(NOT SYSTEM_EXPAT)

add_library(Xml OBJECT ${SOURCES})

if(SYSTEM_EXPAT)
  target_compile_definitions(Xml PRIVATE SYSTEM_EXPAT)
  if (PkgConfig_FOUND)
    pkg_check_modules(PC_EXPAT IMPORTED_TARGET expat)
    target_link_libraries(Xml PkgConfig::PC_EXPAT)
  else()
    find_package(EXPAT)
    target_link_libraries(Xml EXPAT::EXPAT)
  endif()
elseif(WIN32)
  target_compile_definitions(Xml PRIVATE XML_STATIC)
endif()

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/xml COMPONENT devel)
