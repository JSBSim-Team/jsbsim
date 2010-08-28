/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGFunction.cpp
Author: Jon Berndt
Date started: 8/25/2004
Purpose: Stores various parameter types for functions

 ------------- Copyright (C) 2004  Jon S. Berndt (jon@jsbsim.org) -------------

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

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include "FGFunction.h"
#include "FGTable.h"
#include "FGPropertyValue.h"
#include "FGRealValue.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGFunction.cpp,v 1.35 2010/08/28 12:41:56 jberndt Exp $";
static const char *IdHdr = ID_FUNCTION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFunction::FGFunction(FGPropertyManager* propMan, Element* el, const string& prefix)
                                      : PropertyManager(propMan), Prefix(prefix)
{
  Element* element;
  string operation, property_name;
  cached = false;
  cachedValue = -HUGE_VAL;
  invlog2val = 1.0/log10(2.0);

  property_string = "property";
  value_string = "value";
  table_string = "table";
  p_string = "p";
  v_string = "v";
  t_string = "t";

  function_string = "function";
  description_string = "description";
  sum_string = "sum";
  difference_string = "difference";
  product_string = "product";
  quotient_string = "quotient";
  pow_string = "pow";
  exp_string = "exp";
  log2_string = "log2";
  ln_string = "ln";
  log10_string = "log10";
  abs_string = "abs";
  sin_string = "sin";
  cos_string = "cos";
  tan_string = "tan";
  asin_string = "asin";
  acos_string = "acos";
  atan_string = "atan";
  atan2_string = "atan2";
  min_string = "min";
  max_string = "max";
  avg_string = "avg";
  fraction_string = "fraction";
  mod_string = "mod";
  random_string = "random";
  integer_string = "integer";

  Name = el->GetAttributeValue("name");
  operation = el->GetName();

  if (operation == function_string) {
    Type = eTopLevel;
  } else if (operation == product_string) {
    Type = eProduct;
  } else if (operation == difference_string) {
    Type = eDifference;
  } else if (operation == sum_string) {
    Type = eSum;
  } else if (operation == quotient_string) {
    Type = eQuotient;
  } else if (operation == pow_string) {
    Type = ePow;
  } else if (operation == log2_string) {
    Type = eLog2;
  } else if (operation == ln_string) {
    Type = eLn;
  } else if (operation == log10_string) {
    Type = eLog10;
  } else if (operation == abs_string) {
    Type = eAbs;
  } else if (operation == sin_string) {
    Type = eSin;
  } else if (operation == exp_string) {
    Type = eExp;
  } else if (operation == cos_string) {
    Type = eCos;
  } else if (operation == tan_string) {
    Type = eTan;
  } else if (operation == asin_string) {
    Type = eASin;
  } else if (operation == acos_string) {
    Type = eACos;
  } else if (operation == atan_string) {
    Type = eATan;
  } else if (operation == atan2_string) {
    Type = eATan2;
  } else if (operation == min_string) {
    Type = eMin;
  } else if (operation == max_string) {
    Type = eMax;
  } else if (operation == avg_string) {
    Type = eAvg;
  } else if (operation == fraction_string) {
    Type = eFrac;
  } else if (operation == integer_string) {
    Type = eInteger;
  } else if (operation == mod_string) {
    Type = eMod;
  } else if (operation == random_string) {
    Type = eRandom;
  } else if (operation != description_string) {
    cerr << "Bad operation " << operation << " detected in configuration file" << endl;
  }

  element = el->GetElement();
  if (!element) {
    cerr << fgred << highint << endl;
    cerr << "  No element was specified as an argument to the \"" << operation << "\" operation" << endl;
    cerr << "  This can happen when, for instance, a cos operation is specified and a " << endl;
    cerr << "  property name is given explicitly, but is not placed within a" << endl;
    cerr << "  <property></property> element tag pair." << endl;
    cerr << reset;
    exit(-2);
  }
  
  while (element) {
    operation = element->GetName();

    // data types
    if (operation == property_string || operation == p_string) {
      property_name = element->GetDataLine();
      if (property_name.find("#") != string::npos) {
        if (is_number(Prefix)) {
          property_name = replace(property_name,"#",Prefix);
        }
      }
      FGPropertyManager* newNode = 0L;
      if (PropertyManager->HasNode(property_name)) {
        newNode = PropertyManager->GetNode(property_name);
        Parameters.push_back(new FGPropertyValue( newNode ));
      } else {
        cerr << fgcyan << "The property " + property_name + " is initially undefined."
             << reset << endl;
        Parameters.push_back(new FGPropertyValue( property_name ));
      }
    } else if (operation == value_string || operation == v_string) {
      Parameters.push_back(new FGRealValue(element->GetDataAsNumber()));
    } else if (operation == table_string || operation == t_string) {
      Parameters.push_back(new FGTable(PropertyManager, element));
    // operations
    } else if (operation == product_string ||
               operation == difference_string ||
               operation == sum_string ||
               operation == quotient_string ||
               operation == pow_string ||
               operation == exp_string ||
               operation == log2_string ||
               operation == ln_string ||
               operation == log10_string ||
               operation == abs_string ||
               operation == sin_string ||
               operation == cos_string ||
               operation == tan_string ||
               operation == asin_string ||
               operation == acos_string ||
               operation == atan_string ||
               operation == atan2_string ||
               operation == min_string ||
               operation == max_string ||
               operation == fraction_string ||
               operation == integer_string ||
               operation == mod_string ||
               operation == random_string ||
               operation == avg_string )
    {
      Parameters.push_back(new FGFunction(PropertyManager, element, Prefix));
    } else if (operation != description_string) {
      cerr << "Bad operation " << operation << " detected in configuration file" << endl;
    }
    element = el->GetNextElement();
  }

  bind(); // Allow any function to save its value

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFunction::~FGFunction(void)
{
  for (unsigned int i=0; i<Parameters.size(); i++) delete Parameters[i];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::cacheValue(bool cache)
{
  cached = false; // Must set cached to false prior to calling GetValue(), else
                  // it will _never_ calculate the value;
  if (cache) {
    cachedValue = GetValue();
    cached = true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFunction::GetValue(void) const
{
  unsigned int i;
  double scratch;
  double temp=0;

  if (cached) return cachedValue;

  try {
    temp = Parameters[0]->GetValue();
  } catch (string prop) {
    if (PropertyManager->HasNode(prop)) {
      ((FGPropertyValue*)Parameters[0])->SetNode(PropertyManager->GetNode(prop));
      temp = Parameters[0]->GetValue();
    } else {
      throw("Property " + prop + " was not defined anywhere.");
    }
  }

  switch (Type) {
  case eTopLevel:
    break;
  case eProduct:
    for (i=1;i<Parameters.size();i++) {
      temp *= Parameters[i]->GetValue();
    }
    break;
  case eDifference:
    for (i=1;i<Parameters.size();i++) {
      temp -= Parameters[i]->GetValue();
    }
    break;
  case eSum:
    for (i=1;i<Parameters.size();i++) {
      temp += Parameters[i]->GetValue();
    }
    break;
  case eQuotient:
    if (Parameters[1]->GetValue() != 0.0)
      temp /= Parameters[1]->GetValue();
    else
      temp = HUGE_VAL;
    break;
  case ePow:
    temp = pow(temp,Parameters[1]->GetValue());
    break;
  case eExp:
    temp = exp(temp);
    break;
  case eLog2:
    if (temp > 0.00) temp = log10(temp)*invlog2val;
    else temp = -HUGE_VAL;
    break;
  case eLn:
    if (temp > 0.00) temp = log(temp);
    else temp = -HUGE_VAL;
    break;
  case eLog10:
    if (temp > 0.00) temp = log10(temp);
    else temp = -HUGE_VAL;
    break;
  case eAbs:
    temp = fabs(temp);
    break;
  case eSin:
    temp = sin(temp);
    break;
  case eCos:
    temp = cos(temp);
    break;
  case eTan:
    temp = tan(temp);
    break;
  case eACos:
    temp = acos(temp);
    break;
  case eASin:
    temp = asin(temp);
    break;
  case eATan:
    temp = atan(temp);
    break;
  case eATan2:
    temp = atan2(temp, Parameters[1]->GetValue());
    break;
  case eMod:
    temp = ((int)temp) % ((int) Parameters[1]->GetValue());
    break;
  case eMin:
    for (i=1;i<Parameters.size();i++) {
      if (Parameters[i]->GetValue() < temp) temp = Parameters[i]->GetValue();
    }    
    break;
  case eMax:
    for (i=1;i<Parameters.size();i++) {
      if (Parameters[i]->GetValue() > temp) temp = Parameters[i]->GetValue();
    }    
    break;
  case eAvg:
    for (i=1;i<Parameters.size();i++) {
      temp += Parameters[i]->GetValue();
    }
    temp /= Parameters.size();
    break;
  case eFrac:
    temp = modf(temp, &scratch);
    break;
  case eInteger:
    modf(temp, &scratch);
    temp = scratch;
    break;
  case eRandom:
    temp = GaussianRandomNumber();
    break;
  default:
    cerr << "Unknown function operation type" << endl;
    break;
  }

  return temp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFunction::GetValueAsString(void) const
{
  ostringstream buffer;

  buffer << setw(9) << setprecision(6) << GetValue();
  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::bind(void)
{
  if ( !Name.empty() ) {
    string tmp;
    if (Prefix.empty())
      tmp  = PropertyManager->mkPropertyName(Name, false);
    else {
      if (is_number(Prefix)) {
        if (Name.find("#") != string::npos) { // if "#" is found
          Name = replace(Name,"#",Prefix);
          tmp  = PropertyManager->mkPropertyName(Name, false);
        } else {
          cerr << "Malformed function name with number: " << Prefix
            << " and property name: " << Name
            << " but no \"#\" sign for substitution." << endl;
        }
      } else {
        tmp  = PropertyManager->mkPropertyName(Prefix + "/" + Name, false);
      }
    }

    PropertyManager->Tie( tmp, this, &FGFunction::GetValue);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGFunction::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (Type == eTopLevel)
        cout << "    Function: " << Name << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFunction" << endl;
    if (from == 1) cout << "Destroyed:    FGFunction" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

}
