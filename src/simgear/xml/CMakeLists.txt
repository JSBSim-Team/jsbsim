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
                      siphash.h)

  set_property(SOURCE xcsinc.c PROPERTY HEADER_FILE_ONLY)
  set_property(SOURCE xmltok_impl.c PROPERTY HEADER_FILE_ONLY)
  set_property(SOURCE xmltok_ns.c PROPERTY HEADER_FILE_ONLY)

  set(XML_GE 1)                # Make general entity parsing functionality available.
  set(XML_CONTEXT_BYTES 1024)  # Specify how much context to retain around the current parse point, 0 to disable.

  include(${CMAKE_CURRENT_SOURCE_DIR}/ConfigureChecks.cmake)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/expat_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/expat_config.h)
  list(APPEND HEADERS ${CMAKE_CURRENT_BINARY_DIR}/expat_config.h)

  if(HAVE_ARC4RANDOM_BUF)
    list(APPEND SOURCES random_arc4random_buf.c)
    list(APPEND HEADERS random_arc4random_buf.h
                        memory_sanitizer.h)
  elseif(HAVE_ARC4RANDOM)
    list(APPEND SOURCES random_arc4random.c)
    list(APPEND HEADERS random_arc4random.h)
  elseif(WIN32)
    list(APPEND SOURCES random_rand_s.c)
    list(APPEND HEADERS random_rand_s.h)
  elseif(HAVE_GETENTROPY)
    list(APPEND SOURCES random_getentropy.c)
    list(APPEND HEADERS random_getentropy.h
                        memory_sanitizer.h)
  elseif(HAVE_GETRANDOM OR HAVE_SYSCALL_GETRANDOM)
    list(APPEND SOURCES random_getrandom.c)
    list(APPEND HEADERS random_getrandom.h
                        memory_sanitizer.h)
  else()
    list(APPEND SOURCES random_dev_urandom.c)
    list(APPEND HEADERS random_dev_urandom.h)
  endif()
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
else(NOT SYSTEM_EXPAT)
  target_include_directories(Xml PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  if(WIN32)
    target_compile_definitions(Xml PRIVATE XML_STATIC)
  endif()
endif(SYSTEM_EXPAT)

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/xml COMPONENT devel)
