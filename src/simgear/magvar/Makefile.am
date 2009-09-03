includedir = @includedir@/JSBSim/simgear/magvar

LIBRARY_SOURCES = coremag.cxx

LIBRARY_INCLUDES = coremag.hxx

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libcoremag.la
include_HEADERS = $(LIBRARY_INCLUDES)
libcoremag_la_SOURCES = $(LIBRARY_SOURCES)
libcoremag_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libcoremag.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libcoremag_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/simgear
