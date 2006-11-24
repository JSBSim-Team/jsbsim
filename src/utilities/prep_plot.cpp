/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       prep_plot.cpp
 Author:       Jon S. Berndt
 Date started: 11/24/2006
 Purpose:      CSV -> gnuplot prepping tool
 
 ------------- Copyright (C) 2006  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

prep_plot
---------

Input:

A data file in comma-separated value (CSV) format, with time in the first
column. The data file is expected to be all numeric. That is, NaN will
mess it up, I think.

Multiple files (currently up to 10) can be input to prep_plot provided
the names of the files are all the same except for a digit. The digit
is substituted for on the command line to prep_plot using the "#" character.

For example:

./prep_plot F4NOutput#.csv

will look for all input files named F4NOutput0.csv, F4NOutput1.csv,
F4NOutput2.csv, etc. up to F4NOutput9.csv. This will probably be modified
in the future to accept a larger number of plots.

Output:

The output is to standard out (the console, stdout). The output consists of
commands to gnuplot. This file is used as input to gnuplot. Gnuplot then
produces a postscript format file consisting of all plots generated. This
postscript file can easily be converted to PDF format using the ps2pdf utility
that is part of Ghostscript.

Compiling:

g++ prep_plot.cpp -o prep_plot

Usage:
(note that an argument with embedded spaces needs to be surrounded by quotes)

prep_plot <filename> <optional title>

I have used this utility as follows:

./prep_plot.exe F4NOutput#.csv "F4N Ground Reactions Testing (0.3, 0.2)" > gpfile.txt
gnuplot gpfile.txt
ps2pdf14 F4NOutput0.ps F4NOutput0.pdf

Special Notes:

When a set of 3 subsequent data terms is encountered in the .csv data file that
have as part of their names the "subscripts" "_X", "_Y", and "_Z" (at the end
of the variable name) the variables will be plotted on the same page.
Otherwise, all items are plotted individually. This capability will be extended
eventually to include _P, _Q, and _R and perhaps other aeronautically relevant
parameters.

The names of data items as output by JSBSim (as part of FGOutput.cpp) will
likely be modified to work better with this utility.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char **argv)
{
  string in_string, var_name;
  vector <string> names;
  int ctr=1, next_comma=0, len=0, start=0, file_ctr=0;
  vector <string> files;
  ifstream infile2;
  string filename(argv[1]), new_filename;
  char num[4];

  if (filename.find("#") != string::npos) { // if plotting multiple files
    while (file_ctr<10) {
      new_filename=filename;
      sprintf(num,"%d",file_ctr);
      new_filename.replace(new_filename.find("#"),1,num);
      infile2.open(new_filename.c_str());
      if (!infile2.is_open()) break;
      infile2.close();
      files.push_back(new_filename);
      file_ctr++;
    }
  } else {
    files.push_back(filename);
  }
  
  ifstream infile(files[0].c_str());
  if (!infile.is_open()) {
    cerr << "Could not open file: " << files[0] << endl;
    exit(-1);
  }
  getline(infile, in_string, '\n');
  
  cout << "set terminal postscript color \"Helvetica,12\"" << endl;
  if (argc >= 3) {
    cout << "set title \"" << argv[2] << "\" font \"Helvetica,12\"" << endl;
  }
  cout << "set output '" << files[0].substr(0,files[0].size()-4) << ".ps'" << endl;
  
  cout << "set lmargin  6" << endl;
  cout << "set rmargin  4" << endl;
  cout << "set tmargin  1" << endl;
  cout << "set bmargin  1" << endl;
  
  cout << "set datafile separator \",\"" << endl;
  cout << "set grid xtics ytics" << endl;
  cout << "set xtics font \"Helvetica,8\"" << endl;
  cout << "set ytics font \"Helvetica,8\"" << endl;
  cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \"Helvetica,8\"" << endl;

  while(next_comma != string::npos) {
    next_comma=in_string.find_first_of(",",start);
    if (next_comma == string::npos) {
      var_name=in_string.substr(start);
    } else {
      len = next_comma-start;
      var_name=in_string.substr(start,len);
    }
    var_name.erase(0,var_name.find_first_not_of(" "));
    names.push_back(var_name);
    start = next_comma+1;
    ctr++;
  }
  
  unsigned int num_names=names.size();
  for (int i=1;i<num_names;i++) {
  
    if (    i <= num_names-3
         && names[i].find("_X") != string::npos
         && names[i+1].find("_Y") != string::npos
         && names[i+2].find("_Z") != string::npos )

    { // XYZ value

      if (argc >= 3) {
        cout << "set title \"" << argv[2]
             << "\\n" << names[i] << " vs. Time\" font \"Helvetica,12\"" << endl;
      }

      cout << "set multiplot" << endl;
      cout << "set size 1.0,0.3" << endl;
      if (files.size()==1) { // Single file
        cout << "set origin 0.0,0.66" << endl;
        cout << "set xlabel \"\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \"Helvetica,10\"" << endl;
        cout << "set timestamp \"\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;

        cout << "set origin 0.0,0.33" << endl;
        cout << "set title \"\"" << endl;
        cout << "set ylabel \"" << names[i+1] << "\" font \"Helvetica,10\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << "\"" << endl;

        cout << "set origin 0.0,0.034" << endl;
        cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
        cout << "set ylabel \"" << names[i+2] << "\" font \"Helvetica,10\"" << endl;
        cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \"Helvetica,8\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << "\"" << endl;

      } else { // Multiple files, multiple plots per page

        // Plot 1 (top) X
        cout << "set origin 0.0,0.66" << endl;
        cout << "set xlabel \"\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \"Helvetica,10\"" << endl;
        cout << "set timestamp \"\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": 1" << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": " << f+1 << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": " << files.size() << "\"" << endl;

        // Plot 2 (middle) Y
        cout << "set origin 0.0,0.33" << endl;
        cout << "set title \"\"" << endl;
        cout << "set ylabel \"" << names[i+1] << "\" font \"Helvetica,10\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << ": 1" << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << ": " << f+1 << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << ": " << files.size() << "\"" << endl;

        // Plot 3 (bottom) Z
        cout << "set origin 0.0,0.034" << endl;
        cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
        cout << "set ylabel \"" << names[i+2] << "\" font \"Helvetica,10\"" << endl;
        cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \"Helvetica,8\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << ": 1" << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << ": " << f+1 << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << ": " << files.size() << "\"" << endl;
      }
      i += 3;
      cout << "unset multiplot" << endl;
      cout << "set size 1.0,1.0" << endl;
      cout << "set offset 0.0,0.0" << endl;

    } else { // Straight single value to plot

      if (argc >= 3) { // title added

        cout << "set title \"" << argv[2] 
             << "\\n" << names[i] << " vs. Time\" font \"Helvetica,12\"" << endl;
      }
      cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
      cout << "set ylabel \"" << names[i] << "\" font \"Helvetica,10\"" << endl;

      if (files.size()==1) { // Single file

        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;

      } else { // Multiple files

        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": 1" << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": " << f+1 << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << ": " << files.size() << "\"" << endl;
      }
    }
  }

}
