includedir = @includedir@/JSBSim/models/propulsion

LIBRARY_SOURCES = FGElectric.cpp FGEngine.cpp FGForce.cpp FGNozzle.cpp \
                         FGPiston.cpp FGPropeller.cpp FGRocket.cpp \
                         FGTank.cpp FGThruster.cpp FGTurbine.cpp FGTurboProp.cpp

LIBRARY_INCLUDES = FGElectric.h FGEngine.h FGForce.h FGNozzle.h FGPiston.h \
                  FGPropeller.h FGRocket.h FGTank.h FGThruster.h \
                  FGTurbine.h FGTurboProp.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libPropulsion.la
include_HEADERS = $(LIBRARY_INCLUDES)
libPropulsion_la_SOURCES = $(LIBRARY_SOURCES)
libPropulsion_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libPropulsion.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libPropulsion_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../../
