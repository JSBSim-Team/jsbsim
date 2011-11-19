includedir = @includedir@/JSBSim/simgear/props

LIBRARY_SOURCES = props.cxx

LIBRARY_INCLUDES = props.hxx

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libProperties.la
include_HEADERS = $(LIBRARY_INCLUDES)
libProperties_la_SOURCES = $(LIBRARY_SOURCES)
libProperties_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libProperties.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libProperties_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src
