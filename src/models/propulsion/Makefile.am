AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libPropulsion.a

libPropulsion_a_SOURCES = FGElectric.cpp FGEngine.cpp FGForce.cpp FGNozzle.cpp \
                         FGPiston.cpp FGPropeller.cpp FGRocket.cpp FGRotor.cpp \
                         FGTank.cpp FGThruster.cpp FGTurbine.cpp

noinst_HEADERS =  FGElectric.h FGEngine.h FGForce.h FGNozzle.h FGPiston.h \
                  FGPropeller.h FGRocket.h FGRotor.h FGTank.h FGThruster.h \
                  FGTurbine.h

