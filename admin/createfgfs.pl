#! /usr/bin/perl -w
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#
# File:         CreateFGFS.pl
# Author:       Jon Berndt based on scripts by Norman Vine and Curt Olson
# Date started: 12/20/00
# Purpose:      gets and builds code for FGFS; or updates / makes as requested
#               type;
#                 ./createfgfs help
#               at the command line for more info. Should work on any platform.

$CLEAN   = 0;
$FGFS    = 0;
$FGB     = 0;
$PLIB    = 0;
$SIMGEAR = 0;
$MAKE    = 1;
$BUILD   = 0;
$UPDATE  = 0;
$HELP    = 0;

for ($i=0;$i<=$#ARGV;$i++) {
  if ($ARGV[$i] eq "update" or $ARGV[$i] eq "UPDATE") {
    $UPDATE = 1;
    $BUILD  = 1;
    $MAKE   = 1;
  } elsif ($ARGV[$i] eq "build" or $ARGV[$i] eq "BUILD") {
    $BUILD = 1;
    $MAKE = 1;
  } elsif ($ARGV[$i] eq "make" or $ARGV[$i] eq "MAKE") {
    $MAKE = 1;
  } elsif ($ARGV[$i] eq "plib" or $ARGV[$i] eq "PLIB") {
    $PLIB    = 1;
    $SIMGEAR = 1;
    $FGFS    = 1;
  } elsif ($ARGV[$i] eq "simgear" or $ARGV[$i] eq "SIMGEAR") {
    $SIMGEAR = 1;
    $FGFS    = 1;
  } elsif ($ARGV[$i] eq "fgfs" or $ARGV[$i] eq "FGFS") {
    $FGFS = 1;
  } elsif ($ARGV[$i] eq "fgb" or $ARGV[$i] eq "FGB") {
    $FGB = 1;
  } elsif ($ARGV[$i] eq "all" or $ARGV[$i] eq "ALL") {
    $PLIB    = 1;
    $SIMGEAR = 1;
    $FGFS    = 1;
    $FGB     = 1;
  } elsif ($ARGV[$i] eq "clean" or $ARGV[$i] eq "CLEAN") {
    $CLEAN = 1;
  } elsif ($ARGV[$i] eq "help" or $ARGV[$i] eq "HELP") {
    $HELP = 1;
  } else {
    $HELP = 1;
  }
}

if ($PLIB eq 0 and $SIMGEAR eq 0 and $FGFS eq 0 and $FGB eq 0 and $HELP eq 0) {
  $HELP = 1;
  print "\n\nYou must specify an entity to operate on\n";
}

if ($HELP) {
  print "\nUsage:\n\n";
  print "CreateFGFS <UPDATE|MAKE|BUILD> <PLIB|SIMGEAR|FGFS|FGB|ALL> <CLEAN>\n\n";
  print "Options can be entered all lower or all upper case\n\n";
  print "UPDATE causes the selected entities to be updated from\n";
  print "their respective CVS repositories. All lower dependencies\n";
  print "will be executed as well, i.e. UPDATE causes a BUILD and a\n";
  print "MAKE to be performed. If this is an initial checkout (i.e.\n";
  print "the cvs/root file does not exist for an entity) you will need\n";
  print "to be present in order to enter the default password for the\n";
  print "cvs repository at checkout\n";
  print "BUILD causes the configure; aclocal; automake, etc. tools to\n";
  print "be run.\n";
  print "MAKE merely causes a make to be run on the selected entity[ies]\n";
  print "MAKE does not need to be specified, as it is the default.\n";
  print "PLIB causes plib, simgear, and fgfs to be operated on as";
  print "specified\n";
  print "SIMGEAR causes simgear and fgfs to be operated on as specified\n";
  print "FGFS causes flightgear to be operated on as specified\n\n";
  print "FGB causes the flightgear base package to be retrieved\n\n";
  print "CLEAN causes the directories to be cleaned of object files\n";
  print "after make has been run. This is good to do if you have\n";
  print "limited space, but you will need to rebuild the entity again\n";
  print "in its entirity the next time you run make or build\n";
  print "\nThis script assumes that the code will be kept in directories\n";
  print "under the user home directory, under an src directory\n";
  print "FlightGear base files are kept under the user home directory\n";
  print "under a fgfsbase directory\n";
  die "\n";
}

system("export FG_ROOT='~/fgfsbase'");

if (!chdir) {die "Cannot chdir to ~\n";}

if (!chdir "src") {
  mkdir "src";
  print "\nYou have not checked out any code yet. You will first do an update\n";
  $UPDATE  = 1;
  $BUILD   = 1;
  $PLIB    = 1;
  $SIMGEAR = 1;
  $FGB     = 1;
  $FGFS    = 1;
} else {
  chdir; # go back to $HOME
  if (!chdir "data/CVS") {
    print "\nYou have not yet installed the base package. Base package will be installed\n";
    $UPDATE = 1;
    $BUILD   = 1;
    $FGB    = 1;
  }
  chdir; # go back to $HOME
  if (!chdir "src/plib/CVS") {
    print "\nYou have not yet installed the plib package. Plib will be installed\n";
    $UPDATE = 1;
    $BUILD   = 1;
    $PLIB   = 1;
  }
  chdir; # go back to $HOME
  if (!chdir "src/simgear/source/CVS") {
    print "\nYou have not yet installed the simgear package. SimGear will be installed\n";
    $UPDATE  = 1;
    $BUILD   = 1;
    $SIMGEAR = 1;
  }
  chdir; # go back to $HOME
  if (!chdir "src/flightgear/source/CVS") {
    print "\nYou have not yet installed the flightgear package. FlightGear will be installed\n";
    $UPDATE = 1;
    $BUILD   = 1;
    $FGFS   = 1;
  }
}

if ($UPDATE) { #start update code

  if ($PLIB) {
    chdir; # go back to $HOME
    mkdir "src";

    $result = open (PLIBFILE, "src/plib/CVS/root" ) ;
    close(PLIBFILE);
    if (!$result) {
      if (!chdir "src") {die "Cannot chdir to ~/src\n";}
      print "\n\n--------------------------------\n\n";
      print "Checking out plib from cvs\n\n";
      print "Hit enter key when requested for password\n";
      print "--------------------------------\n";
      system("cvs -d:pserver:anonymous\@plib.cvs.sourceforge.net:/cvsroot/plib login");
      system("cvs -z3 -d:pserver:anonymous\@plib.cvs.sourceforge.net:/cvsroot/plib co -P plib");
    } else {
      print "\n\n--------------------------------\n\n";
      print "Updating plib from cvs\n\n";
      print "--------------------------------\n";
      if (!chdir "src/plib/") {die "Cannot chdir to ~/src/plib/";}
      system("cvs -z3 update -Pd");
    }
  }

#-------------------------------

  if ($SIMGEAR) {
    if (!chdir "") {die "Cannot chdir to ~\n";}
    $result = open (SGFILE, "src/simgear/source/CVS/Root" ) ;
    close(SGFILE);
    if (!$result) {
      if (!chdir "src/simgear/source") {die "Cannot chdir to ~src/simgear/source\n";}
      print "\n\n--------------------------------\n\n";
      print "Checking out simgear from cvs\n\n";
      print "Enter 'guest' when requested for password\n";
      print "--------------------------------\n";
      system("cvs -d :pserver:cvsguest\@cvs.simgear.org:/var/cvs/SimGear-0.3 login");
      system("cvs -d :pserver:cvsguest\@cvs.simgear.org:/var/cvs/SimGear-0.3 co source");
    } else {
      if (!chdir "src/simgear/source") {die "Cannot chdir to ~/src/simgear/source\n";}
      print "\n\n--------------------------------\n\n";
      print "Updating simgear from cvs\n\n";
      print "--------------------------------\n";
      system("cvs update -Pd");
    }
  }

#-------------------------------

  if ($FGB) {
    if (!chdir "") {die "Cannot chdir to ~\n";}
    $result = open (FGBFILE, "data/CVS/root" ) ;
    close(FGBFILE);
    if (!$result) {
      print "\n\n--------------------------------\n\n";
      print "Checking out flightgear base from cvs\n\n";
      print "Enter 'guest' when requested for cvs password\n";
      print "--------------------------------\n";
      system("cvs -d :pserver:cvsguest\@cvs.flightgear.org:/var/cvs/FlightGear-0.9 login");
      system("cvs -d :pserver:cvsguest\@cvs.flightgear.org:/var/cvs/FlightGear-0.9 co data");
    } else {
      if (!chdir "data") {die "Cannot chdir to ~/data\n";}
      print "\n\n--------------------------------\n\n";
      print "Updating flightgear base from cvs\n\n";
      print "--------------------------------\n";
      system("cvs -z3 update -Pd");
    }
  }

#-------------------------------

  if ($FGFS) {
    if (!chdir "") {die "Cannot chdir to ~\n";}
    $result = open (FGFILE, "src/flightgear/source/src/CVS/root" ) ;
    close(FGFILE);
    if (!$result) {
      if (!chdir "src/flightgear") {die "Cannot chdir to ~/src/flightgear\n";}
      print "\n\n--------------------------------\n\n";
      print "Checking out flightgear DEVELOPMENT source from cvs\n\n";
      print "Enter 'guest' when requested for password\n";
      print "--------------------------------\n";
      system("cvs -d :pserver:cvsguest\@cvs.flightgear.org:/var/cvs/FlightGear-0.9 login");
      system("cvs -d :pserver:cvsguest\@cvs.flightgear.org:/var/cvs/FlightGear-0.9 co source");
    } else {
      if (!chdir "src/flightgear/source/src/") {die "Cannot chdir to ~/src/flightgear/source/src/\n";}
      print "\n\n--------------------------------\n\n";
      print "Updating flightgear source from cvs\n\n";
      print "--------------------------------\n";
      system("cvs -z3 update -Pd");
    }
  }

} # end update code

#----------------------------------------------------------------------

if ($PLIB) { #make plib
  if (!chdir) {die "Cannot chdir to ~\n";}
  if (!chdir "src/plib") {die "Cannot chdir to ~/src/plib";}
  if ($BUILD) {
    print "\n\n BUILDING PLIB ***";
    print "\n\n   --- rm config.cache ---\n\n";
    system("rm config.cache");
    print "\n\n   --- make distclean ---\n\n";
    system("if (!(make distclean)) then echo ' ... already clean\n\n'; fi;");
    print "\n\n   --- aclocal ---\n\n";
    system("aclocal");
    print "\n\n   --- automake -a ---\n\n";
    system("automake -a");
    print "\n\n   --- autoconf ---\n\n";
    system("autoconf");
    print "\n\n   --- configure ---\n\n";
    system('CFLAGS="" CXXFLAGS="" ./configure --without-logging');
  }
  print "\n\n MAKING PLIB ***\n\n";
  print "\n\n   --- make ---\n\n";
  system("make");
  print "\n\n   --- make install ---\n\n";
  system("make install");
  if ($CLEAN) {system("make clean");}
  print "\n\n*** PLIB FINISHED\n\n";
}

if ($SIMGEAR) { #make simgear
  if (!chdir "") {die "Cannot chdir to ~\n";}
  if (!chdir "src/simgear/source") {die "Cannot chdir to ~/src/simgear/source\n";}
  if ($BUILD) {
    print "\n\n BUILDING SIMGEAR ***";
    print "\n\n   --- rm config.cache ---\n\n";
    system("rm config.cache");
    print "\n\n   --- make distclean ---\n\n";
    system("if (!(make distclean)) then echo ' ... already clean\n\n'; fi;");
    print "\n\n   --- aclocal ---\n\n";
    system("aclocal");
    print "\n\n   --- autoheader ---\n\n";
    system("autoheader");
    print "\n\n   --- automake --add-missing ---\n\n";
    system("automake --add-missing");
    print "\n\n   --- autoconf ---\n\n";
    system("autoconf");
    print "\n\n   --- configure ---\n\n";
    system('CFLAGS="" CXXFLAGS="" CPPFLAGS="-DNOMINMAX" ./configure --with-logging');
  }
  print "\n\n MAKING SIMGEAR ***";
  print "\n\n   --- make ---\n\n";
  system("make");
  print "\n\n   --- make install ---\n\n";
  system("make install");
  if ($CLEAN) {system("make clean");}
  print "\n\n*** SIMGEAR FINISHED\n\n";
}

if ($FGFS) { #make flightgear
  if (!chdir "") {die "Cannot chdir to ~\n";}
  if (!chdir "src/flightgear/source") {die "Cannot chdir to ~/src/flightgear/source\n";}
  if ($BUILD) {
    print "\n\n BUILDING FLIGHTGEAR ***";
    print "\n\n   --- rm config.cache ---\n\n";
    system("rm config.cache");
    print "\n\n   --- make distclean ---\n\n";
    system("if (!(make distclean)) then echo ' ... already clean\n\n'; fi;");
    print "\n\n   --- aclocal ---\n\n";
    system("aclocal");
    print "\n\n   --- autoheader ---\n\n";
    system("autoheader");
    print "\n\n   --- automake --add-missing ---\n\n";
    system("automake --add-missing");
    print "\n\n   --- autoconf ---\n\n";
    system("autoconf");
    print "\n\n   --- configure ---\n\n";
    system('CFLAGS="" CXXFLAGS="" CPPFLAGS="-DNOMINMAX" ./configure --with-logging');
  }
  print "\n\n MAKING FLIGHTGEAR***";
  print "\n\n   --- make ---\n\n";
  system("make");
  print "\n\n   --- make install ---\n\n";
  system("make install");
  if ($CLEAN) {system("make clean");}
  print "\n\n*** FLIGHTGEAR FINISHED\n\n";
}

