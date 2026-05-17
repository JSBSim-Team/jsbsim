#!/bin/tcsh
foreach file (*.csv)
  if (-f data_plot/$file:r.xml) prep_plot $file --plot=data_plot/$file:r.xml | gnuplot
end
gs -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile=output.pdf *.ps
