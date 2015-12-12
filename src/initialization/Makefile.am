includedir = @includedir@/JSBSim/initialization

LIBRARY_SOURCES = FGInitialCondition.cpp FGTrim.cpp FGTrimAxis.cpp

LIBRARY_INCLUDES = FGInitialCondition.h FGTrim.h FGTrimAxis.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libInit.la
include_HEADERS = $(LIBRARY_INCLUDES)
libInit_la_SOURCES = $(LIBRARY_SOURCES)
libInit_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libInit.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libInit_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../
