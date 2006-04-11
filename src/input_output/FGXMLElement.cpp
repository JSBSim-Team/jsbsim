/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Author:       Jon Berndt
 Date started: 09/28/2004
 Purpose:      XML element class
 Called by:    FGXMLParse

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLElement.h"
#ifdef FGFS
#  ifndef __BORLANDC__
#    include <simgear/compiler.h>
#  endif
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#    include <cstdlib>
#  else
#    include <math.h>
#    include <stdlib.h>
#  endif
#else
#  if defined (sgi) && !defined(__GNUC__)
#    include <math.h>
#    include <stdlib.h>
#  else
#    include <cmath>
#    include <cstdlib>
#  endif
#endif

#include <stdlib.h>
#include <math.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

static const char *IdSrc = "$Id: FGXMLElement.cpp,v 1.11 2006/04/11 09:13:09 jberndt Exp $";
static const char *IdHdr = ID_XMLELEMENT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Element::Element(string nm)
{
  name   = nm;
  parent = 0L;
  element_index = 0;

  // convert ["from"]["to"] = factor, so: from * factor = to
  convert["M"]["FT"] = 3.2808399;
  convert["FT"]["M"] = 1.0/convert["M"]["FT"];
  convert["M2"]["FT2"] = convert["M"]["FT"]*convert["M"]["FT"];
  convert["FT2"]["M2"] = 1.0/convert["M2"]["FT2"];
  convert["FT"]["IN"] = 12.0;
  convert["IN"]["FT"] = 1.0/convert["FT"]["IN"];
  convert["LBS"]["KG"] = 0.45359237;
  convert["KG"]["LBS"] = 1.0/convert["LBS"]["KG"];
  convert["SLUG*FT2"]["KG*M2"] = 1.35594;
  convert["KG*M2"]["SLUG*FT2"] = 1.0/convert["SLUG*FT2"]["KG*M2"];
  convert["RAD"]["DEG"] = 360.0/(2.0*3.1415926);
  convert["DEG"]["RAD"] = 1.0/convert["RAD"]["DEG"];
  convert["LBS/FT"]["N/M"] = 14.5939;
  convert["LBS/FT/SEC"]["N/M/SEC"] = 14.5939;
  convert["N/M"]["LBS/FT"] = 1.0/convert["LBS/FT"]["N/M"];
  convert["N/M/SEC"]["LBS/FT/SEC"] = 1.0/convert["LBS/FT/SEC"]["N/M/SEC"];
  convert["WATTS"]["HP"] = 0.001341022;
  convert["HP"]["WATTS"] = 1.0/convert["WATTS"]["HP"];
  convert["N"]["LBS"] = 0.22482;
  convert["LBS"]["N"] = 1.0/convert["N"]["LBS"];
  convert["KTS"]["FT/SEC"] = 1.68781;
  convert["FT/SEC"]["KTS"] = 1.0/convert["KTS"]["FT/SEC"];
  convert["FT*LBS"]["N*M"] = 1.35581795;
  convert["N*M"]["FT*LBS"] = 1/convert["FT*LBS"]["N*M"];
  convert["IN"]["M"] = convert["IN"]["FT"] * convert["FT"]["M"];
  convert["M"]["IN"] = convert["M"]["FT"] * convert["FT"]["IN"];

  convert["M"]["M"] = 1.00;
  convert["FT"]["FT"] = 1.00;
  convert["IN"]["IN"] = 1.00;
  convert["IN3"]["IN3"] = 1.00;
  convert["DEG"]["DEG"] = 1.00;
  convert["RAD"]["RAD"] = 1.00;
  convert["M2"]["M2"] = 1.00;
  convert["FT2"]["FT2"] = 1.00;
  convert["KG*M2"]["KG*M2"] = 1.00;
  convert["SLUG*FT2"]["SLUG*FT2"] = 1.00;
  convert["KG"]["KG"] = 1.00;
  convert["LBS"]["LBS"] = 1.00;
  convert["LBS/FT"]["LBS/FT"] = 1.00;
  convert["LBS/SEC"]["LBS/SEC"] = 1.00;
  convert["LBS/FT/SEC"]["LBS/FT/SEC"] = 1.00;
  convert["N/M"]["N/M"] = 1.00;
  convert["N/M/SEC"]["N/M/SEC"] = 1.00;
  convert["PSI"]["PSI"] = 1.00;
  convert["PSF"]["PSF"] = 1.00;
  convert["INHG"]["INHG"] = 1.00;
  convert["HP"]["HP"] = 1.00;
  convert["N"]["N"] = 1.00;
  convert["WATTS"]["WATTS"] = 1.00;
  convert["LBS/SEC"]["LBS/SEC"] = 1.00;
  convert["FT/SEC"]["FT/SEC"] = 1.00;
  convert["KTS"]["KTS"] = 1.00;
  convert["FT*LBS"]["FT*LBS"] = 1.00;
  convert["N*M"]["N*M"] = 1.00;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element::~Element(void)
{
  for (int i=0; i<children.size(); i++) delete children[i];
  data_lines.clear();
  attributes.clear();
  attribute_key.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::GetAttributeValue(string attr)
{
  int select=-1;
  for (int i=0; i<attribute_key.size(); i++) {
    if (attribute_key[i] == attr) select = i;
  }
  if (select < 0) return string("");
  else return attributes[attr];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetAttributeValueAsNumber(string attr)
{
  string attribute = GetAttributeValue(attr);

  if (attribute.empty()) return 99e99;
  else return (atof(attribute.c_str()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element* Element::GetElement(int el)
{
  if (children.size() > el) {
    element_index = el;
    return children[el];
  }
  else {
    element_index = 0;
    return 0L;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element* Element::GetNextElement(void)
{
  if (children.size() > element_index+1) {
    element_index++;
    return children[element_index];
  } else {
    element_index = 0;
    return 0L;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::GetDataLine(int i)
{
  if (data_lines.size() > 0) return data_lines[i];
  else return string("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetDataAsNumber(void)
{
  if (data_lines.size() == 1) {
    return atof(data_lines[0].c_str());
  } else {
    return 99e99;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int Element::GetNumElements(string element_name)
{
  int number_of_elements=0;
  Element* el=FindElement(element_name);
  while (el) {
    number_of_elements++;
    el=FindNextElement(element_name);
  }
  return number_of_elements;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element* Element::FindElement(string el)
{
  if (el.empty() && children.size() >= 1) {
    element_index = 1;
    return children[0];
  }
  for (int i=0; i<children.size(); i++) {
    if (el == children[i]->GetName()) {
      element_index = i+1;
      return children[i];
    }
  }
  element_index = 0;
  return 0L;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element* Element::FindNextElement(string el)
{
  if (el.empty()) {
    if (element_index < children.size()) {
      return children[element_index++];
    } else {
      element_index = 0;
      return 0L;
    }
  }
  for (int i=element_index; i<children.size(); i++) {
    if (el == children[i]->GetName()) {
      element_index = i+1;
      return children[i];
    }
  }
  element_index = 0;
  return 0L;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumber(string el)
{
  Element* element = FindElement(el);
  if (element) {
    return element->GetDataAsNumber();
  } else {
    return 99e99;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::FindElementValue(string el)
{
  Element* element = FindElement(el);
  if (element) {
    return element->GetDataLine();
  } else {
    return "";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumberConvertTo(string el, string target_units)
{
  Element* element = FindElement(el);
  double value;
  string supplied_units="";

  if (element) {
     value = element->GetDataAsNumber();
     supplied_units = element->GetAttributeValue("unit");
     if (!supplied_units.empty()) {
       if (convert.find(supplied_units) != convert.end()) {
         if (convert[supplied_units].find(target_units) != convert[supplied_units].end()) {
           value *= convert[supplied_units][target_units];
         } else {
           cerr << endl << "Target unit: \"" << target_units << "\" does not exist (typo?). Add new unit"
                << " conversion in FGXMLElement.cpp." << endl;
           exit(-1);
         }
       } else {
         cerr << endl << "Supplied unit: \"" << supplied_units << "\" does not exist (typo?). Add new unit"
              << " conversion in FGXMLElement.cpp." << endl;
         exit(-1);
       }
     }
  } else {
    return 99e99;
  }
  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumberConvertFromTo( string el,
                                                       string supplied_units,
                                                       string target_units)
{
  Element* element = FindElement(el);
  double value;

  if (element) {
     value = element->GetDataAsNumber();
     if (!supplied_units.empty()) {
       if (convert.find(supplied_units) != convert.end()) {
         if (convert[supplied_units].find(target_units) != convert[supplied_units].end()) {
           value *= convert[supplied_units][target_units];
         } else {
           cerr << endl << "Target unit: \"" << target_units << "\" does not exist (typo?). Add new unit"
                << " conversion in FGXMLElement.cpp." << endl;
           exit(-1);
         }
       } else {
         cerr << endl << "Supplied unit: \"" << supplied_units << "\" does not exist (typo?). Add new unit"
              << " conversion in FGXMLElement.cpp." << endl;
         exit(-1);
       }
     }
  } else {
    return 99e99;
  }
  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 Element::FindElementTripletConvertTo( string target_units)
{
  FGColumnVector3 triplet;
  Element* item;
  double value=0.0;
  string supplied_units = GetAttributeValue("unit");

  item = FindElement("x");
  if (!item) item = FindElement("roll");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
  } else {
    value = 0.0;
    cerr << "Could not find an X triplet item for this column vector." << endl;
  }
  triplet(1) = value;

  item = FindElement("y");
  if (!item) item = FindElement("pitch");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
  } else {
    value = 0.0;
    cerr << "Could not find a Y triplet item for this column vector." << endl;
  }
  triplet(2) = value;

  item = FindElement("z");
  if (!item) item = FindElement("yaw");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
  } else {
    value = 0.0;
    cerr << "Could not find a Z triplet item for this column vector." << endl;
  }
  triplet(3) = value;

  return triplet;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::Print(int level)
{
  int i, spaces;

  level+=2;
  for (spaces=0; spaces<=level; spaces++) cout << " "; // format output
  cout << "Element Name: " << name;
  for (i=0; i<attributes.size(); i++) {
    cout << "  " << attribute_key[i] << " = " << attributes[attribute_key[i]];
  }
  cout << endl;
  for (i=0; i<data_lines.size(); i++) {
    for (spaces=0; spaces<=level; spaces++) cout << " "; // format output
    cout << data_lines[i] << endl;
  }
  for (i=0; i<children.size(); i++) {
    children[i]->Print(level);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::AddAttribute(string name, string value)
{
  attribute_key.push_back(name);
  attributes[name] = value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::AddData(string d)
{
  int string_start = d.find_first_not_of(" \t");
  if (string_start > 0) {
    d.erase(0,string_start);
  }
  data_lines.push_back(d);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // end namespace JSBSim
