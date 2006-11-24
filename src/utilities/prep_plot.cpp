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
  ifstream infile;
  string filename(argv[1]), new_filename;
  char num[4];

  if (filename.find("#") != string::npos) { // if plotting multiple files
    while (file_ctr<10) {
      new_filename=filename;
      sprintf(num,"%d",file_ctr);
      new_filename.replace(new_filename.find("#"),1,num);
      infile.open(new_filename.c_str());
      if (!infile.is_open()) break;
      infile.close();
      files.push_back(new_filename);
      file_ctr++;
    }
  } else {
    files.push_back(filename);
  }
  
  infile.open(files[0].c_str());
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
    { // see if this is one of a set of standard XYZ values
      if (argc >= 3) {
        cout << "set title \"" << argv[2]
             << "\\n" << names[i].substr(2) << " vs. Time\" font \"Helvetica,12\"" << endl;
      }
      cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
      cout << "set ylabel \"" << names[i] << "\" font \"Helvetica,10\"" << endl;
      cout << "plot \"" << argv[1] << "\" using 1:" << i+1 << " with lines"
           << " title " << "\"" << names[i] << "\"" << endl;
    } else {
      if (argc >= 3) {
        cout << "set title \"" << argv[2] 
             << "\\n" << names[i] << " vs. Time\" font \"Helvetica,12\"" << endl;
      }
      cout << "set xlabel \"Time (sec)\" font \"Helvetica,10\"" << endl;
      cout << "set ylabel \"" << names[i] << "\" font \"Helvetica,10\"" << endl;

      if (files.size()==1) {
        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;
      } else {
        cout << "plot \"" << files[0] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        for (int f=1;f<files.size()-2;f++){
          cout << "\"" << files[f] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\",\\" << endl;
        }
        cout << "\"" << files[files.size()-1] << "\" using 1:" << i+1 << " with lines" << " title " << "\"" << names[i] << "\"" << endl;
      }
    }
  }

  

}
