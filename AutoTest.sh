#! /bin/csh

#
# do scripted runs and make plot files for regression testing
#
# Usage: RegresTest <outfilename.html>
#



set test = 1

foreach script (scripts/*.xml)
   echo "----------"
   echo Test $test
   ./jsbsim --script=$script > {$script}:r.out
   head -12 {$script}:r.out | tail -5
   @ test = $test + 1
end
