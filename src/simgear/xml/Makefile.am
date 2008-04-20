includedir = @includedir@/JSBSim/simgear/xml

AM_CPPFLAGS = -DHAVE_EXPAT_CONFIG_H 

LIBRARY_SOURCES = xmlparse.c xmltok.c xmlrole.c easyxml.cxx

EXTRA_DIST = xmltok_impl.c  xmltok_ns.c

LIBRARY_INCLUDES = ascii.h asciitab.h easyxml.hxx expat.h expat_external.h \
                 iasciitab.h internal.h latin1tab.h macconfig.h nametab.h \
                 utf8tab.h winconfig.h xmlrole.h xmltok.h xmltok_impl.h \
                 expat_config.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libExpat.la
include_HEADERS = $(LIBRARY_INCLUDES)
libExpat_la_SOURCES = $(LIBRARY_SOURCES)
libExpat_la_CFLAGS = $(AM_CFLAGS)
libExpat_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libExpat.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libExpat_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/simgear/xml
