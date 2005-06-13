AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libInputOutput.a

libInputOutput_a_SOURCES = FGGroundCallback.cpp FGPropertyManager.cpp FGScript.cpp FGXMLElement.cpp FGXMLParse.cpp FGfdmSocket.cpp

noinst_HEADERS = FGGroundCallback.h FGPropertyManager.h FGScript.h FGXMLElement.h FGXMLParse.h FGfdmSocket.h 
