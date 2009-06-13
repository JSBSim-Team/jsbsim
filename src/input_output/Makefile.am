includedir = @includedir@/JSBSim/input_output

LIBRARY_SOURCES = FGGroundCallback.cpp FGPropertyManager.cpp FGScript.cpp FGXMLElement.cpp FGXMLParse.cpp FGfdmSocket.cpp

LIBRARY_INCLUDES = FGGroundCallback.h FGPropertyManager.h FGScript.h FGXMLElement.h FGXMLParse.h FGfdmSocket.h FGXMLFileRead.h net_fdm.hxx string_utilities.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libInputOutput.la
include_HEADERS = $(LIBRARY_INCLUDES)
libInputOutput_la_SOURCES = $(LIBRARY_SOURCES)
libInputOutput_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libInputOutput.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libInputOutput_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../
