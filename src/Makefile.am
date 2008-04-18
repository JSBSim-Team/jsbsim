includedir = @includedir@/JSBSim

EXTRA_DIST = \
	JSBSim.hxx JSBSim.cxx

SUBDIRS = initialization models input_output math simgear

noinst_LTLIBRARIES = libJSBBase.la

libJSBBase_la_SOURCES = FGFDMExec.cpp FGJSBBase.cpp FGState.cpp

include_HEADERS = FGFDMExec.h FGJSBBase.h FGState.h

lib_LTLIBRARIES = libJSBSim.la

libJSBSim_la_SOURCES =
libJSBSim_la_LIBADD = \
	libJSBBase.la \
	initialization/libInit.la \
	models/libModels.la \
	models/flight_control/libFlightControl.la \
	models/atmosphere/libAtmosphere.la \
	models/propulsion/libPropulsion.la \
	input_output/libInputOutput.la \
	math/libMath.la \
	math/direct_search/libdirect_search.la \
	simgear/props/libProperties.la \
	simgear/xml/libExpat.la

noinst_PROGRAMS = JSBSim

JSBSim_SOURCES = JSBSim.cpp

JSBSim_LDADD = libJSBSim.la -lm

INCLUDES = -I$(top_srcdir)/src 
