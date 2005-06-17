AM_CPPFLAGS = -DHAVE_EXPAT_CONFIG_H 

noinst_LIBRARIES = libExpat.a

libExpat_a_SOURCES = xmlparse.c xmltok.c xmlrole.c easyxml.cxx

EXTRA_DIST = xmltok_impl.c  xmltok_ns.c

noinst_HEADERS = ascii.h asciitab.h easyxml.hxx expat.h expat_external.h \
                 iasciitab.h internal.h latin1tab.h macconfig.h nametab.h \
                 utf8tab.h winconfig.h xmlrole.h xmltok.h xmltok_impl.h \
                 expat_config.h

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/simgear/xml
