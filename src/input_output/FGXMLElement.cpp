/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Author:       Jon Berndt
 Date started: 09/28/2004
 Purpose:      XML element class
 Called by:    FGXMLParse

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>  // for assembling the error messages / what of exceptions.
#include <stdexcept>  // using domain_error, invalid_argument, and length_error.

#include "FGXMLElement.h"
#include "FGJSBBase.h"
#include "input_output/string_utilities.h"

using namespace std;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

bool Element::converterIsInitialized = false;
map <string, map <string, double> > Element::convert;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Element::Element(const string& nm)
{
  name   = nm;
  parent = 0L;
  element_index = 0;
  line_number = -1;

  if (!converterIsInitialized) {
    converterIsInitialized = true;
    // convert ["from"]["to"] = factor, so: from * factor = to
    // Length
    convert["M"]["FT"] = 3.2808399;
    convert["FT"]["M"] = 1.0/convert["M"]["FT"];
    convert["CM"]["FT"] = 0.032808399;
    convert["FT"]["CM"] = 1.0/convert["CM"]["FT"];
    convert["KM"]["FT"] = 3280.8399;
    convert["FT"]["KM"] = 1.0/convert["KM"]["FT"];
    convert["FT"]["IN"] = 12.0;
    convert["IN"]["FT"] = 1.0/convert["FT"]["IN"];
    convert["IN"]["M"] = convert["IN"]["FT"] * convert["FT"]["M"];
    convert["M"]["IN"] = convert["M"]["FT"] * convert["FT"]["IN"];
    // Area
    convert["M2"]["FT2"] = convert["M"]["FT"]*convert["M"]["FT"];
    convert["FT2"]["M2"] = 1.0/convert["M2"]["FT2"];
    convert["CM2"]["FT2"] = convert["CM"]["FT"]*convert["CM"]["FT"];
    convert["FT2"]["CM2"] = 1.0/convert["CM2"]["FT2"];
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
    convert["GAL"]["FT3"] = 0.133681;
    convert["FT3"]["GAL"] = 1.0/convert["GAL"]["FT3"];
    convert["IN3"]["GAL"] = convert["IN3"]["FT3"]*convert["FT3"]["GAL"];
    convert["LTR"]["GAL"] = convert["LTR"]["IN3"]*convert["IN3"]["GAL"];
    convert["M3"]["GAL"] = 1000.*convert["LTR"]["GAL"];
    convert["CC"]["GAL"] = convert["CC"]["IN3"]*convert["IN3"]["GAL"];
    // Mass & Weight
    convert["LBS"]["KG"] = 0.45359237;
    convert["KG"]["LBS"] = 1.0/convert["LBS"]["KG"];
    convert["SLUG"]["KG"] = 14.59390;
    convert["KG"]["SLUG"] = 1.0/convert["SLUG"]["KG"];
    // Moments of Inertia
    convert["SLUG*FT2"]["KG*M2"] = 1.35594;
    convert["KG*M2"]["SLUG*FT2"] = 1.0/convert["SLUG*FT2"]["KG*M2"];
    // Angles
    convert["RAD"]["DEG"] = 180.0/M_PI;
    convert["DEG"]["RAD"] = 1.0/convert["RAD"]["DEG"];
    // Angular rates
    convert["RAD/SEC"]["DEG/SEC"] = convert["RAD"]["DEG"];
    convert["DEG/SEC"]["RAD/SEC"] = 1.0/convert["RAD/SEC"]["DEG/SEC"];
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
    convert["KTS"]["FT/SEC"] = 1.6878098571;
    convert["FT/SEC"]["KTS"] = 1.0/convert["KTS"]["FT/SEC"];
    convert["M/S"]["FT/S"] = 3.2808399;
    convert["M/S"]["KTS"] = convert["M/S"]["FT/S"]/convert["KTS"]["FT/SEC"];
    convert["M/SEC"]["FT/SEC"] = 3.2808399;
    convert["FT/S"]["M/S"] = 1.0/convert["M/S"]["FT/S"];
    convert["M/SEC"]["FT/SEC"] = 3.2808399;
    convert["FT/SEC"]["M/SEC"] = 1.0/convert["M/SEC"]["FT/SEC"];
    convert["KM/SEC"]["FT/SEC"] = 3280.8399;
    convert["FT/SEC"]["KM/SEC"] = 1.0/convert["KM/SEC"]["FT/SEC"];
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
    convert["KG/SEC"]["LBS/SEC"] = convert["KG"]["LBS"];
    convert ["N/SEC"]["LBS/SEC"] = 0.224808943;
    convert ["LBS/SEC"]["N/SEC"] = 1.0/convert ["N/SEC"]["LBS/SEC"];
    // Fuel Consumption
    convert["LBS/HP*HR"]["KG/KW*HR"] = 0.6083;
    convert["KG/KW*HR"]["LBS/HP*HR"] = 1.0/convert["LBS/HP*HR"]["KG/KW*HR"];
    // Density
    convert["KG/L"]["LBS/GAL"] = 8.3454045;
    convert["LBS/GAL"]["KG/L"] = 1.0/convert["KG/L"]["LBS/GAL"];
    // Gravitational
    convert["FT3/SEC2"]["M3/SEC2"] = convert["FT3"]["M3"];
    convert["M3/SEC2"]["FT3/SEC2"] = convert["M3"]["FT3"];

    // Length
    convert["M"]["M"] = 1.00;
    convert["KM"]["KM"] = 1.00;
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
    convert["GAL"]["GAL"] = 1.0;
    // Mass & Weight
    convert["KG"]["KG"] = 1.00;
    convert["LBS"]["LBS"] = 1.00;
    // Moments of Inertia
    convert["KG*M2"]["KG*M2"] = 1.00;
    convert["SLUG*FT2"]["SLUG*FT2"] = 1.00;
    // Angles
    convert["DEG"]["DEG"] = 1.00;
    convert["RAD"]["RAD"] = 1.00;
    // Angular rates
    convert["DEG/SEC"]["DEG/SEC"] = 1.00;
    convert["RAD/SEC"]["RAD/SEC"] = 1.00;
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
    convert["M/SEC"]["M/SEC"] = 1.0;
    convert["KM/SEC"]["KM/SEC"] = 1.0;
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
    convert["N/SEC"]["N/SEC"] = 1.0;
    // Fuel Consumption
    convert["LBS/HP*HR"]["LBS/HP*HR"] = 1.0;
    convert["KG/KW*HR"]["KG/KW*HR"] = 1.0;
    // Density
    convert["KG/L"]["KG/L"] = 1.0;
    convert["LBS/GAL"]["LBS/GAL"] = 1.0;
    // Gravitational
    convert["FT3/SEC2"]["FT3/SEC2"] = 1.0;
    convert["M3/SEC2"]["M3/SEC2"] = 1.0;
    // Electrical
    convert["VOLTS"]["VOLTS"] = 1.0;
    convert["OHMS"]["OHMS"] = 1.0;
    convert["AMPERES"]["AMPERES"] = 1.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Element::~Element(void)
{
  for (unsigned int i = 0; i < children.size(); ++i)
    children[i]->SetParent(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::GetAttributeValue(const string& attr)
{
  if (HasAttribute(attr))  return attributes[attr];
  else                     return ("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Element::SetAttributeValue(const std::string& key, const std::string& value)
{
  bool ret = HasAttribute(key);
  if (ret)
    attributes[key] = value;

  return ret;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetAttributeValueAsNumber(const string& attr)
{
  string attribute = GetAttributeValue(attr);

  if (attribute.empty()) {
    std::stringstream s;
    s << ReadFrom() << "Expecting numeric attribute value, but got no data";
    cerr << s.str() << endl;
    throw length_error(s.str());
  }
  else {
    double number=0;
    try {
      number = atof_locale_c(attribute);
    } catch (InvalidNumber& e) {
      std::stringstream s;
      s << ReadFrom() << e.what();
      cerr << s.str() << endl;
      throw BaseException(s.str());
    }

    return (number);
  }
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
  if (!data_lines.empty()) return data_lines[i];
  else return string("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetDataAsNumber(void)
{
  if (data_lines.size() == 1) {
    double number=0;
    try {
      number = atof_locale_c(data_lines[0]);
    } catch (InvalidNumber& e) {
      std::stringstream s;
      s << ReadFrom() << e.what();
      cerr << s.str() << endl;
      throw BaseException(s.str());
    }

    return number;
  } else if (data_lines.empty()) {
    std::stringstream s;
    s << ReadFrom() << "Expected numeric value, but got no data";
    cerr << s.str() << endl;
    throw length_error(s.str());
  } else {
    cerr << ReadFrom() << "Attempting to get single data value in element "
         << "<" << name << ">" << endl
         << " from multiple lines:" << endl;
    for(unsigned int i=0; i<data_lines.size(); ++i)
      cerr << data_lines[i] << endl;
    std::stringstream s;
    s << ReadFrom() << "Attempting to get single data value in element "
      << "<" << name << ">"
      << " from multiple lines (" << data_lines.size() << ").";
    throw length_error(s.str());
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned int Element::GetNumElements(const string& element_name)
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

Element* Element::FindElement(const string& el)
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

Element* Element::FindNextElement(const string& el)
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

double Element::FindElementValueAsNumber(const string& el)
{
  Element* element = FindElement(el);
  if (element) {
    double value = element->GetDataAsNumber();
    value = DisperseValue(element, value);
    return value;
  } else {
    std::stringstream s;
    s << ReadFrom() << "Attempting to get non-existent element " << el;
    cerr << s.str() << endl;
    throw length_error(s.str());
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Element::FindElementValueAsBoolean(const string& el)
{
  Element* element = FindElement(el);
  if (element) {
    // check value as an ordinary number
    double value = element->GetDataAsNumber();

    // now check how it should return data
    if (value == 0) {
      return false;
    } else {
      return true;
    }
  } else {
    cerr << ReadFrom() << "Attempting to get non-existent element " << el << " ;returning false"
         << endl;
    return false;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::FindElementValue(const string& el)
{
  Element* element = FindElement(el);
  if (element) {
    return element->GetDataLine();
  } else {
    return "";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumberConvertTo(const string& el, const string& target_units)
{
  Element* element = FindElement(el);

  if (!element) {
    std::stringstream s;
    s << ReadFrom() << "Attempting to get non-existent element " << el;
    cerr << s.str() << endl;
    throw length_error(s.str());
  }

  string supplied_units = element->GetAttributeValue("unit");

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      std::stringstream s;
      s << element->ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" does not exist (typo?).";
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      std::stringstream s;
      s << element->ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" cannot be converted to " << target_units;
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
  }

  double value = element->GetDataAsNumber();

  // Sanity check for angular values
  if ((supplied_units == "RAD") && (fabs(value) > 2 * M_PI)) {
    cerr << element->ReadFrom() << element->GetName() << " value "
         << value << " RAD is outside the range [ -2*M_PI RAD ; +2*M_PI RAD ]"
         << endl;
  }
  if ((supplied_units == "DEG") && (fabs(value) > 360.0)) {
    cerr << element->ReadFrom() << element->GetName() << " value "
         << value << " DEG is outside the range [ -360 DEG ; +360 DEG ]"
         << endl;
  }


  if (!supplied_units.empty()) {
    value *= convert[supplied_units][target_units];
  }

  if ((target_units == "RAD") && (fabs(value) > 2 * M_PI)) {
    cerr << element->ReadFrom() << element->GetName() << " value "
         << value << " RAD is outside the range [ -2*M_PI RAD ; +2*M_PI RAD ]"
         << endl;
  }
  if ((target_units == "DEG") && (fabs(value) > 360.0)) {
    cerr << element->ReadFrom() << element->GetName() << " value "
         << value << " DEG is outside the range [ -360 DEG ; +360 DEG ]"
         << endl;
  }

  value = DisperseValue(element, value, supplied_units, target_units);

  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::FindElementValueAsNumberConvertFromTo( const string& el,
                                                       const string& supplied_units,
                                                       const string& target_units)
{
  Element* element = FindElement(el);

  if (!element) {
    std::stringstream s;
    s << ReadFrom() << "Attempting to get non-existent element " << el;
    cerr << s.str() << endl;
    throw length_error(s.str());
  }

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      std::stringstream s;
      s << element->ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" does not exist (typo?).";
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      std::stringstream s;
      s << element->ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" cannot be converted to " << target_units;
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
  }

  double value = element->GetDataAsNumber();
  if (!supplied_units.empty()) {
    value *= convert[supplied_units][target_units];
  }

  value = DisperseValue(element, value, supplied_units, target_units);

  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 Element::FindElementTripletConvertTo( const string& target_units)
{
  FGColumnVector3 triplet;
  Element* item;
  double value=0.0;
  string supplied_units = GetAttributeValue("unit");

  if (!supplied_units.empty()) {
    if (convert.find(supplied_units) == convert.end()) {
      std::stringstream s;
      s << ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" does not exist (typo?).";
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
    if (convert[supplied_units].find(target_units) == convert[supplied_units].end()) {
      std::stringstream s;
      s << ReadFrom() << "Supplied unit: \"" << supplied_units
        << "\" cannot be converted to " << target_units;
      cerr << s.str() << endl;
      throw invalid_argument(s.str());
    }
  }

  item = FindElement("x");
  if (!item) item = FindElement("roll");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
    triplet(1) = DisperseValue(item, value, supplied_units, target_units);
  } else {
    triplet(1) = 0.0;
  }


  item = FindElement("y");
  if (!item) item = FindElement("pitch");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
    triplet(2) = DisperseValue(item, value, supplied_units, target_units);
  } else {
    triplet(2) = 0.0;
  }

  item = FindElement("z");
  if (!item) item = FindElement("yaw");
  if (item) {
    value = item->GetDataAsNumber();
    if (!supplied_units.empty()) value *= convert[supplied_units][target_units];
    triplet(3) = DisperseValue(item, value, supplied_units, target_units);
  } else {
    triplet(3) = 0.0;
  }

  return triplet;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::DisperseValue(Element *e, double val, const std::string& supplied_units,
                              const std::string& target_units)
{
  double value=val;
  bool disperse = false;

  if(char* num = getenv("JSBSIM_DISPERSE"); num != nullptr)
    disperse = strtol(num, nullptr, 0) == 1;  // set dispersions

  if (e->HasAttribute("dispersion") && disperse) {
    double disp = e->GetAttributeValueAsNumber("dispersion");
    if (!supplied_units.empty()) disp *= convert[supplied_units][target_units];
    string attType = e->GetAttributeValue("type");
    RandomNumberGenerator generator;

    if (attType == "gaussian" || attType == "gaussiansigned") {
      double grn = generator.GetNormalRandomNumber();
      if (attType == "gaussian")
        value = val + disp*grn;
      else // Assume gaussiansigned
        value = (val + disp*grn)*FGJSBBase::sign(grn);
    } else if (attType == "uniform" || attType == "uniformsigned") {
      double urn = generator.GetUniformRandomNumber();
      if (attType == "uniform")
        value = val + disp * urn;
      else // Assume uniformsigned
        value = (val + disp * urn)*FGJSBBase::sign(urn);
    } else {
      std::stringstream s;
      s << ReadFrom() << "Unknown dispersion type" << attType;
      cerr << s.str() << endl;
      throw domain_error(s.str());
    }

  }
  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::Print(unsigned int level)
{
  unsigned int i, spaces;

  level+=2;
  for (spaces=0; spaces<=level; spaces++) cout << " "; // format output
  cout << "Element Name: " << name;

  map<string, string>::iterator it;
  for (it = attributes.begin(); it != attributes.end(); ++it)
    cout << "  " << it->first << " = " << it->second;

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

void Element::AddAttribute(const string& name, const string& value)
{
  attributes[name] = value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::AddData(string d)
{
  string::size_type string_start = d.find_first_not_of(" \t");
  if (string_start != string::npos && string_start > 0) {
    d.erase(0,string_start);
  }
  data_lines.push_back(d);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string Element::ReadFrom(void) const
{
  ostringstream message;

  message << endl
          << "In file " << GetFileName() << ": line " << GetLineNumber()
          << endl;

  return message.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Element::MergeAttributes(Element* el)
{
  map<string, string>::iterator it;

  for (it=el->attributes.begin(); it != el->attributes.end(); ++it) {
    if (attributes.find(it->first) == attributes.end())
      attributes[it->first] = it->second;
    else {
      if (FGJSBBase::debug_lvl > 0 && (attributes[it->first] != it->second))
        cout << el->ReadFrom() << " Attribute '" << it->first << "' is overridden in file "
             << GetFileName() << ": line " << GetLineNumber() << endl
             << " The value '" << attributes[it->first] << "' will be used instead of '"
             << it->second << "'." << endl;
    }
  }
}

} // end namespace JSBSim
