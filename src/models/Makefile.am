includedir = @includedir@/JSBSim/models

SUBDIRS = atmosphere propulsion flight_control

noinst_LTLIBRARIES = libModels.la

libModels_la_SOURCES = FGAerodynamics.cpp FGAircraft.cpp FGAtmosphere.cpp \
                      FGAuxiliary.cpp FGFCS.cpp FGGroundReactions.cpp FGInertial.cpp \
                      FGLGear.cpp FGMassBalance.cpp FGModel.cpp FGOutput.cpp \
                      FGPropagate.cpp FGPropulsion.cpp FGInput.cpp \
                      FGExternalReactions.cpp FGExternalForce.cpp \
                      FGBuoyantForces.cpp FGGasCell.cpp

include_HEADERS = FGAerodynamics.h FGAircraft.h FGAtmosphere.h FGAuxiliary.h \
                 FGFCS.h FGGroundReactions.h FGInertial.h FGLGear.h FGMassBalance.h \
                 FGModel.h FGOutput.h FGPropagate.h FGPropulsion.h FGInput.h \
                 FGExternalReactions.h FGExternalForce.h \
                 FGBuoyantForces.h FGGasCell.h

INCLUDES = -I$(top_srcdir)/src -I../
