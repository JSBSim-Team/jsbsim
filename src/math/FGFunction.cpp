/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGFunction.cpp
Author: Jon Berndt
Date started: 8/25/2004
Purpose: Stores various parameter types for functions

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdio.h>

#include "FGFunction.h"
#include "FGTable.h"
#include "FGPropertyValue.h"
#include "FGRealValue.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGFunction.cpp,v 1.6 2006/03/07 11:39:35 jberndt Exp $";
static const char *IdHdr = ID_FUNCTION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFunction::FGFunction(FGPropertyManager* propMan, Element* el, string prefix)
                                      : PropertyManager(propMan), Prefix(prefix)
{
  int i;
  Element* element;
  string operation, property_name;
  int size = el->GetNumElements();
  cached = false;
  cachedValue = -HUGE_VAL;

  Name = el->GetAttributeValue("name");
  operation = el->GetName();
  if (operation == string("function")) {
    Type = eTopLevel;
    bind();
  } else if (operation == string("product")) {
    Type = eProduct;
  } else if (operation == string("difference")) {
    Type = eDifference;
  } else if (operation == string("sum")) {
    Type = eSum;
  } else if (operation == string("quotient")) {
    Type = eQuotient;
  } else if (operation == string("pow")) {
    Type = ePow;
  } else if (operation == string("abs")) {
    Type = eAbs;
  } else if (operation == string("sin")) {
    Type = eSin;
  } else if (operation == string("cos")) {
    Type = eCos;
  } else if (operation == string("tan")) {
    Type = eTan;
  } else if (operation == string("asin")) {
    Type = eASin;
  } else if (operation == string("acos")) {
    Type = eACos;
  } else if (operation == string("atan")) {
    Type = eATan;
  } else if (operation == string("atan2")) {
    Type = eATan2;
  } else if (operation != string("description")) {
    cerr << "Bad operation " << operation << " detected in configuration file" << endl;
  }

  element = el->GetElement();
  while (element) {
    operation = element->GetName();

    // data types
    if (operation == string("property")) {
      property_name = element->GetDataLine();
      FGPropertyManager* newNode = PropertyManager->GetNode(property_name);
      if (newNode == 0) {
        cerr << "The property " << property_name << " is undefined." << endl;
        abort();
      } else {
        Parameters.push_back(new FGPropertyValue( newNode ));
      }
    } else if (operation == string("value")) {
      Parameters.push_back(new FGRealValue(element->GetDataAsNumber()));
    } else if (operation == string("table")) {
      Parameters.push_back(new FGTable(PropertyManager, element));
    // operations
    } else if (operation == string("product") ||
               operation == string("difference") ||
               operation == string("sum") ||
               operation == string("quotient") ||
               operation == string("pow") ||
               operation == string("abs") ||
               operation == string("sin") ||
               operation == string("cos") ||
               operation == string("tan") ||
               operation == string("asin") ||
               operation == string("acos") ||
               operation == string("atan") ||
               operation == string("atan2"))
    {
      Parameters.push_back(new FGFunction(PropertyManager, element));
    } else if (operation != string("description")) {
      cerr << "Bad operation " << operation << " detected in configuration file" << endl;
    }
    element = el->GetNextElement();
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFunction::~FGFunction(void)
{
  string tmp = PropertyManager->mkPropertyName(Prefix + Name, false); // Allow upper case
  PropertyManager->Untie(tmp);
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
  int i;

  if (cached) return cachedValue;

  double temp = Parameters[0]->GetValue();

  switch (Type) {
  case eTopLevel:
    break;
  case eProduct:
    for (i=1;i<Parameters.size();i++) temp *= Parameters[i]->GetValue();
    break;
  case eDifference:
    for (i=1;i<Parameters.size();i++) temp -= Parameters[i]->GetValue();
    break;
  case eSum:
    for (i=1;i<Parameters.size();i++) temp += Parameters[i]->GetValue();
    break;
  case eQuotient:
    temp /= Parameters[1]->GetValue();
    break;
  case ePow:
    temp = pow(temp,Parameters[1]->GetValue());
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
  default:
    cerr << "Unknown function operation type" << endl;
    break;
  }

  return temp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFunction::GetValueAsString(void) const
{
  char buffer[20];
  string value;

  sprintf(buffer,"%9.6f",GetValue());
  value = string(buffer);
  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::bind(void)
{
  if ( !Name.empty() ) {
    string tmp = PropertyManager->mkPropertyName(Prefix + Name, false); // Allow upper case
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
    if (from == 0) cout << "Instantiated: FGGroundReactions" << endl;
    if (from == 1) cout << "Destroyed:    FGGroundReactions" << endl;
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
