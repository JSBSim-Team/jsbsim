#! /usr/bin/perl
if (length($ENV{CC}) gt 0) {
  print "CC = $ENV{CC}\n";
} else {
  print "CC = g++\n";
}
#print "CCOPTS = -pg -O2\n";
print "INCLUDES = -I. -Isimgear/props\n";
print "LINKDIR= -Lfiltersjb/ -Lsimgear/props/\n";
#
print "JSBSim_objects = FGAircraft.o FGAtmosphere.o FGCoefficient.o FGFCS.o FGFDMExec.o\\\n";
print "FGModel.o FGOutput.o FGState.o \\\n";
print "FGTank.o FGAuxiliary.o FGfdmSocket.o FGTrim.o FGTrimAxis.o\\\n";
print "FGConfigFile.o FGInitialCondition.o FGLGear.o FGMatrix33.o FGPropulsion.o FGRocket.o\\\n";
print "FGPiston.o FGForce.o FGThruster.o FGEngine.o\\\n";
print "FGTable.o FGPropeller.o FGNozzle.o FGAerodynamics.o FGMassBalance.o FGInertial.o\\\n";
print "FGFactorGroup.o FGColumnVector3.o FGQuaternion.o FGGroundReactions.o FGScript.o\\\n";
print "FGJSBBase.o FGPropertyManager.o FGTurbine.o FGElectric.o FGPropagate.o\n\n";
#
print "JSBSim_sources = FG*.cpp FG*.h JSBSim.cpp *.?xx filtersjb/*.cpp filtersjb/*.h simgear/props/*.?xx control/*.xml\\\n";
print "Makefile.* aircraft/*/*.xml engine/*.xml scripts/*.xml *ake* */*ake* */*/*ake*\n\n";
#
print "JSBSim : \$(JSBSim_objects) JSBSim.o libFCSComponents.a libProperties.a\n";
print "	\$(CC) \$(INCLUDES) \$(CCOPTS) \$(LINKDIR) \$(JSBSim_objects) JSBSim.o -oJSBSim -lm -lFCSComponents -lProperties\n\n";
print "libFCSComponents.a:\n";
print "	cd filtersjb; make -fMakefile.solo; cd ..\n\n";
print "libProperties.a:\n";
print "	cd simgear/props; make -fMakefile.solo; cd ../../\n\n";
@files =  glob("*.cpp");
foreach $file (@files) {
  system "g++ -DNOSIMGEAR -I. -MM $file";
  print "	\$(CC) \$(INCLUDES) \$(CCOPTS) -o";
  print substr($file,0,length($file)-4);
  print ".o -c $file\n\n";
}
print "\n";
print "x15trim.o:x15trim.cpp\n";
print "	\$(CC) \$(INCLUDES) \$(CCOPTS) -c x15trim.cpp\n\n";
print "x15trim:\$(JSBSim_objects) x15trim.o libFCSComponents.a\n";
print "	\$(CC) \$(INCLUDES) \$(CCOPTS) \$(LINKDIR) \$(JSBSim_objects) x15trim.o -ox15trim -lm -lFCSComponents\n\n";

print "clean:\n";
print "	-mv *.*~ backup\n";
print "	-rm *.o\n\n";

print "all:\n";
print "	touch *.cpp\n";
print "	cd filtersjb; make all -fMakefile.solo; cd ..\n";
print "	cd simgear/props; make all -fMakefile.solo; cd ..\n";
print "	make JSBSim -fMakefile.solo\n\n";

print "debug:\n";
print "	touch *.cpp\n";
print "	touch filtersjb/*.cpp\n";
print "	touch simgear/props/*.cxx\n";
print "	cd filtersjb; make debug CCOPTS=-g -fMakefile.solo; cd ..\n";
print "	cd simgear/props; make debug CCOPTS=-g -fMakefile.solo; cd ..\n";
print "	make JSBSim CCOPTS=-g -fMakefile.solo\n";

print "dist:\n";
print "	tar -cvf JSBSim_source.tar \$(JSBSim_sources)\n";
print "	gzip JSBSim_source.tar\n\n";

