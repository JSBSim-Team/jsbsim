if ENABLE_SIMPLOT
UTILITIES_DIR = utilities
endif

# if this line is broken into multiple, make sure an actual directory
# follows $(UTILITIES_DIR) on the same line
SUBDIRS = \
	aircraft engine filtersjb scripts simgear \
	$(UTILITIES_DIR) aeromatic datcom

dist-hook:
	( cd $(top_srcdir); )

INCLUDES = -I$(top_srcdir)

bin_PROGRAMS = JSBSim

JSBSim_SOURCES = \
	JSBSim.cpp \
	JSBSim.hxx

JSBSim_LDADD = \
	$(top_builddir)/libJSBSim.a \
	$(top_builddir)/filtersjb/libFCSComponents.a \
	$(top_builddir)/simgear/props/libProperties.a \
	-lm

lib_LIBRARIES = libJSBSim.a

libJSBSim_a_SOURCES = \
	FGAerodynamics.cpp \
	FGAerodynamics.h \
	FGAircraft.cpp \
	FGAircraft.h \
	FGAtmosphere.cpp \
	FGAtmosphere.h \
	FGAuxiliary.cpp \
	FGAuxiliary.h \
	FGCoefficient.cpp \
	FGCoefficient.h \
	FGColumnVector3.cpp \
	FGColumnVector3.h \
	FGColumnVector4.cpp \
	FGColumnVector4.h \
	FGConfigFile.cpp \
	FGConfigFile.h \
	FGEngine.cpp \
	FGEngine.h \
	FGFactorGroup.cpp \
	FGFactorGroup.h \
	FGFCS.cpp \
	FGFCS.h \
	FGFDMExec.cpp \
	FGFDMExec.h \
	FGfdmSocket.cpp \
	FGfdmSocket.h \
	FGForce.cpp \
	FGForce.h \
	FGGroundReactions.cpp \
	FGGroundReactions.h \
	FGInertial.cpp \
	FGInertial.h \
	FGInitialCondition.cpp \
	FGInitialCondition.h \
	FGJSBBase.cpp \
	FGJSBBase.h \
	FGLGear.cpp \
	FGLGear.h \
	FGMassBalance.cpp \
	FGMassBalance.h \
	FGMatrix33.cpp \
	FGMatrix33.h \
	FGModel.cpp \
	FGModel.h \
	FGNozzle.cpp \
	FGNozzle.h \
	FGOutput.cpp \
	FGOutput.h \
	FGPiston.cpp \
	FGPiston.h \
	FGPosition.cpp \
	FGPosition.h \
	FGPropeller.cpp \
	FGPropeller.h \
	FGPropertyManager.cpp \
	FGPropertyManager.h \
	FGPropulsion.cpp \
	FGPropulsion.h \
	FGRocket.cpp \
	FGRocket.h \
	FGRotation.cpp \
	FGRotation.h \
	FGRotor.cpp \
	FGRotor.h \
        FGScript.cpp \
        FGScript.h \
	FGState.cpp \
	FGState.h \
	FGTable.cpp \
	FGTable.h \
	FGTank.cpp \
	FGTank.h \
	FGThruster.cpp \
	FGThruster.h \
	FGTranslation.cpp \
	FGTranslation.h \
	FGTrimAxis.cpp \
	FGTrimAxis.h \
	FGTrim.cpp \
	FGTrim.h \
	FGTurbine.cpp \
	FGTurbine.h \
        FGSimTurbine.cpp \
        FGSimTurbine.h \
	FGUtility.cpp \
	FGUtility.h

libJSBSim_a_LIBADD = \
	filtersjb/libFCSComponents.a \
	simgear/props/libProperties.a

EXTRA_DIST = \
	autogen.sh
