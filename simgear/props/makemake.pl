#! /usr/bin/perl
if (length($ENV{CC}) gt 0) {
  print "CC = $ENV{CC}\n";
} else {
  print "CC = g++\n";
}
print "libProperties_OBJECTS = props.o\n\n";
print "INCLUDES = -I../../\n\n";
print "libProperties.a: \$(libProperties_OBJECTS)\n";
print "	-rm -f libProperties.a\n";
print "	ar cru libProperties.a \$\(libProperties_OBJECTS)\n";
print "	ranlib libProperties.a\n\n";
@files =  glob("*.cxx");
foreach $file (@files) {
  system "g++ -I../../ -MM $file";
  print "	\$(CC) \$(INCLUDES) \$(CCOPTS) -o";
  print substr($file,0,length($file)-4);
  print ".o -c $file\n\n";
} 
print "clean:\n";
print "	-rm -f *.o\n";
print "	-rm -f *.a\n\n";
print "all:\n";
print "	touch *.cxx\n";
print "	make libProperties.a -fMakefile.solo\n\n";
print "debug:\n";
print "	make all CCOPTS=-g -fMakefile.solo\n";
