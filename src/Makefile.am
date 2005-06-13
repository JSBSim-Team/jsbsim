SUBDIRS = initialization models input_output math simgear

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_PROGRAMS = JSBSim

JSBSim_SOURCES = JSBSim.cpp

JSBSim_LDADD = \
	$(top_srcdir)/libJSBSim.a \
	$(top_srcdir)/initialization/libInit.a \
	$(top_srcdir)/models/libModels.a \
	$(top_srcdir)/models/flight_control/libFlightControl.a \
	$(top_srcdir)/models/atmosphere/libAtmosphere.a \
	$(top_srcdir)/models/propulsion/libPropulsion.a \
	$(top_srcdir)/input_output/libInputOutput.a \
	$(top_srcdir)/math/libMath.a \
	$(top_srcdir)/simgear/props/libProperties.a \
	$(top_srcdir)/simgear/xml/libExpat.a \
	-lm

noinst_LIBRARIES = libJSBSim.a

libJSBSim_a_SOURCES =  FGFDMExec.cpp FGJSBBase.cpp FGState.cpp

noinst_HEADERS = FGFDMExec.h FGJSBBase.h FGState.h
