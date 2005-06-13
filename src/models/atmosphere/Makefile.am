AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libAtmosphere.a

libAtmosphere_a_SOURCES = FGMSIS.cpp FGMSISData.cpp FGMars.cpp

noinst_HEADERS = FGMSIS.h FGMars.h

