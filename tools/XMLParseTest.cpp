#include "FGXMLParse.h"
#include <iostream>
#include <fstream>

/*
g++ FGXMLParse.cpp XMLParseTest.cpp -oXMLParse -L/usr/local/lib -lsgxml -lsgstructure
*/

using namespace std;
using namespace JSBSim;

int main (int argc, char** argv)
{

  ifstream inputfile(argv[1]);
  if (!inputfile) {
    cerr << "Could not open XML file " << argv[1] << endl << endl;
    exit(-1);
  }
  FGXMLParse myXMLFile;
  readXML (inputfile, myXMLFile);
  myXMLFile.GetDocument()->Print();
}
