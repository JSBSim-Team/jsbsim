#include "FGXMLParse.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

/*
g++ ../FGXMLParse.cpp ../FGXMLElement.cpp XMLParseTest.cpp -I../ -oXMLParse -L/usr/local/lib -lsgxml -lsgstructure
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
