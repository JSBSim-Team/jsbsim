#! perl
if (length($ENV{CC}) gt 0) {
  print "CC = $ENV{CC}\n";
} else {
  print "CC = g++\n";
}
print "libFCSComponents_OBJECTS = FGDeadBand.o FGFilter.o FGGradient.o FGSwitch.o\\\n";
print "                           FGFCSComponent.o FGGain.o FGSummer.o FGFlaps.o\n\n";
print "INCLUDES = -I../\n\n";
print "libFCSComponents.a: \$(libFCSComponents_OBJECTS)\n";
print "	-rm -f libFCSComponents.a\n";
print "	ar cru libFCSComponents.a \$\(libFCSComponents_OBJECTS)\n";
print "	ranlib libFCSComponents.a\n\n";
@files =  glob("*.cpp");
foreach $file (@files) {
  system "g++ -MM $file";
  print "	\$(CC) \$(INCLUDES) \$(CCOPTS) -o";
  print substr($file,0,length($file)-4);
  print ".o -c $file\n\n";
} 
print "clean:\n";
print "	-rm -f *.o\n";
print "	-rm -f *.a\n\n";
print "all:\n";
print "	touch *.cpp\n";
print "	make libFCSComponents.a -fMakefile.solo\n\n";
print "debug:\n";
print "	make all CCOPTS=-g -fMakefile.solo\n";
