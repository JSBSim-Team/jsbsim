AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libInit.a

libInit_a_SOURCES = FGInitialCondition.cpp FGTrim.cpp FGTrimAxis.cpp

noinst_HEADERS = FGInitialCondition.h FGTrim.h FGTrimAxis.h
