SUBDIRS = filtersjb simgear

INCLUDES = -I$(top_srcdir)

bin_PROGRAMS = JSBSim

JSBSim_SOURCES = JSBSim.cpp

JSBSim_LDADD = \
	$(top_builddir)/libJSBSim.a \
	$(top_builddir)/filtersjb/libFCSComponents.a \
	$(top_builddir)/simgear/props/libProperties.a \
	-lm

noinst_LIBRARIES = libJSBSim.a

libJSBSim_a_SOURCES = \
	FGAerodynamics.cpp \
	FGAircraft.cpp \
	FGAtmosphere.cpp \
	FGAuxiliary.cpp \
	FGCoefficient.cpp \
	FGColumnVector3.cpp \
	FGConfigFile.cpp \
	FGElectric.cpp \
	FGFCS.cpp \
	FGFDMExec.cpp \
	FGFactorGroup.cpp \
	FGForce.cpp \
	FGGroundReactions.cpp \
	FGInertial.cpp \
	FGInitialCondition.cpp \
	FGJSBBase.cpp \
	FGLGear.cpp \
	FGMassBalance.cpp \
	FGMatrix33.cpp \
	FGModel.cpp \
	FGNozzle.cpp \
	FGOutput.cpp \
	FGPiston.cpp \
	FGPropagate.cpp \
	FGPropeller.cpp \
	FGPropulsion.cpp \
	FGQuaternion.cpp \
	FGRotor.cpp \
	FGRocket.cpp \
	FGScript.cpp \
	FGState.cpp \
	FGTable.cpp \
	FGThruster.cpp \
	FGTrim.cpp \
	FGTrimAxis.cpp \
        FGTurbine.cpp \
	FGEngine.cpp \
	FGTank.cpp \
	FGfdmSocket.cpp \
	FGPropertyManager.cpp \
	FGSimTurbine.cpp

noinst_HEADERS = \
	FGAerodynamics.h \
	FGAircraft.h \
	FGAtmosphere.h \
	FGAuxiliary.h \
	FGCoefficient.h \
	FGColumnVector3.h \
	FGConfigFile.h \
	FGElectric.h \
	FGFCS.h \
	FGFDMExec.h \
	FGFactorGroup.h \
	FGForce.h \
	FGGroundReactions.h \
	FGInertial.h \
	FGInitialCondition.h \
	FGJSBBase.h \
	FGLGear.h \
	FGMassBalance.h \
	FGMatrix33.h \
	FGModel.h \
	FGNozzle.h \
	FGOutput.h \
	FGPiston.h \
	FGPropagate.h \
	FGPropeller.h \
	FGPropulsion.h \
	FGQuaternion.h \
	FGRotor.h \
	FGRocket.h \
	FGScript.h \
	FGState.h \
	FGTable.h \
	FGThruster.h \
	FGTrim.h \
	FGTrimAxis.h \
        FGTurbine.h \
	FGEngine.h \
	FGTank.h \
	FGfdmSocket.h \
	FGPropertyManager.h  \
	FGSimTurbine.h \
	JSBSim.hxx
