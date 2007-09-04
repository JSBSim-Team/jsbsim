EXTRA_DIST = \
	JSBSim.hxx JSBSim.cxx

SUBDIRS = initialization models input_output math simgear

noinst_LIBRARIES = libJSBSim.a

libJSBSim_a_SOURCES =  FGFDMExec.cpp FGJSBBase.cpp FGState.cpp

noinst_HEADERS = FGFDMExec.h FGJSBBase.h FGState.h

noinst_PROGRAMS = JSBSim

JSBSim_SOURCES = JSBSim.cpp

JSBSim_LDADD = \
	$(top_builddir)/src/libJSBSim.a \
	$(top_builddir)/src/initialization/libInit.a \
	$(top_builddir)/src/models/libModels.a \
	$(top_builddir)/src/models/flight_control/libFlightControl.a \
	$(top_builddir)/src/models/atmosphere/libAtmosphere.a \
	$(top_builddir)/src/models/propulsion/libPropulsion.a \
	$(top_builddir)/src/input_output/libInputOutput.a \
	$(top_builddir)/src/math/libMath.a \
	$(top_builddir)/src/math/direct_search/libdirect_search.a \
	$(top_builddir)/src/simgear/props/libProperties.a \
	$(top_builddir)/src/simgear/xml/libExpat.a \
	-lm

INCLUDES = -I$(top_srcdir)/src 
