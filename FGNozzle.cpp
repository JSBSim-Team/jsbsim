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

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_ALGORITHM
#else
#  include <algorithm>
#endif

#include "FGNozzle.h"

static const char *IdSrc = "$Id: FGNozzle.cpp,v 1.23 2001/11/14 23:53:27 jberndt Exp $";
static const char *IdHdr = ID_NOZZLE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGNozzle::FGNozzle(FGFDMExec* FDMExec, FGConfigFile* Nzl_cfg) : FGThruster(FDMExec)
{
  string token;

  Name = Nzl_cfg->GetValue("NAME");
  Nzl_cfg->GetNextConfigLine();
  while (Nzl_cfg->GetValue() != string("/FG_NOZZLE")) {
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

double FGNozzle::Calculate(double CfPc)
{
  double pAtm = fdmex->GetAtmosphere()->GetPressure();
  Thrust = max((double)0.0, (CfPc * AreaT + (PE - pAtm)*Area2) * nzlEff);
  vFn(1) = Thrust;

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGNozzle::GetPowerRequired(void)
{
  return PE;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGNozzle::Debug(void)
{
    //TODO: Add your source code here
}

