SUBDIRS = filtersjb

EXTRA_DIST = Makefile.solo

noinst_LIBRARIES = libJSBSim.a

libJSBSim_a_SOURCES = \
	FGAerodynamics.cpp FGAerodynamics.h \
	FGAircraft.cpp FGAircraft.h \
	FGAtmosphere.cpp FGAtmosphere.h \
	FGAuxiliary.cpp FGAuxiliary.h \
	FGCoefficient.cpp FGCoefficient.h \
        FGColumnVector3.cpp FGColumnVector3.h \
        FGColumnVector4.cpp FGColumnVector4.h \
	FGConfigFile.cpp FGConfigFile.h \
	FGDefs.h \
        FGElectric.cpp FGElectric.h \
	FGFCS.cpp FGFCS.h \
	FGFDMExec.cpp FGFDMExec.h \
	FGFactorGroup.cpp FGFactorGroup.h \
	FGForce.cpp FGForce.h \
        FGGroundReactions.cpp FGGroundReactions.h \
        FGInertial.cpp FGInertial.h \
	FGInitialCondition.cpp FGInitialCondition.h \
        FGJSBBase.cpp FGJSBBase.h \
	FGLGear.cpp FGLGear.h \
        FGMassBalance.cpp FGMassBalance.h \
        FGMatrix33.cpp FGMatrix33.h \
	FGModel.cpp FGModel.h \
	FGNozzle.cpp FGNozzle.h \
	FGOutput.cpp FGOutput.h \
        FGPiston.cpp FGPiston.h \
        FGPropeller.cpp FGPropeller.h \
        FGPropulsion.cpp FGPropulsion.h \
	FGPosition.cpp FGPosition.h \
        FGQuaternion.cpp FGQuaternion.h \
	FGRotation.cpp FGRotation.h \
 	FGRotor.cpp FGRotor.h \
	FGRocket.cpp FGRocket.h \
	FGScript.cpp FGScript.h \
	FGState.cpp FGState.h \
	FGTable.cpp FGTable.h \
	FGThruster.cpp FGThruster.h \
	FGTranslation.cpp FGTranslation.h \
	FGTrim.cpp FGTrim.h \
	FGTrimAxis.cpp FGTrimAxis.h \
	FGTurbine.cpp FGTurbine.h \
	FGUtility.cpp FGUtility.h \
	FGEngine.cpp FGEngine.h \
	FGTank.cpp FGTank.h \
	FGfdmSocket.cpp FGfdmSocket.h \
	FGTurbine.cpp FGTurbine.h \
	FGPropertyManager.cpp FGPropertyManager.h  \
	FGSimTurbine.cpp FGSimTurbine.h \
	JSBSim.cxx JSBSim.hxx


# noinst_PROGRAMS = testJSBsim

AM_CXXFLAGS = -DFGFS

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
