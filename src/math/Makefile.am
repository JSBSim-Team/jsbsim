includedir = @includedir@/JSBSim/math

LIBRARY_SOURCES = FGColumnVector3.cpp FGFunction.cpp FGLocation.cpp FGMatrix33.cpp \
                    FGPropertyValue.cpp FGQuaternion.cpp FGRealValue.cpp FGTable.cpp \
                    FGCondition.cpp FGRungeKutta.cpp FGModelFunctions.cpp

LIBRARY_INCLUDES = FGColumnVector3.h FGFunction.h FGLocation.h FGMatrix33.h \
                 FGParameter.h FGPropertyValue.h FGQuaternion.h FGRealValue.h FGTable.h \
                 FGCondition.h FGRungeKutta.h FGModelFunctions.h LagrangeMultiplier.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libMath.la
include_HEADERS = $(LIBRARY_INCLUDES)
libMath_la_SOURCES = $(LIBRARY_SOURCES)
libMath_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libMath.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libMath_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../
