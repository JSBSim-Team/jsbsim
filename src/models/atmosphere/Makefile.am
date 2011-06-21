includedir = @includedir@/JSBSim/models/atmosphere

LIBRARY_SOURCES = FGMSIS.cpp FGMSISData.cpp FGMars.cpp FGStandardAtmosphere.cpp FGWinds.cpp

LIBRARY_INCLUDES = FGMSIS.h FGMars.h FGStandardAtmosphere.h FGWinds.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libAtmosphere.la
include_HEADERS = $(LIBRARY_INCLUDES)
libAtmosphere_la_SOURCES = $(LIBRARY_SOURCES)
libAtmosphere_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libAtmosphere.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libAtmosphere_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../../
