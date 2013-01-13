set(JSBSIM_SIMGEAR_XML_SRC xmlparse.c
                           xmltok.c
                           xmlrole.c
                           easyxml.cxx)

set(JSBSIM_SIMGEAR_XML_HDR ascii.h
                           asciitab.h
                           easyxml.hxx
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

add_definitions("-DHAVE_EXPAT_CONFIG_H")
add_library(Expat STATIC ${JSBSIM_SIMGEAR_XML_HDR} ${JSBSIM_SIMGEAR_XML_SRC})

install(TARGETS Expat LIBRARY DESTINATION lib
                      ARCHIVE DESTINATION lib)

