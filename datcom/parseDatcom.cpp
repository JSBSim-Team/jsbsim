#include <iostream>
#include <string>
#include <fstream>
#include <vector>

// =============================================================================
// data definitions
// =============================================================================

using namespace std;

struct slots {
  int start;
  int length;
};

// The following data describes where specific columns start and how long they are
// so that the data can be grabbed from the output file. There are two kinds of
// data output I have seen (so far), a characteristics table and a dynamic derivative
// data table.
static struct slots Slots[][12] = {
  {{ 1, 6},{ 8, 8},{ 17, 8},{ 26, 9},{ 36,7},{44,8},{53,8},{62,12},{75,12},{88,12},{101,12},{114,12}},
  {{ 1, 8},{10,12},{23,12},{36,13},{50,12},{63,12},{76,12},{89,12},{102,12},{114,14},{0,0},{0,0}}
};

struct column {
   string name;
   vector <float> data;
};

struct characteristics {
   float mach;        // ND
   float altitude;    // ft
   float velocity;    // ft/sec
   float pressure;   // psf
   float temperature; // rankine
   float RN;         // Reynolds number 1/ft
   float ref_area;    // sqft
   float ref_latlen;  // ft
   float ref_lonlen;  // ft
   float mom_refhorz; // ft
   float mom_refvert; // ft
   struct column Columns[12];
};

// =============================================================================
// globals
// =============================================================================

vector <struct characteristics*> AllData;

// =============================================================================
// functions
// =============================================================================

struct characteristics* get_characteristics(ifstream& infile, int type)
{
  string line;
  char temp[180];
  int scratch;
  int dim[2] = {12,10};

  while (!infile.eof())
  {
    getline(infile, line);
    if (line[0] == '0') break;
  }

  struct characteristics* chars = new characteristics();
  strcpy(temp, line.c_str());
  sscanf(temp, "%d %f %f %f %E %f %E %f %f %f %f %f",
          &scratch,
          &chars->mach,
          &chars->altitude,
          &chars->velocity,
          &chars->pressure,
          &chars->temperature,
          &chars->RN,
          &chars->ref_area,
          &chars->ref_latlen,
          &chars->ref_lonlen,
          &chars->mom_refhorz,
          &chars->mom_refvert);

  getline(infile, line);
  getline(infile, line);

  strcpy(temp, line.c_str());

  int i=0;
  char temp_str[12][20];
  sscanf(temp, "%s %s %s %s %s %s %s %s %s %s %s %s %s",
         &scratch,
         temp_str[0],
         temp_str[1],
         temp_str[2],
         temp_str[3],
         temp_str[4],
         temp_str[5],
         temp_str[6],
         temp_str[7],
         temp_str[8],
         temp_str[9],
         temp_str[10],
         temp_str[11]);

  for (int i=0; i<12; i++)
    chars->Columns[i].name = temp_str[i];

  getline(infile, line);
  getline(infile, line);

  while (line[0] != '0') {
    for (int i=0;i<dim[type];i++) {
      int starts = Slots[type][i].start;
      int length = Slots[type][i].length;
//      cout << "Start: " << starts << " length: " << length;
      string str = line.substr(starts, length);
//      cout << " string: " << str;
      double num = atof(str.c_str());
//      cout << " Num: " << num << endl;
      chars->Columns[i].data.push_back(num);
    }
    getline(infile, line);
  }

  return chars;
}

// =============================================================================

void report(string which)
{
  int column = -1, type = -1;

  for (int col=0; col<12; col++) {
    cout << "Target: " << which << "  this col: " << AllData[0]->Columns[col].name << endl;
    if (AllData[0]->Columns[col].name == which) {
      type = 0;
      column = col;
      break;
    } else if (AllData[1]->Columns[col].name == which) {
      type = 1;
      column = col;
      break;
    }
  }
  if (column < 0) {
    cout << "Could not find data for column: " << which << endl;
  } else {
    cout << "======================================" << endl;
    cout << "Report for " << which << endl << endl;
    for (int col=0+type; col<AllData.size(); col+=2) {
      cout << "  " << AllData[col]->RN;
    }
    cout << endl;
    for (int ctr=0; ctr<AllData[0]->Columns[column].data.size(); ctr++) {
      cout << " " << AllData[0]->Columns[0].data[ctr];
      for (int col=0+type; col<AllData.size(); col+=2) {
        cout << " " << AllData[col]->Columns[column].data[ctr];
      }
      cout << endl;
    }
  }
}

// =============================================================================
//  program start
// =============================================================================

int main(int argc, char **argv)
{
   string line;
   const int not_found = string::npos;

   if (argc != 2) {
     cout << "\n Usage: parseDatcom <filename>\n\n";
     return -1;
   }

   ifstream infile(argv[1]);

   while (!infile.eof())
   {
     getline(infile, line);

     if (line.find("CHARACTERISTICS") != not_found) {
       AllData.push_back(get_characteristics(infile, 0));
     } else if (line.find("DYNAMIC") != not_found) {
       AllData.push_back(get_characteristics(infile, 1));
     }
   }

// data is now all read in

  report("CD");
  report("CL");

   return 0;
}
