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

static const char *IdSrc = "$Id: FGXMLElement.cpp,v 1.2 2004/10/03 13:43:34 jberndt Exp $";
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
  convert["SLUG*FT2"]["KG*M2"] = 1.35694;
  convert["KG*M2"]["SLUG*FT2"] = 1.0/convert["SLUG*FT2"]["KG*M2"];
  convert["RAD"]["DEG"] = 360.0/(2.0*3.1415926);
  convert["DEG"]["RAD"] = 1.0/convert["RAD"]["DEG"];

  convert["M"]["M"] = 1.00;
  convert["FT"]["FT"] = 1.00;
  convert["IN"]["IN"] = 1.00;
  convert["DEG"]["DEG"] = 1.00;
  convert["RAD"]["RAD"] = 1.00;
  convert["M2"]["M2"] = 1.00;
  convert["FT2"]["FT2"] = 1.00;
  convert["KG*M2"]["KG*M2"] = 1.00;
  convert["SLUG*FT2"]["SLUG*FT2"] = 1.00;
  convert["KG"]["KG"] = 1.00;
  convert["LBS"]["LBS"] = 1.00;
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
  if (data_lines.size() > 0)
    return data_lines[i];
  else
    return string("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Element::GetDataAsNumber(void)
{
  if (data_lines.size() == 1) {
    return atof(data_lines[0].c_str());
  } else {
    return HUGE_VAL;
  }

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
      element_index = i;
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
    return HUGE_VAL;
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
     supplied_units = element->GetAttributeValue("UNIT");
     if (!supplied_units.empty()) {
       value *= convert[supplied_units][target_units];
     }
  } else {
    return HUGE_VAL;
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
       value *= convert[supplied_units][target_units];
     }
  } else {
    return HUGE_VAL;
  }
  return value;
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

void Element::Print(int level)
{
  level+=2;
  for (int spaces=0; spaces<=level; spaces++) cout << " "; // format output
  cout << "Element Name: " << name;
  for (int i=0; i<attributes.size(); i++) {
    cout << "  " << attribute_key[i] << " = " << attributes[attribute_key[i]];
  }
  cout << endl;
  for (int i=0; i<data_lines.size(); i++) {
    for (int spaces=0; spaces<=level; spaces++) cout << " "; // format output
    cout << data_lines[i] << endl;
  }
  for (int i=0; i<children.size(); i++) {
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
  int string_start = d.find_first_not_of(" ");
  if (string_start > 0) d.erase(0,string_start-1);
  data_lines.push_back(d);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // end namespace JSBSim
