#! /usr/bin/perl
if (length($ENV{CC}) gt 0) {
  print "CC = $ENV{CC}\n";
} else {
  print "CC = g++\n";
}
#print "CCOPTS = -pg -O2\n";
print "INCLUDES = -I. -Isimgear/misc\n";
print "LINKDIR= -Lfiltersjb/ -Lsimgear/misc/\n";
print "JSBSim_objects = FGAircraft.o FGAtmosphere.o FGCoefficient.o FGFCS.o FGFDMExec.o\\\n";
print "FGModel.o FGOutput.o FGPosition.o FGRotation.o FGState.o FGTranslation.o\\\n";
print "FGUtility.o FGTank.o FGAuxiliary.o FGfdmSocket.o FGTrim.o FGTrimAxis.o\\\n";
print "FGConfigFile.o FGInitialCondition.o FGLGear.o FGMatrix33.o FGPropulsion.o FGRocket.o\\\n";
print "FGTurbine.o FGPiston.o FGForce.o FGThruster.o FGEngine.o\\\n";
print "FGTable.o FGPropeller.o FGNozzle.o FGAerodynamics.o FGMassBalance.o FGInertial.o\\\n";
print "FGFactorGroup.o FGColumnVector3.o FGColumnVector4.o FGGroundReactions.o FGScript.o\\\n";
print "FGJSBBase.o FGPropertyManager.o FGSimTurbine.o\n\n";
print "JSBSim : \$(JSBSim_objects) JSBSim.o libFCSComponents.a libProperties.a\n";
print "	\$(CC) \$(INCLUDES) \$(CCOPTS) \$(LINKDIR) \$(JSBSim_objects) JSBSim.o -oJSBSim -lm -lFCSComponents -lProperties\n\n";
print "libFCSComponents.a:\n";
print "	cd filtersjb; make -fMakefile.solo; cd ..\n\n";
print "libProperties.a:\n";
print "	cd simgear/misc; make -fMakefile.solo; cd ../../\n\n";
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
print "	cd simgear/misc; make all -fMakefile.solo; cd ..\n";
print "	make JSBSim -fMakefile.solo\n\n";
print "debug:\n";
print "	touch *.cpp\n";
print "	touch filtersjb/*.cpp\n";
print "	touch simgear/misc/*.cxx\n";
print "	cd filtersjb; make debug CCOPTS=-g -fMakefile.solo; cd ..\n";
print "	cd simgear/misc; make debug CCOPTS=-g -fMakefile.solo; cd ..\n";
print "	make JSBSim CCOPTS=-g -fMakefile.solo\n";
