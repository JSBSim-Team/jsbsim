includedir = @includedir@/JSBSim/models/flight_control

LIBRARY_SOURCES = FGDeadBand.cpp FGFCSComponent.cpp \
                            FGFilter.cpp FGGain.cpp FGKinemat.cpp \
                            FGSummer.cpp FGSwitch.cpp FGFCSFunction.cpp FGSensor.cpp \
                            FGPID.cpp FGActuator.cpp FGAccelerometer.cpp

LIBRARY_INCLUDES = FGDeadBand.h FGFCSComponent.h FGFilter.h \
                 FGGain.h FGKinemat.h FGSummer.h FGSwitch.h FGFCSFunction.h \
                 FGSensor.h FGPID.h FGActuator.h FGAccelerometer.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libFlightControl.la
include_HEADERS = $(LIBRARY_INCLUDES)
libFlightControl_la_SOURCES = $(LIBRARY_SOURCES)
libFlightControl_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libFlightControl.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libFlightControl_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../../
