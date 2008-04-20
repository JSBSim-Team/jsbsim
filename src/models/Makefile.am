includedir = @includedir@/JSBSim/models

SUBDIRS = atmosphere propulsion flight_control

LIBRARY_SOURCES = FGAerodynamics.cpp FGAircraft.cpp FGAtmosphere.cpp \
                      FGAuxiliary.cpp FGFCS.cpp FGGroundReactions.cpp FGInertial.cpp \
                      FGLGear.cpp FGMassBalance.cpp FGModel.cpp FGOutput.cpp \
                      FGPropagate.cpp FGPropulsion.cpp FGInput.cpp \
                      FGExternalReactions.cpp FGExternalForce.cpp \
                      FGBuoyantForces.cpp FGGasCell.cpp

LIBRARY_INCLUDES = FGAerodynamics.h FGAircraft.h FGAtmosphere.h FGAuxiliary.h \
                 FGFCS.h FGGroundReactions.h FGInertial.h FGLGear.h FGMassBalance.h \
                 FGModel.h FGOutput.h FGPropagate.h FGPropulsion.h FGInput.h \
                 FGExternalReactions.h FGExternalForce.h \
                 FGBuoyantForces.h FGGasCell.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libModels.la
include_HEADERS = $(LIBRARY_INCLUDES)
libModels_la_SOURCES = $(LIBRARY_SOURCES)
libModels_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libModels.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libModels_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../
