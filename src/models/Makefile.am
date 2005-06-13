SUBDIRS = atmosphere propulsion flight_control

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/math -I$(top_srcdir)/initialization \
              -I$(top_srcdir)/input_output -I$(top_srcdir)/simgear/xml \
              -I$(top_srcdir)/prop -I$(top_srcdir)/models -I$(top_srcdir)/models/propulsion \
              -I$(top_srcdir)/models/flight_control 

noinst_LIBRARIES = libModels.a

libModels_a_SOURCES = FGAerodynamics.cpp FGAircraft.cpp FGAtmosphere.cpp \
                      FGAuxiliary.cpp FGFCS.cpp FGGroundReactions.cpp FGInertial.cpp \
                      FGLGear.cpp FGMassBalance.cpp FGModel.cpp FGOutput.cpp \
                      FGPropagate.cpp FGPropulsion.cpp FGUtility.cpp

noinst_HEADERS = FGAerodynamics.h FGAircraft.h FGAtmosphere.h FGAuxiliary.h \
                 FGFCS.h FGGroundReactions.h FGInertial.h FGLGear.h FGMassBalance.h \
                 FGModel.h FGOutput.h FGPropagate.h FGPropulsion.h FGUtility.h 
