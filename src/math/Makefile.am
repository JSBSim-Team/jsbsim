AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libMath.a

libMath_a_SOURCES = FGColumnVector3.cpp FGFunction.cpp FGLocation.cpp FGMatrix33.cpp \
                    FGPropertyValue.cpp FGQuaternion.cpp FGRealValue.cpp FGTable.cpp

noinst_HEADERS = FGColumnVector3.h FGFunction.h FGLocation.h FGMatrix33.h \
                 FGParameter.h FGPropertyValue.h FGQuaternion.h FGRealValue.h FGTable.h 
