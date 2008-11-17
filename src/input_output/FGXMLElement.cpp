/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Author:       Jon Berndt
 Date started: 09/28/2004
 Purpose:      XML element class
 Called by:    FGXMLParse

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLElement.h"

#include <cmath>
#include <cstdlib>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

static const char *IdSrc = "$Id: FGXMLElement.cpp,v 1.24 2008/11/17 12:21:07 jberndt Exp $";
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
  // Length
  convert["M"]["FT"] = 3.2808399;
  convert["FT"]["M"] = 1.0/convert["M"]["FT"];
  convert["FT"]["IN"] = 12.0;
  convert["IN"]["FT"] = 1.0/convert["FT"]["IN"];
  convert["IN"]["M"] = convert["IN"]["FT"] * convert["FT"]["M"];
  convert["M"]["IN"] = convert["M"]["FT"] * convert["FT"]["IN"];
  // Area
  convert["M2"]["FT2"] = convert["M"]["FT"]*convert["M"]["FT"];
  convert["FT2"]["M2"] = 1.0/convert["M2"]["FT2"];
  convert["M2"]["IN2"] = convert["M"]["IN"]*convert["M"]["IN"];
  convert["IN2"]["M2"] = 1.0/convert["M2"]["IN2"];
  convert["FT2"]["IN2"] = 144.0;
  convert["IN2"]["FT2"] = 1.0/convert["FT2"]["IN2"];
  // Volume
  convert["IN3"]["CC"] = 16.387064;
  convert["CC"]["IN3"] = 1.0/convert["IN3"]["CC"];
  convert["FT3"]["IN3"] = 1728.0;
  convert["IN3"]["FT3"] = 1.0/convert["FT3"]["IN3"];
  convert["M3"]["FT3"] = 35.3146667;
  convert["FT3"]["M3"] = 1.0/convert["M3"]["FT3"];
  convert["LTR"]["IN3"] = 61.0237441;
  convert["IN3"]["LTR"] = 1.0/convert["LTR"]["IN3"];
  // Mass & Weight
  convert["LBS"]["KG"] = 0.45359237;
  convert["KG"]["LBS"] = 1.0/convert["LBS"]["KG"];
  convert["SLUG"]["KG"] = 14.59390;
  convert["KG"]["SLUG"] = 1.0/convert["SLUG"]["KG"];
  // Moments of Inertia
  convert["SLUG*FT2"]["KG*M2"] = 1.35594;
  convert["KG*M2"]["SLUG*FT2"] = 1.0/convert["SLUG*FT2"]["KG*M2"];
  // Angles
  convert["RAD"]["DEG"] = 360.0/(2.0*3.1415926);
  convert["DEG"]["RAD"] = 1.0/convert["RAD"]["DEG"];
  // Spring force
  convert["LBS/FT"]["N/M"] = 14.5939;
  convert["N/M"]["LBS/FT"] = 1.0/convert["LBS/FT"]["N/M"];
  // Damping force
  convert["LBS/FT/SEC"]["N/M/SEC"] = 14.5939;
  convert["N/M/SEC"]["LBS/FT/SEC"] = 1.0/convert["LBS/FT/SEC"]["N/M/SEC"];
  // Damping force (Square Law)
  convert["LBS/FT2/SEC2"]["N/M2/SEC2"] = 47.880259;
  convert["N/M2/SEC2"]["LBS/FT2/SEC2"] = 1.0/convert["LBS/FT2/SEC2"]["N/M2/SEC2"];
  // Power
  convert["WATTS"]["HP"] = 0.001341022;
  convert["HP"]["WATTS"] = 1.0/convert["WATTS"]["HP"];
  // Force
  convert["N"]["LBS"] = 0.22482;
  convert["LBS"]["N"] = 1.0/convert["N"]["LBS"];
  // Velocity
  convert["KTS"]["FT/SEC"] = 1.68781;
  convert["FT/SEC"]["KTS"] = 1.0/convert["KTS"]["FT/SEC"];
  convert["M/S"]["FT/S"] = 3.2808399;
  convert["FT/S"]["M/S"] = 1.0/convert["M/S"]["FT/S"];
  // Torque
  convert["FT*LBS"]["N*M"] = 1.35581795;
  convert["N*M"]["FT*LBS"] = 1/convert["FT*LBS"]["N*M"];
  // Valve
  convert["M4*SEC/KG"]["FT4*SEC/SLUG"] = convert["M"]["FT"]*convert["M"]["FT"]*
    convert["M"]["FT"]*convert["M"]["FT"]/convert["KG"]["SLUG"];
  convert["FT4*SEC/SLUG"]["M4*SEC/KG"] =
    1.0/convert["M4*SEC/KG"]["FT4*SEC/SLUG"];
  // Pressure
  convert["INHG"]["PSF"] = 70.7180803;
  convert["PSF"]["INHG"] = 1.0/convert["INHG"]["PSF"];
  convert["ATM"]["INHG"] = 29.9246899;
  convert["INHG"]["ATM"] = 1.0/convert["ATM"]["INHG"];
  convert["PSI"]["INHG"] = 2.03625437;
  convert["INHG"]["PSI"] = 1.0/convert["PSI"]["INHG"];
  convert["INHG"]["PA"] = 3386.0; // inches Mercury to pascals
  convert["PA"]["INHG"] = 1.0/convert["INHG"]["PA"];
  convert["LBS/FT2"]["N/M2"] = 14.5939/convert["FT"]["M"];
  convert["N/M2"]["LBS/FT2"] = 1.0/convert["LBS/FT2"]["N/M2"];
  convert["LBS/FT2"]["PA"] = convert["LBS/FT2"]["N/M2"];
  convert["PA"]["LBS/FT2"] = 1.0/convert["LBS/FT2"]["PA"];
  // Mass flow
  convert["KG/MIN"]["LBS/MIN"] = convert["KG"]["LBS"];
  // Fuel Consumption
  convert["LBS/HP*HR"]["KG/KW*HR"] = 0.6083;
  convert["KG/KW*HR"]["LBS/HP*HR"] = 1.0/convert["LBS/HP*HR"]["KG/KW*HR"];

  // Length
  convert["M"]["M"] = 1.00;
  convert["FT"]["FT"] = 1.00;
  convert["IN"]["IN"] = 1.00;
  // Area
  convert["M2"]["M2"] = 1.00;
  convert["FT2"]["FT2"] = 1.00;
  // Volume
  convert["IN3"]["IN3"] = 1.00;
  convert["CC"]["CC"] = 1.0;
  convert["M3"]["M3"] = 1.0;
  convert["FT3"]["FT3"] = 1.0;
  convert["LTR"]["LTR"] = 1.0;
  // Mass & Weight
  convert["KG"]["KG"] = 1.00;
  convert["LBS"]["LBS"] = 1.00;
  // Moments of Inertia
  convert["KG*M2"]["KG*M2"] = 1.00;
  convert["SLUG*FT2"]["SLUG*FT2"] = 1.00;
  // Angles
  convert["DEG"]["DEG"] = 1.00;
  convert["RAD"]["RAD"] = 1.00;
  // Spring force
  convert["LBS/FT"]["LBS/FT"] = 1.00;
  convert["N/M"]["N/M"] = 1.00;
  // Damping force
  convert["LBS/FT/SEC"]["LBS/FT/SEC"] = 1.00;
  convert["N/M/SEC"]["N/M/SEC"] = 1.00;
  // Damping force (Square law)
  convert["LBS/FT2/SEC2"]["LBS/FT2/SEC2"] = 1.00;
  convert["N/M2/SEC2"]["N/M2/SEC2"] = 1.00;
  // Power
  convert["HP"]["HP"] = 1.00;
  convert["WATTS"]["WATTS"] = 1.00;
  // Force
  convert["N"]["N"] = 1.00;
  // Velocity
  convert["FT/SEC"]["FT/SEC"] = 1.00;
  convert["KTS"]["KTS"] = 1.00;
  convert["M/S"]["M/S"] = 1.0;
  // Torque
  convert["FT*LBS"]["FT*LBS"] = 1.00;
  convert["N*M"]["N*M"] = 1.00;
  // Valve
  convert["M4*SEC/KG"]["M4*SEC/KG"] = 1.0;
  convert["FT4*SEC/SLUG"]["FT4*SEC/SLUG"] = 1.0;
  // Pressure
  convert["PSI"]["PSI"] = 1.00;
  convert["PSF"]["PSF"] = 1.00;
  convert["INHG"]["INHG"] = 1.00;
  convert["ATM"]["ATM"] = 1.0;
  convert["PA"]["PA"] = 1.0;
  convert["N/M2"]["N/M2"] = 1.00;
  convert["LBS/FT2"]["LBS/FT2"] = 1.00;
  // Mass flow
  convert["LBS/SEC"]["LBS/SEC"] = 1.00;
  convert["KG/MIN"]["KG/MIN"] = 1.0;
  convert["LBS/MIN"]["LBS/MIN"] = 1.0;
  // Fuel Consumption
  convert["LBS/HP*HR"]["LBS/HP*HR"] = 1.0;
  convert["KG/KW*HR"]["KG/KW*HR"] = 1.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element::~Element(void)
{
  for (unsigned int i=0; i<children.size(); i++) delete children[i];
  data_lines.clear();
  attributes.clear();
  attribute_key.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::GetAttributeValue(string attr)
{
  int select=-1;
  for (unsigned int i=0; i<attribute_key.size(); i++) {
    if (attribute_key[i] == attr) select = i;
  }
  if (select < 0) return string("");
  else return attributes[attr];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetAttributeValueAsNumber(string attr)
{
  string attribute = GetAttributeValue(attr);

  if (attribute.empty()) return HUGE_VAL;
  else return (atof(attribute.c_str()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element* Element::GetElement(unsigned int el)
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

string Element::GetDataLine(unsigned int i)
{
  if (data_lines.size() > 0) return data_lines[i];
  else return string("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetDataAsNumber(void)
{
  if (data_lines.size() == 1) {
    return atof(data_lines[0].c_str());
  } else if (data_lines.size() == 0) {
    return HUGE_VAL;
  } else {
    cerr << "Attempting to get single data value from multiple lines in element " << name << endl;
    return HUGE_VAL;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned int Element::GetNumElements(string element_name)
{
  unsigned int number_of_elements=0;
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
  for (unsigned int i=0; i<children.size(); i++) {
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
  for (unsigned int i=element_index; i<children.size(); i++) {
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
    cerr << "Attempting to get single data value from multiple lines" << endl;
    return 0;
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

  if (!element) {
    cerr << "Attempting to get non-existent element " << el << endl;
    exit(0);
  }

  string supplied_units = element->GetAttributeValue("unit");

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" does not exist (typo?). Add new unit"
           << " conversion in FGXMLElement.cpp." << endl;
      exit(-1);
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" cannot be converted to "
                   << target_units << ". Add new unit conversion in FGXMLElement.cpp or fix typo" << endl;
      exit(-1);
    }
  }

  double value = element->GetDataAsNumber();
  if (!supplied_units.empty()) {
    value *= convert[supplied_units][target_units];
  }

  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumberConvertFromTo( string el,
                                                       string supplied_units,
                                                       string target_units)
{
  Element* element = FindElement(el);

  if (!element) {
    cerr << "Attempting to get non-existent element " << el << endl;
    exit(0);
  }

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" does not exist (typo?). Add new unit"
           << " conversion in FGXMLElement.cpp." << endl;
      exit(-1);
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" cannot be converted to "
                   << target_units << ". Add new unit conversion in FGXMLElement.cpp or fix typo" << endl;
      exit(-1);
    }
  }

  double value = element->GetDataAsNumber();
  if (!supplied_units.empty()) {
    value *= convert[supplied_units][target_units];
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

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" does not exist (typo?). Add new unit"
           << " conversion in FGXMLElement.cpp." << endl;
      exit(-1);
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      cerr << endl << "Supplied unit: \"" << supplied_units << "\" cannot be converted to "
                   << target_units << ". Add new unit conversion in FGXMLElement.cpp or fix typo" << endl;
      exit(-1);
    }
  }

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

void Element::Print(unsigned int level)
{
  unsigned int i, spaces;

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
  unsigned int string_start = (unsigned int)d.find_first_not_of(" \t");
  if (string_start > 0) {
    d.erase(0,string_start);
  }
  data_lines.push_back(d);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // end namespace JSBSim
