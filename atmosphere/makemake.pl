#! /usr/bin/perl
if (length($ENV{CC}) gt 0) {
  print "CC = $ENV{CC}\n";
} else {
  print "CC = g++\n";
}
print "libAtmosphere_OBJECTS = FGMSIS.o FGMars.o FGMSISData.o\n\n";
print "INCLUDES = -I../\n\n";
print "libAtmosphere.a: \$(libAtmosphere_OBJECTS)\n";
print "	-rm -f libAtmosphere.a\n";
print "	ar cru libAtmosphere.a \$\(libAtmosphere_OBJECTS)\n";
print "	ranlib libAtmosphere.a\n\n";
@files =  glob("*.cpp");
foreach $file (@files) {
  system "g++ -I../ -MM $file";
  print "	\$(CC) \$(INCLUDES) \$(CCOPTS) -o";
  print substr($file,0,length($file)-4);
  print ".o -c $file\n\n";
} 
print "clean:\n";
print "	-rm -f *.o\n";
print "	-rm -f *.a\n\n";
print "all:\n";
print "	touch *.cpp\n";
print "	make libAtmosphere.a -fMakefile.solo\n\n";
print "debug:\n";
print "	make all CCOPTS=-g -fMakefile.solo\n";
