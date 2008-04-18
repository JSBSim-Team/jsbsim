includedir = @includedir@/JSBSim/models/flight_control

noinst_LTLIBRARIES = libFlightControl.la

libFlightControl_la_SOURCES = FGDeadBand.cpp FGFCSComponent.cpp \
                            FGFilter.cpp FGGain.cpp FGGradient.cpp FGKinemat.cpp \
                            FGSummer.cpp FGSwitch.cpp FGFCSFunction.cpp FGSensor.cpp \
                            FGPID.cpp FGActuator.cpp

include_HEADERS = FGDeadBand.h FGFCSComponent.h FGFilter.h \
                 FGGain.h FGGradient.h FGKinemat.h FGSummer.h FGSwitch.h FGFCSFunction.h \
                 FGSensor.h FGPID.h FGActuator.h

INCLUDES = -I$(top_srcdir)/src -I../../
