/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGModelFunctions.cpp
 Author:       Jon S. Berndt
 Date started: August 2010

 ------- Copyright (C) 2010  Jon S. Berndt (jon@jsbsim.org) ------------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>
#include <string>
#include "FGModelFunctions.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGModelFunctions.cpp,v 1.6 2012/09/05 05:00:57 jberndt Exp $";
static const char *IdHdr = ID_MODELFUNCTIONS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGModelFunctions::~FGModelFunctions()
{
  for (unsigned int i=0; i<interface_properties.size(); i++) delete interface_properties[i];
  interface_properties.clear();

  for (unsigned int i=0; i<PreFunctions.size(); i++) delete PreFunctions[i];
  for (unsigned int i=0; i<PostFunctions.size(); i++) delete PostFunctions[i];

  if (debug_lvl & 2) cout << "Destroyed:    FGModelFunctions" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModelFunctions::Load(Element* el, FGPropertyManager* PM, string prefix)
{
  // Interface properties are all stored in the interface properties array.
  string interface_property_string = "";

  Element *property_element = el->FindElement("property");
  if (property_element && debug_lvl > 0) cout << endl << "    Declared properties" 
                                              << endl << endl;
  while (property_element) {
    interface_property_string = property_element->GetDataLine();
    if (PM->HasNode(interface_property_string)) {
      cerr << "      Property " << interface_property_string 
           << " is already defined." << endl;
    } else {
      double value=0.0;
      if ( ! property_element->GetAttributeValue("value").empty())
        value = property_element->GetAttributeValueAsNumber("value");
      interface_properties.push_back(new double(value));
      PM->Tie(interface_property_string, interface_properties.back());
      if (debug_lvl > 0)
        cout << "      " << interface_property_string << " (initial value: " 
             << value << ")" << endl << endl;
    }
    property_element = el->FindNextElement("property");
  }
  
  // End of interface property loading logic

  PreLoad(el, PM, prefix);

  return true; // TODO: Need to make this value mean something.
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGModelFunctions::PreLoad(Element* el, FGPropertyManager* PM, string prefix)
{
  // Load model post-functions, if any

  Element *function = el->FindElement("function");

  while (function) {
    if (function->GetAttributeValue("type") == "pre") {
      PreFunctions.push_back(new FGFunction(PM, function, prefix));
    } else if (function->GetAttributeValue("type").empty()) { // Assume pre-function
      string funcname = function->GetAttributeValue("name");
      if (funcname.find("IdleThrust") == string::npos && // Do not process functions that are
          funcname.find("MilThrust") == string::npos  && // already pre-defined turbine engine
          funcname.find("AugThrust") == string::npos  && // functions. These are loaded within
          funcname.find("Injection") == string::npos )   // the Turbine::Load() method.
      {
        PreFunctions.push_back(new FGFunction(PM, function, prefix));
      }
    }
    function = el->FindNextElement("function");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGModelFunctions::PostLoad(Element* el, FGPropertyManager* PM, string prefix)
{
  // Load model post-functions, if any

  Element *function = el->FindElement("function");
  while (function) {
    if (function->GetAttributeValue("type") == "post") {
      PostFunctions.push_back(new FGFunction(PM, function, prefix));
    }
    function = el->FindNextElement("function");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Tell the Functions to cache values, so when the function values 
// are being used in the model, the functions do not get
// calculated each time, but instead use the values that have already
// been calculated for this frame.

void FGModelFunctions::RunPreFunctions(void)
{
  unsigned int sz = PreFunctions.size();
  for (unsigned int i=0; i<sz; i++) {
    PreFunctions[i]->cacheValue(true);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Tell the Functions to cache values, so when the function values 
// are being used in the model, the functions do not get
// calculated each time, but instead use the values that have already
// been calculated for this frame.

void FGModelFunctions::RunPostFunctions(void)
{
  unsigned int sz = PostFunctions.size();
  for (unsigned int i=0; i<sz; i++) {
    PostFunctions[i]->cacheValue(true);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGModelFunctions::GetFunctionStrings(const string& delimeter) const
{
  string FunctionStrings = "";
  bool firstime = true;
  unsigned int sd;

  for (sd = 0; sd < PreFunctions.size(); sd++) {
    if (firstime) {
      firstime = false;
    } else {
      FunctionStrings += delimeter;
    }
    FunctionStrings += PreFunctions[sd]->GetName();
  }

  for (sd = 0; sd < PostFunctions.size(); sd++) {
    if (firstime) {
      firstime = false;
    } else {
      FunctionStrings += delimeter;
    }
    FunctionStrings += PostFunctions[sd]->GetName();
  }

  return FunctionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGModelFunctions::GetFunctionValues(const string& delimeter) const
{
  ostringstream buf;

  for (unsigned int sd = 0; sd < PreFunctions.size(); sd++) {
    if (buf.tellp() > 0) buf << delimeter;
    buf << PreFunctions[sd]->GetValue();
  }

  for (unsigned int sd = 0; sd < PostFunctions.size(); sd++) {
    if (buf.tellp() > 0) buf << delimeter;
    buf << PostFunctions[sd]->GetValue();
  }

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

}
