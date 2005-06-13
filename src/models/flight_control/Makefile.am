AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libFlightControl.a 

libFlightControl_a_SOURCES = FGCondition.cpp FGDeadBand.cpp FGFCSComponent.cpp \
                            FGFilter.cpp FGGain.cpp FGGradient.cpp FGKinemat.cpp \
                            FGSummer.cpp FGSwitch.cpp FGFCSFunction.cpp

noinst_HEADERS = FGCondition.h FGDeadBand.h FGFCSComponent.h FGFilter.h \
                 FGGain.h FGGradient.h FGKinemat.h FGSummer.h FGSwitch.h FGFCSFunction.h

