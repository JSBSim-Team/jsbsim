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
                      expat_config.h)
endif()

add_full_path_name(XML_SRC "${SOURCES}")
add_full_path_name(XML_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/simgear/xml)
