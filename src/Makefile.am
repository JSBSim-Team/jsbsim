includedir = @includedir@/JSBSim

SUBDIRS = initialization models input_output math simgear utilities

LIBRARY_SOURCES = FGFDMExec.cpp FGJSBBase.cpp

LIBRARY_INCLUDES = FGFDMExec.h FGJSBBase.h

noinst_PROGRAMS = JSBSim

if BUILD_LIBRARIES

lib_LTLIBRARIES = libJSBSim.la
include_HEADERS = $(LIBRARY_INCLUDES)

libJSBSim_la_SOURCES = $(LIBRARY_SOURCES)
libJSBSim_la_LIBADD = \
	initialization/libInit.la \
	models/atmosphere/libAtmosphere.la \
	models/libModels.la \
	models/flight_control/libFlightControl.la \
	models/propulsion/libPropulsion.la \
	math/libMath.la \
	input_output/libInputOutput.la \
	simgear/props/libProperties.la \
	simgear/xml/libExpat.la \
	simgear/magvar/libcoremag.la
libJSBSim_la_CXXFLAGS = $(AM_CXXFLAGS)

JSBSim_SOURCES = JSBSim.cpp
JSBSim_LDADD = libJSBSim.la -lm

else

noinst_HEADERS = $(LIBRARY_INCLUDES)

JSBSim_SOURCES = JSBSim.cpp $(LIBRARY_SOURCES)
JSBSim_LDADD = \
	initialization/libInit.a \
	models/atmosphere/libAtmosphere.a \
	models/libModels.a \
	models/flight_control/libFlightControl.a \
	models/propulsion/libPropulsion.a \
	math/libMath.a \
	input_output/libInputOutput.a \
	simgear/props/libProperties.a \
	simgear/xml/libExpat.a \
	simgear/magvar/libcoremag.a \
	-lm

endif

INCLUDES = -I$(top_srcdir)/src 
