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

static const char *IdSrc = "$Id: FGFunction.cpp,v 1.45 2012/09/05 04:54:49 jberndt Exp $";
static const char *IdHdr = ID_FUNCTION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

const std::string FGFunction::property_string = "property";
const std::string FGFunction::value_string = "value";
const std::string FGFunction::table_string = "table";
const std::string FGFunction::p_string = "p";
const std::string FGFunction::v_string = "v";
const std::string FGFunction::t_string = "t";

const std::string FGFunction::function_string = "function";
const std::string FGFunction::description_string = "description";
const std::string FGFunction::sum_string = "sum";
const std::string FGFunction::difference_string = "difference";
const std::string FGFunction::product_string = "product";
const std::string FGFunction::quotient_string = "quotient";
const std::string FGFunction::pow_string = "pow";
const std::string FGFunction::exp_string = "exp";
const std::string FGFunction::log2_string = "log2";
const std::string FGFunction::ln_string = "ln";
const std::string FGFunction::log10_string = "log10";
const std::string FGFunction::abs_string = "abs";
const std::string FGFunction::sign_string = "sign";
const std::string FGFunction::sin_string = "sin";
const std::string FGFunction::cos_string = "cos";
const std::string FGFunction::tan_string = "tan";
const std::string FGFunction::asin_string = "asin";
const std::string FGFunction::acos_string = "acos";
const std::string FGFunction::atan_string = "atan";
const std::string FGFunction::atan2_string = "atan2";
const std::string FGFunction::min_string = "min";
const std::string FGFunction::max_string = "max";
const std::string FGFunction::avg_string = "avg";
const std::string FGFunction::fraction_string = "fraction";
const std::string FGFunction::mod_string = "mod";
const std::string FGFunction::random_string = "random";
const std::string FGFunction::integer_string = "integer";
const std::string FGFunction::rotation_alpha_local_string = "rotation_alpha_local";
const std::string FGFunction::rotation_beta_local_string = "rotation_beta_local";
const std::string FGFunction::rotation_gamma_local_string = "rotation_gamma_local";
const std::string FGFunction::rotation_bf_to_wf_string = "rotation_bf_to_wf";
const std::string FGFunction::rotation_wf_to_bf_string = "rotation_wf_to_bf";

const std::string FGFunction::lessthan_string = "lt";
const std::string FGFunction::lessequal_string = "le";
const std::string FGFunction::greatthan_string = "gt";
const std::string FGFunction::greatequal_string = "ge";
const std::string FGFunction::equal_string = "eq";
const std::string FGFunction::notequal_string = "nq";
const std::string FGFunction::and_string = "and";
const std::string FGFunction::or_string = "or";
const std::string FGFunction::not_string = "not";
const std::string FGFunction::ifthen_string = "ifthen";
const std::string FGFunction::switch_string = "switch";
const std::string FGFunction::interpolate1d_string = "interpolate1d";

FGFunction::FGFunction(FGPropertyManager* propMan, Element* el, const string& prefix)
                                      : PropertyManager(propMan), Prefix(prefix)
{
  Element* element;
  string operation, property_name;
  cached = false;
  cachedValue = -HUGE_VAL;
  invlog2val = 1.0/log10(2.0);
  pCopyTo = 0L;

  Name = el->GetAttributeValue("name");
  operation = el->GetName();

  if (operation == function_string) {
    sCopyTo = el->GetAttributeValue("copyto");
    if (!sCopyTo.empty()) {
      pCopyTo = PropertyManager->GetNode(sCopyTo);
      if (pCopyTo == 0L) cerr << "Property \"" << sCopyTo << "\" must be previously defined in function "
                              << Name << endl;
    }
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
  } else if (operation == sign_string) {
    Type = eSign;
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
  } else if (operation == rotation_alpha_local_string) {
    Type = eRotation_alpha_local;
  } else if (operation == rotation_beta_local_string) {
    Type = eRotation_beta_local;
  } else if (operation == rotation_gamma_local_string) {
    Type = eRotation_gamma_local;
  } else if (operation == rotation_bf_to_wf_string) {
    Type = eRotation_bf_to_wf;
  } else if (operation == rotation_wf_to_bf_string) {
    Type = eRotation_wf_to_bf;
  } else if (operation == lessthan_string) {
    Type = eLT;
  } else if (operation == lessequal_string) {
    Type = eLE;
  } else if (operation == greatthan_string) {
    Type = eGT;
  } else if (operation == greatequal_string) {
    Type = eGE;
  } else if (operation == equal_string) {
    Type = eEQ;
  } else if (operation == notequal_string) {
    Type = eNE;
  } else if (operation == and_string) {
    Type = eAND;
  } else if (operation == or_string) {
    Type = eOR;
  } else if (operation == not_string) {
    Type = eNOT;
  } else if (operation == ifthen_string) {
    Type = eIfThen;
  } else if (operation == switch_string) {
    Type = eSwitch;
  } else if (operation == interpolate1d_string) {
    Type = eInterpolate1D;
  } else if (operation != description_string) {
    cerr << "Bad operation " << operation << " detected in configuration file" << endl;
  }

  element = el->GetElement();
  if (!element && Type != eRandom) {
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
        cerr << fgcyan << "Warning: The property " + property_name + " is initially undefined."
             << reset << endl;
        Parameters.push_back(new FGPropertyValue( property_name,
                                                  PropertyManager ));
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
               operation == sign_string ||
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
               operation == avg_string ||
               operation == rotation_alpha_local_string||
               operation == rotation_beta_local_string||
               operation == rotation_gamma_local_string||
               operation == rotation_bf_to_wf_string||
               operation == rotation_wf_to_bf_string ||
               operation == lessthan_string ||
               operation == lessequal_string ||
               operation == greatthan_string ||
               operation == greatequal_string ||
               operation == equal_string ||
               operation == notequal_string ||
               operation == and_string ||
               operation == or_string ||
               operation == not_string ||
               operation == ifthen_string ||
               operation == switch_string ||
               operation == interpolate1d_string)
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

unsigned int FGFunction::GetBinary(double val) const
{
  val = fabs(val);
  if (val < 1E-9) return 0;
  else if (val-1 < 1E-9) return 1;
  else {
    throw("Malformed conditional check in function definition.");
  }
}
  
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFunction::GetValue(void) const
{
  unsigned int i;
  double scratch;
  double temp=0;

  if (cached) return cachedValue;

  if (Type != eRandom) temp = Parameters[0]->GetValue();
  
  switch (Type) {
  case eTopLevel:
    if (pCopyTo) pCopyTo->setDoubleValue(temp);
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
  case eSign:
    temp =  temp < 0 ? -1:1; // 0.0 counts as positive.
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
  case eLT:
    temp = (temp < Parameters[1]->GetValue())?1:0;
    break;
  case eLE:
    temp = (temp <= Parameters[1]->GetValue())?1:0;
    break;
  case eGT:
    temp = (temp > Parameters[1]->GetValue())?1:0;
    break;
  case eGE:
    temp = (temp >= Parameters[1]->GetValue())?1:0;
    break;
  case eEQ:
    temp = (temp == Parameters[1]->GetValue())?1:0;
    break;
  case eNE:
    temp = (temp != Parameters[1]->GetValue())?1:0;
    break;
  case eAND:
    {
      bool flag = (GetBinary(temp) != 0u);
      for (i=1; i<Parameters.size() && flag; i++) {
        flag = (GetBinary(Parameters[i]->GetValue()) != 0);
      }
      temp = flag ? 1 : 0;
    }
    break;
  case eOR:
    {
      bool flag = (GetBinary(temp) != 0);
      for (i=1; i<Parameters.size() && !flag; i++) {
        flag = (GetBinary(Parameters[i]->GetValue()) != 0);
      }
      temp = flag ? 1 : 0;
    }
    break;
  case eNOT:
    temp = (GetBinary(temp) != 0) ? 0 : 1;
    break;
  case eIfThen:
    {
      i = Parameters.size();
      if (i == 3) {
        if (GetBinary(temp) == 1) {
          temp = Parameters[1]->GetValue();
        } else {
          temp = Parameters[2]->GetValue();
        }
      } else {
        throw("Malformed if/then function statement");
      }
    }
    break;
  case eSwitch:
    {
      unsigned int n = Parameters.size()-1;
      i = int(temp+0.5);
      if (i >= 0u && i < n) {
        temp = Parameters[i+1]->GetValue();
      } else {
        throw(string("The switch function index selected a value above the range of supplied values"
                     " - not enough values were supplied."));
      }
    }
    break;
  case eInterpolate1D:
    {
      unsigned int sz = Parameters.size();
      if (temp <= Parameters[1]->GetValue()) {
        temp = Parameters[2]->GetValue();
      } else if (temp >= Parameters[sz-2]->GetValue()) {
        temp = Parameters[sz-1]->GetValue();
      } else {
        for (unsigned int i=1; i<=sz-4; i+=2) {
          if (temp < Parameters[i+2]->GetValue()) {
            double factor = (temp - Parameters[i]->GetValue()) /
                            (Parameters[i+2]->GetValue() - Parameters[i]->GetValue());
            double span = Parameters[i+3]->GetValue() - Parameters[i+1]->GetValue();
            double val = factor*span;
            temp = Parameters[i+1]->GetValue() + val;
            break;
          }
        }
      }
    }
    break;
  case eRotation_alpha_local:
    if (Parameters.size()==6) // calculates local angle of attack for skydiver body component
        //Euler angles from the intermediate body frame to the local body frame must be from a z-y-x axis rotation order
    {
        double alpha = Parameters[0]->GetValue()*degtorad;
        double beta = Parameters[1]->GetValue()*degtorad;
        double gamma = Parameters[2]->GetValue()*degtorad;
        double phi = Parameters[3]->GetValue()*degtorad;
        double theta = Parameters[4]->GetValue()*degtorad;
        double psi = Parameters[5]->GetValue()*degtorad;
        double cphi2 = cos(-phi/2), ctht2 = cos(-theta/2), cpsi2 = cos(-psi/2);
        double sphi2 = sin(-phi/2), stht2 = sin(-theta/2), spsi2 = sin(-psi/2);
        double calpha2 = cos(-alpha/2), salpha2 = sin(-alpha/2);
        double cbeta2 = cos(beta/2), sbeta2 = sin(beta/2);
        double cgamma2 = cos(-gamma/2), sgamma2 = sin(-gamma/2);
        //calculate local body frame to the intermediate body frame rotation quaternion
        double At = cphi2*ctht2*cpsi2 - sphi2*stht2*spsi2;
        double Ax = cphi2*stht2*spsi2 + sphi2*ctht2*cpsi2;
        double Ay = cphi2*stht2*cpsi2 - sphi2*ctht2*spsi2;
        double Az = cphi2*ctht2*spsi2 + sphi2*stht2*cpsi2;
        //calculate the intermediate body frame to global wind frame rotation quaternion
        double Bt = calpha2*cbeta2*cgamma2 - salpha2*sbeta2*sgamma2;
        double Bx = calpha2*cbeta2*sgamma2 + salpha2*sbeta2*cgamma2;
        double By = calpha2*sbeta2*sgamma2 + salpha2*cbeta2*cgamma2;
        double Bz = calpha2*sbeta2*cgamma2 - salpha2*cbeta2*sgamma2;
        //multiply quaternions
        double Ct = At*Bt - Ax*Bx - Ay*By - Az*Bz;
        double Cx = At*Bx + Ax*Bt + Ay*Bz - Az*By;
        double Cy = At*By - Ax*Bz + Ay*Bt + Az*Bx;
        double Cz = At*Bz + Ax*By - Ay*Bx + Az*Bt;
        //calculate alpha_local
        temp = -atan2(2*(Cy*Ct-Cx*Cz),(Ct*Ct+Cx*Cx-Cy*Cy-Cz*Cz));
        temp *= radtodeg;
    } else {
      temp = 1;
    }
    break;
  case eRotation_beta_local:
    if (Parameters.size()==6) // calculates local angle of sideslip for skydiver body component
        //Euler angles from the intermediate body frame to the local body frame must be from a z-y-x axis rotation order
    {
        double alpha = Parameters[0]->GetValue()*degtorad; //angle of attack of intermediate body frame
        double beta = Parameters[1]->GetValue()*degtorad;  //sideslip angle of intermediate body frame
        double gamma = Parameters[2]->GetValue()*degtorad; //roll angle of intermediate body frame
        double phi = Parameters[3]->GetValue()*degtorad;   //x-axis Euler angle from the intermediate body frame to the local body frame
        double theta = Parameters[4]->GetValue()*degtorad; //y-axis Euler angle from the intermediate body frame to the local body frame
        double psi = Parameters[5]->GetValue()*degtorad;   //z-axis Euler angle from the intermediate body frame to the local body frame
        double cphi2 = cos(-phi/2), ctht2 = cos(-theta/2), cpsi2 = cos(-psi/2);
        double sphi2 = sin(-phi/2), stht2 = sin(-theta/2), spsi2 = sin(-psi/2);
        double calpha2 = cos(-alpha/2), salpha2 = sin(-alpha/2);
        double cbeta2 = cos(beta/2), sbeta2 = sin(beta/2);
        double cgamma2 = cos(-gamma/2), sgamma2 = sin(-gamma/2);
        //calculate local body frame to the intermediate body frame rotation quaternion
        double At = cphi2*ctht2*cpsi2 - sphi2*stht2*spsi2;
        double Ax = cphi2*stht2*spsi2 + sphi2*ctht2*cpsi2;
        double Ay = cphi2*stht2*cpsi2 - sphi2*ctht2*spsi2;
        double Az = cphi2*ctht2*spsi2 + sphi2*stht2*cpsi2;
        //calculate the intermediate body frame to global wind frame rotation quaternion
        double Bt = calpha2*cbeta2*cgamma2 - salpha2*sbeta2*sgamma2;
        double Bx = calpha2*cbeta2*sgamma2 + salpha2*sbeta2*cgamma2;
        double By = calpha2*sbeta2*sgamma2 + salpha2*cbeta2*cgamma2;
        double Bz = calpha2*sbeta2*cgamma2 - salpha2*cbeta2*sgamma2;
        //multiply quaternions
        double Ct = At*Bt - Ax*Bx - Ay*By - Az*Bz;
        double Cx = At*Bx + Ax*Bt + Ay*Bz - Az*By;
        double Cy = At*By - Ax*Bz + Ay*Bt + Az*Bx;
        double Cz = At*Bz + Ax*By - Ay*Bx + Az*Bt;
        //calculate beta_local
        temp = asin(2*(Cx*Cy+Cz*Ct));
        temp *= radtodeg;
    }
    else // 
    {temp = 1;}
    break;
  case eRotation_gamma_local:
    if (Parameters.size()==6) // calculates local angle of attack for skydiver body component
        //Euler angles from the intermediate body frame to the local body frame must be from a z-y-x axis rotation order
        {
        double alpha = Parameters[0]->GetValue()*degtorad; //angle of attack of intermediate body frame
        double beta = Parameters[1]->GetValue()*degtorad;  //sideslip angle of intermediate body frame
        double gamma = Parameters[2]->GetValue()*degtorad; //roll angle of intermediate body frame
        double phi = Parameters[3]->GetValue()*degtorad;   //x-axis Euler angle from the intermediate body frame to the local body frame
        double theta = Parameters[4]->GetValue()*degtorad; //y-axis Euler angle from the intermediate body frame to the local body frame
        double psi = Parameters[5]->GetValue()*degtorad;   //z-axis Euler angle from the intermediate body frame to the local body frame
        double cphi2 = cos(-phi/2), ctht2 = cos(-theta/2), cpsi2 = cos(-psi/2);
        double sphi2 = sin(-phi/2), stht2 = sin(-theta/2), spsi2 = sin(-psi/2);
        double calpha2 = cos(-alpha/2), salpha2 = sin(-alpha/2);
        double cbeta2 = cos(beta/2), sbeta2 = sin(beta/2);
        double cgamma2 = cos(-gamma/2), sgamma2 = sin(-gamma/2);
        //calculate local body frame to the intermediate body frame rotation quaternion
        double At = cphi2*ctht2*cpsi2 - sphi2*stht2*spsi2;
        double Ax = cphi2*stht2*spsi2 + sphi2*ctht2*cpsi2;
        double Ay = cphi2*stht2*cpsi2 - sphi2*ctht2*spsi2;
        double Az = cphi2*ctht2*spsi2 + sphi2*stht2*cpsi2;
        //calculate the intermediate body frame to global wind frame rotation quaternion
        double Bt = calpha2*cbeta2*cgamma2 - salpha2*sbeta2*sgamma2;
        double Bx = calpha2*cbeta2*sgamma2 + salpha2*sbeta2*cgamma2;
        double By = calpha2*sbeta2*sgamma2 + salpha2*cbeta2*cgamma2;
        double Bz = calpha2*sbeta2*cgamma2 - salpha2*cbeta2*sgamma2;
        //multiply quaternions
        double Ct = At*Bt - Ax*Bx - Ay*By - Az*Bz;
        double Cx = At*Bx + Ax*Bt + Ay*Bz - Az*By;
        double Cy = At*By - Ax*Bz + Ay*Bt + Az*Bx;
        double Cz = At*Bz + Ax*By - Ay*Bx + Az*Bt;
        //calculate local roll anlge
        temp = -atan2(2*(Cx*Ct-Cz*Cy),(Ct*Ct-Cx*Cx+Cy*Cy-Cz*Cz));
        temp *= radtodeg;
    }
    else // 
    {temp = 1;}
    break;
  case eRotation_bf_to_wf:
    if (Parameters.size()==7) // transforms the input vector from a body frame to a wind frame.  The origin of the vector remains the same.
    {
        double rx = Parameters[0]->GetValue();             //x component of input vector
        double ry = Parameters[1]->GetValue();             //y component of input vector
        double rz = Parameters[2]->GetValue();             //z component of input vector
        double alpha = Parameters[3]->GetValue()*degtorad; //angle of attack of the body frame
        double beta = Parameters[4]->GetValue()*degtorad;  //sideslip angle of the body frame
        double gamma = Parameters[5]->GetValue()*degtorad; //roll angle of the body frame
        double index = Parameters[6]->GetValue();
        double calpha2 = cos(-alpha/2), salpha2 = sin(-alpha/2);
        double cbeta2 = cos(beta/2), sbeta2 = sin(beta/2);
        double cgamma2 = cos(-gamma/2), sgamma2 = sin(-gamma/2);
        //calculate the body frame to wind frame quaternion
        double qt = calpha2*cbeta2*cgamma2 - salpha2*sbeta2*sgamma2;
        double qx = calpha2*cbeta2*sgamma2 + salpha2*sbeta2*cgamma2;
        double qy = calpha2*sbeta2*sgamma2 + salpha2*cbeta2*cgamma2;
        double qz = calpha2*sbeta2*cgamma2 - salpha2*cbeta2*sgamma2;
        //calculate the quaternion conjugate
        double qstart = qt;
        double qstarx = -qx;
        double qstary = -qy;
        double qstarz = -qz;
        //multiply quaternions v*q
        double vqt = -rx*qx - ry*qy - rz*qz;
        double vqx =  rx*qt + ry*qz - rz*qy;
        double vqy = -rx*qz + ry*qt + rz*qx;
        double vqz =  rx*qy - ry*qx + rz*qt;
        //multiply quaternions qstar*vq
        double Cx = qstart*vqx + qstarx*vqt + qstary*vqz - qstarz*vqy;
        double Cy = qstart*vqy - qstarx*vqz + qstary*vqt + qstarz*vqx;
        double Cz = qstart*vqz + qstarx*vqy - qstary*vqx + qstarz*vqt;

        if (index == 1)     temp = Cx;
        else if (index ==2) temp = Cy;
        else                temp = Cz;
    }
    else // 
    {temp = 1;}
    break;
  case eRotation_wf_to_bf:
    if (Parameters.size()==7) // transforms the input vector from q wind frame to a body frame.  The origin of the vector remains the same.
    {
        double rx = Parameters[0]->GetValue();             //x component of input vector
        double ry = Parameters[1]->GetValue();             //y component of input vector
        double rz = Parameters[2]->GetValue();             //z component of input vector
        double alpha = Parameters[3]->GetValue()*degtorad; //angle of attack of the body frame
        double beta = Parameters[4]->GetValue()*degtorad;  //sideslip angle of the body frame
        double gamma = Parameters[5]->GetValue()*degtorad; //roll angle of the body frame
        double index = Parameters[6]->GetValue();
        double calpha2 = cos(alpha/2), salpha2 = sin(alpha/2);
        double cbeta2 = cos(-beta/2), sbeta2 = sin(-beta/2);
        double cgamma2 = cos(gamma/2), sgamma2 = sin(gamma/2);
        //calculate the wind frame to body frame quaternion
        double qt =  cgamma2*cbeta2*calpha2 + sgamma2*sbeta2*salpha2;
        double qx = -cgamma2*sbeta2*salpha2 + sgamma2*cbeta2*calpha2;
        double qy =  cgamma2*cbeta2*salpha2 - sgamma2*sbeta2*calpha2;
        double qz =  cgamma2*sbeta2*calpha2 + sgamma2*cbeta2*salpha2;
        //calculate the quaternion conjugate
        double qstart =  qt;
        double qstarx = -qx;
        double qstary = -qy;
        double qstarz = -qz;
        //multiply quaternions v*q
        double vqt = -rx*qx - ry*qy - rz*qz;
        double vqx =  rx*qt + ry*qz - rz*qy;
        double vqy = -rx*qz + ry*qt + rz*qx;
        double vqz =  rx*qy - ry*qx + rz*qt;
        //multiply quaternions qstar*vq
        double Cx = qstart*vqx + qstarx*vqt + qstary*vqz - qstarz*vqy;
        double Cy = qstart*vqy - qstarx*vqz + qstary*vqt + qstarz*vqx;
        double Cz = qstart*vqz + qstarx*vqy - qstary*vqx + qstarz*vqt;

        if (index == 1)     temp = Cx;
        else if (index ==2) temp = Cy;
        else                temp = Cz;
    }
    else // 
    {temp = 1;}
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

    if (PropertyManager->HasNode(tmp)) {
      cout << "Property " << tmp << " has already been successfully bound (late)." << endl;
    } else {
    PropertyManager->Tie( tmp, this, &FGFunction::GetValue);
    }
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
