/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGModelFunctions.cpp
 Author:       Jon S. Berndt
 Date started: August 2010

 ------- Copyright (C) 2010  Jon S. Berndt (jon@jsbsim.org) ------------------

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

#include "FGFDMExec.h"
#include "FGModelFunctions.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

bool FGModelFunctions::InitModel(void)
{
  LocalProperties.ResetToIC();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModelFunctions::Load(Element* el, FGFDMExec* fdmex, string prefix)
{
  LocalProperties.Load(el, fdmex->GetPropertyManager().get(), false);
  PreLoad(el, fdmex, prefix);

  return true; // TODO: Need to make this value mean something.
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGModelFunctions::PreLoad(Element* el, FGFDMExec* fdmex, string prefix)
{
  // Load model post-functions, if any

  Element *function = el->FindElement("function");

  while (function) {
    string fType = function->GetAttributeValue("type");
    if (fType.empty() || fType == "pre")
      PreFunctions.push_back(std::make_shared<FGFunction>(fdmex, function, prefix));
    else if (fType == "template") {
      string name = function->GetAttributeValue("name");
      fdmex->AddTemplateFunc(name, function);
    }

    function = el->FindNextElement("function");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGModelFunctions::PostLoad(Element* el, FGFDMExec* fdmex, string prefix)
{
  // Load model post-functions, if any

  Element *function = el->FindElement("function");
  while (function) {
    if (function->GetAttributeValue("type") == "post") {
      PostFunctions.push_back(std::make_shared<FGFunction>(fdmex, function, prefix));
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
  for (auto& prefunc: PreFunctions)
    prefunc->cacheValue(true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Tell the Functions to cache values, so when the function values
// are being used in the model, the functions do not get
// calculated each time, but instead use the values that have already
// been calculated for this frame.

void FGModelFunctions::RunPostFunctions(void)
{
  for (auto& postfunc: PostFunctions)
    postfunc->cacheValue(true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGFunction> FGModelFunctions::GetPreFunction(const std::string& name)
{
  for (auto& prefunc: PreFunctions) {
    if (prefunc->GetName() == name)
      return prefunc;
  }

  return nullptr;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGModelFunctions::GetFunctionStrings(const string& delimeter) const
{
  string FunctionStrings;

  for (auto& prefunc: PreFunctions) {
    if (!FunctionStrings.empty())
      FunctionStrings += delimeter;

    FunctionStrings += prefunc->GetName();
  }

  for (auto& postfunc: PostFunctions) {
    if (!FunctionStrings.empty())
      FunctionStrings += delimeter;

    FunctionStrings += postfunc->GetName();
  }

  return FunctionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGModelFunctions::GetFunctionValues(const string& delimeter) const
{
  ostringstream buf;

  for (auto& prefunc: PreFunctions) {
    if (buf.tellp() > 0) buf << delimeter;
    buf << prefunc->GetValue();
  }

  for (auto& postfunc: PostFunctions) {
    if (buf.tellp() > 0) buf << delimeter;
    buf << postfunc->GetValue();
  }

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

}
