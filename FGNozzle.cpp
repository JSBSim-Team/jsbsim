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

static const char *IdSrc = "$Id: FGNozzle.cpp,v 1.17 2001/04/20 13:11:02 jberndt Exp $";
static const char *IdHdr = ID_NOZZLE;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGNozzle::FGNozzle(FGFDMExec* FDMExec, FGConfigFile* Nzl_cfg) : FGThruster(FDMExec)
{
  string token;

  Name = Nzl_cfg->GetValue("NAME");
  Nzl_cfg->GetNextConfigLine();
  while (Nzl_cfg->GetValue() != "/FG_NOZZLE") {
    *Nzl_cfg >> token;
    if      (token == "PE")      *Nzl_cfg >> PE;
    else if (token == "EXPR")    *Nzl_cfg >> ExpR;
    else if (token == "NZL_EFF") *Nzl_cfg >> nzlEff;
    else if (token == "DIAM")    *Nzl_cfg >> Diameter;
    else cerr << "Unhandled token in Nozzle config file: " << token << endl;
  }

  if (debug_lvl > 0) {
    cout << "      Nozzle Name: " << Name << endl;
    cout << "      Nozzle Exit Pressure = " << PE << endl;
    cout << "      Nozzle Expansion Ratio = " << ExpR << endl;
    cout << "      Nozzle Efficiency = " << nzlEff << endl;
    cout << "      Nozzle Diameter = " << Diameter << endl;
  }

  Thrust = 0;
  Type = ttNozzle;
  Area2 = (Diameter*Diameter/4.0)*M_PI;
  AreaT = Area2/ExpR;

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
  Thrust = (CfPc * AreaT + (PE - pAtm)*Area2) * nzlEff;

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

