/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGCoefficient.cpp
 Author:       Jon S. Berndt
 Date started: 12/28/98
 Purpose:      Encapsulates the stability derivative class FGCoefficient;
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This class models the stability derivative coefficient lookup tables or
equations. Note that the coefficients need not be calculated each delta-t.

Note that the values in a row which index into the table must be the same value
for each column of data, so the first column of numbers for each altitude are
seen to be equal, and there are the same number of values for each altitude.

See the header file FGCoefficient.h for the values of the identifiers.

HISTORY
--------------------------------------------------------------------------------
12/28/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdio.h>

#include "FGCoefficient.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGPropertyManager.h"

#ifndef FGFS
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <iomanip.h>
#  else
#    include <iomanip>
#  endif
#else
#  include STL_IOMANIP
#endif

namespace JSBSim {

static const char *IdSrc = "$Id: FGCoefficient.cpp,v 1.64 2003/12/29 10:57:39 ehofman Exp $";
static const char *IdHdr = ID_COEFFICIENT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGCoefficient::FGCoefficient( FGFDMExec* fdex )
{
  FDMExec = fdex;
  State   = FDMExec->GetState();
  Table   = 0;
  
  PropertyManager = FDMExec->GetPropertyManager();
  
  Table = (FGTable*)0L;
  LookupR = LookupC = 0;
  numInstances = 0;
  rows = columns = 0;

  StaticValue  = 0.0;
  totalValue   = 0.0;
  bias = 0.0;
  gain = 1.0;
  SD = 0.0;

  filename.erase();
  description.erase();
  name.erase();
  method.erase();
  multparms.erase();
  multparmsRow.erase();
  multparmsCol.erase();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGCoefficient::~FGCoefficient()
{
  if (Table) delete Table;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGCoefficient::Load(FGConfigFile *AC_cfg)
{
  int start, end, n;
  string mult;

  if (AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    method = AC_cfg->GetValue("TYPE");
    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;
    if      (method == "EQUATION") type = EQUATION;
    else if (method == "TABLE")    type = TABLE;
    else if (method == "VECTOR")   type = VECTOR;
    else if (method == "VALUE")    type = VALUE;
    else                           type = UNKNOWN;

    if (type == VECTOR || type == TABLE) {
      *AC_cfg >> rows;
      if (type == TABLE) {
        *AC_cfg >> columns;
        Table = new FGTable(rows, columns);
      } else {
        Table = new FGTable(rows);
      }

      *AC_cfg >> multparmsRow;
      LookupR = PropertyManager->GetNode( multparmsRow );
    }

    if (type == TABLE) {
      *AC_cfg >> multparmsCol;

      LookupC = PropertyManager->GetNode( multparmsCol );
    }

    // Here, read in the line of the form (e.g.) FG_MACH|FG_QBAR|FG_ALPHA
    // where each non-dimensionalizing parameter for this coefficient is
    // separated by a | character

    string line=AC_cfg->GetCurrentLine();
    unsigned j=0;
    char tmp[255];
    for(unsigned i=0;i<line.length(); i++ ) {
      if( !isspace(line[i]) ) {
        tmp[j]=line[i];
        j++;
      }
    } 
    tmp[j]='\0'; multparms=tmp;  
    end  = multparms.length();

    n     = multparms.find("|");
    start = 0;
    if (multparms != string("none")) {
      while (n < end && n >= 0) {
        n -= start;
        mult = multparms.substr(start,n);
        multipliers.push_back( resolveSymbol( mult ) );
        start += n+1;
        n = multparms.find("|",start);
      }
      mult = multparms.substr(start,n);
      multipliers.push_back( resolveSymbol( mult ) );
      // End of non-dimensionalizing parameter read-in
    }
    AC_cfg->GetNextConfigLine();
    if (type == VALUE) {
      *AC_cfg >> StaticValue;
    } else if (type == VECTOR || type == TABLE) {
      *Table << *AC_cfg;
    } else {
      cerr << "Unimplemented coefficient type: " << type << endl;
    }

    AC_cfg->GetNextConfigLine();
    FGCoefficient::Debug(2);

    return true;
  } else {
    return false;
  }  
}



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(double rVal, double cVal)
{
  double Value;
  unsigned int midx;

  SD = Value = gain*Table->GetValue(rVal, cVal) + bias;

  for (midx=0; midx < multipliers.size(); midx++) {
      Value *= multipliers[midx]->getDoubleValue();
  }
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(double Val)
{
  double Value;
  
  SD = Value = gain*Table->GetValue(Val) + bias;
  
  for (unsigned int midx=0; midx < multipliers.size(); midx++) 
      Value *= multipliers[midx]->getDoubleValue();
  
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(void)
{
	double Value;

  SD = Value = gain*StaticValue + bias;

  for (unsigned int midx=0; midx < multipliers.size(); midx++)
    Value *= multipliers[midx]->getDoubleValue();

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::TotalValue(void)
{
  switch(type) {

  case UNKNOWN:
    totalValue = -1;
    break;

  case VALUE:
    totalValue = Value();
    break;

  case VECTOR:
    totalValue = Value( LookupR->getDoubleValue() );
    break;

  case TABLE:
    totalValue = Value( LookupR->getDoubleValue(),
                      LookupC->getDoubleValue() );
    break;

  case EQUATION:
    totalValue = 0.0;
    break;
  }
  return totalValue;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCoefficient::DisplayCoeffFactors(void)
{
  unsigned int i;

  cout << "   Non-Dimensionalized by: ";

  if (multipliers.size() == 0) {
    cout << "none" << endl;
  } else {
    for (i=0; i<multipliers.size(); i++) 
      cout << multipliers[i]->getName() << "  ";
  }
  cout << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGCoefficient::GetSDstring(void)
{
  char buffer[16];
  string value;

  sprintf(buffer,"%9.6f",SD);
  value = string(buffer);
  return value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCoefficient::bind(FGPropertyManager *parent)
{
  string mult;
  unsigned i;
  
  node = parent->GetNode(name,true);
  
  node->SetString("description",description);
  if (LookupR) node->SetString("row-parm",LookupR->getName() );
  if (LookupC) node->SetString("column-parm",LookupC->getName() );
  
  mult="";
  if (multipliers.size() == 0) 
    mult="none";
    
  for (i=0; i<multipliers.size(); i++) {
      mult += multipliers[i]->getName();
      if ( i < multipliers.size()-1 ) mult += " "; 
  }
  node->SetString("multipliers",mult);
  
  node->Tie("SD-norm",this,&FGCoefficient::GetSD );
  node->Tie("value-lbs",this,&FGCoefficient::GetValue );
  
  node->Tie("bias", this, &FGCoefficient::getBias,
                          &FGCoefficient::setBias );
                          
  node->Tie("gain", this, &FGCoefficient::getGain,
                          &FGCoefficient::setGain );

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCoefficient::unbind(void)
{
  node->Untie("SD-norm");
  node->Untie("value-lbs"); 
  node->Untie("bias");  
  node->Untie("gain");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyManager* FGCoefficient::resolveSymbol(string name)
{
  FGPropertyManager* tmpn;

  tmpn = PropertyManager->GetNode(name,false);
  if ( !tmpn ) {
    cerr << "Coefficient multipliers cannot create properties, check spelling?" << endl;
    exit(1);
  } 
  return tmpn; 
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

void FGCoefficient::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    
    if (from == 2) { // Loading
      cout << "\n   " << highint << underon << name << underoff << normint << endl;
      cout << "   " << description << endl;
      cout << "   " << method << endl;

      if (type == VECTOR || type == TABLE) {
        cout << "   Rows: " << rows << " ";
        if (type == TABLE) {
          cout << "Cols: " << columns;
        }
        cout << endl << "   Row indexing parameter: " << LookupR->getName() << endl;
      }

      if (type == TABLE) {
        cout << "   Column indexing parameter: " << LookupC->getName() << endl;
      }

      if (type == VALUE) {
        cout << "      Value = " << StaticValue << endl;
      } else if (type == VECTOR || type == TABLE) {
        Table->Print();
      }

      DisplayCoeffFactors();
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGCoefficient" << endl;
    if (from == 1) cout << "Destroyed:    FGCoefficient" << endl;
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

} // namespace JSBSim
