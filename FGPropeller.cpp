/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropeller.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the propeller object

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

#include "FGPropeller.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropeller.cpp,v 1.4 2001/01/11 00:44:55 jsb Exp $";
static const char *IdHdr = ID_PROPELLER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropeller::FGPropeller(FGFDMExec* exec, FGConfigFile* Prop_cfg) : FGThruster(exec)
{
  string token;
  int rows, cols;

  PropName = Prop_cfg->GetValue("NAME");
  cout << "\n    Propeller Name: " << PropName << endl;
  Prop_cfg->GetNextConfigLine();
  while (Prop_cfg->GetValue() != "/PROPELLER") {
    *Prop_cfg >> token;
    if (token == "IXX") {
      *Prop_cfg >> Ixx;
      cout << "      IXX = " << Ixx << endl;
    } else if (token == "DIAMETER") {
      *Prop_cfg >> Diameter;
      cout << "      Diameter = " << Diameter << endl;
    } else if (token == "NUMBLADES") {
      *Prop_cfg >> numBlades;
      cout << "      Number of Blades  = " << numBlades << endl;
    } else if (token == "EFFICIENCY") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) Efficiency = new FGTable(rows);
	     else           Efficiency = new FGTable(rows, cols);
       *Efficiency << *Prop_cfg;
       cout << "      Efficiency: " <<  endl;
       Efficiency->Print();
    } else if (token == "C_THRUST") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) cThrust = new FGTable(rows);
	     else           cThrust = new FGTable(rows, cols);
       *cThrust << *Prop_cfg;
       cout << "      Thrust Coefficient: " <<  endl;
       cThrust->Print();
    } else if (token == "C_POWER") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) cPower = new FGTable(rows);
	     else           cPower = new FGTable(rows, cols);
       *cPower << *Prop_cfg;
       cout << "      Power Coefficient: " <<  endl;
       cPower->Print();
    } else {
      cout << "Unhandled token in Propeller config file: " << token << endl;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropeller::~FGPropeller(void)
{
  if (Efficiency) delete Efficiency;
  if (cThrust)    delete cThrust;
  if (cPower)     delete cPower;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropeller::Calculate(void)
{
  FGThruster::Calculate();
  float Vel = (fdmex->GetTranslation()->GetUVW())(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

