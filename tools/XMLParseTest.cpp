#include "FGXMLParse.h"
#include <iostream>
#include <fstream>

/*
g++ ../FGXMLParse.cpp XMLParseTest.cpp -I../ -oXMLParse -L/usr/local/lib -lsgxml -lsgstructure
*/

using namespace std;
using namespace JSBSim;

int main (int argc, char** argv)
{
  Element *element, *element1;
  ifstream inputfile(argv[1]);
  if (!inputfile) {
    cerr << "Could not open XML file " << argv[1] << endl << endl;
    exit(-1);
  }
  FGXMLParse myXMLFile;
  readXML (inputfile, myXMLFile);
  element1 = myXMLFile.GetDocument()->FindElement("AERODYNAMICS");
  while (element = element1->FindNextElement("AXIS")) {
    element->Print();
  }
  element1 = myXMLFile.GetDocument();
  element = element1->FindElement();
  while (element) {
    cout << "Element: " << element->GetName() << endl;
    element = element1->FindNextElement();
  }
}
