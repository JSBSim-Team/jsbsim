/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGNozzle.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the nozzle object

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
08/24/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGNozzle.h"

static const char *IdSrc = "$Id: FGNozzle.cpp,v 1.14 2001/03/31 15:43:13 jberndt Exp $";
static const char *IdHdr = ID_NOZZLE;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGNozzle::FGNozzle(FGFDMExec* FDMExec, FGConfigFile* Nzl_cfg) : FGThruster(FDMExec)
{
  string token;

  Name = Nzl_cfg->GetValue("NAME");
  cout << "      Nozzle Name: " << Name << endl;
  Nzl_cfg->GetNextConfigLine();
  while (Nzl_cfg->GetValue() != "/FG_NOZZLE") {
    *Nzl_cfg >> token;
    if (token == "PE") {
      *Nzl_cfg >> PE;
      cout << "      Nozzle Exit Pressure = " << PE << endl;
    } else if (token == "EXPR") {
      *Nzl_cfg >> ExpR;
      cout << "      Nozzle Expansion Ratio = " << ExpR << endl;
    } else if (token == "NZL_EFF") {
      *Nzl_cfg >> nzlEff;
      cout << "      Nozzle Efficiency = " << nzlEff << endl;
    } else if (token == "DIAM") {
      *Nzl_cfg >> Diameter;
      cout << "      Nozzle Diameter = " << Diameter << endl;
    } else {
      cout << "Unhandled token in Nozzle config file: " << token << endl;
    }
  }

  Thrust = 0;

  if (debug_lvl & 2) cout << "Instantiated: FGNozzle" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGNozzle::~FGNozzle()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGNozzle" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGNozzle::Calculate(float CfPc)
{
  float pAtm = fdmex->GetAtmosphere()->GetPressure();
  
  Thrust = (CfPc + (PE - pAtm)*ExpR) * (Diameter / ExpR) * nzlEff;

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGNozzle::GetPowerRequired(void)
{
  return PE;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGNozzle::Debug(void)
{
    //TODO: Add your source code here
}

