set(JSBSIM_SIMGEAR_XML_SRC easyxml.cxx)
set(JSBSIM_SIMGEAR_XML_HDR easyxml.hxx)

if (SYSTEM_EXPAT)
  find_package(EXPAT)
endif()

if(NOT EXPAT_FOUND)
  add_definitions("-DHAVE_EXPAT_CONFIG_H")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  list(APPEND JSBSIM_SIMGEAR_XML_SRC xmlparse.c
                                     xmltok.c
                                     xmlrole.c)
  list(APPEND JSBSIM_SIMGEAR_XML_HDR ascii.h
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
else()
  include_directories(${EXPAT_INCLUDE_DIRS})
endif()

add_library(Expat OBJECT ${JSBSIM_SIMGEAR_XML_HDR} ${JSBSIM_SIMGEAR_XML_SRC})

install(FILES ${JSBSIM_SIMGEAR_XML_HDR} DESTINATION include/JSBSim/simgear/xml)
