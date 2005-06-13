AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control -I$(top_srcdir)/simgear \
              -DNOSIMGEAR -DHAVE_EXPAT_CONFIG_H -DHAVE_MEMMOVE

noinst_LIBRARIES = libExpat.a

libExpat_a_SOURCES = xmlparse.c xmltok.c xmlrole.c easyxml.cxx

noinst_HEADERS = ascii.h asciitab.h easyxml.hxx expat.h expat_external.h \
                 iasciitab.h internal.h latin1tab.h macconfig.h nametab.h \
                 utf8tab.h winconfig.h xmlrole.h xmltok.h xmltok_impl.h
