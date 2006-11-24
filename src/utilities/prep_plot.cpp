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
        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;

        // Plot 2 (middle) Y
        cout << "set origin 0.0,0.33" << endl;
        cout << "set title \"\"" << endl;
        cout << "set ylabel \"" << names[i+1] << "\" font \"Helvetica,10\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+2 << " with lines" << " title " << "\"" << names[i+1] << "\"" << endl;

        // Plot 3 (bottom) Z
        cout << "set origin 0.0,0.034" << endl;
        cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
        cout << "set ylabel \"" << names[i+2] << "\" font \"Helvetica,10\"" << endl;
        cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \"Helvetica,8\"" << endl;
        cout << "plot \"" << files[0] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+3 << " with lines" << " title " << "\"" << names[i+2] << "\"" << endl;
      }
      i += 3;
      cout << "unset multiplot" << endl;
      cout << "set size -1.0,-1.0" << endl;
      cout << "set offset -0.0,-0.0" << endl;

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

        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;
      }
    }
  }

}
