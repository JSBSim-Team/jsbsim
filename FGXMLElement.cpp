/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Author:       Jon Berndt
 Date started: 09/28/2004
 Purpose:      XML element class
 Called by:    FGXMLParse

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLElement.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGXMLElement.cpp,v 1.3 2004/10/04 19:19:16 ehofman Exp $";
static const char *IdHdr = ID_XMLELEMENT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Element::Element(string nm)
{
  name   = nm;
  parent = 0L;
  element_index = 0;
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
