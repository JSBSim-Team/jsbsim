/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTurbine.cpp
 Author:       Jon S. Berndt
 Date started: 08/23/2002
 Purpose:      This module models a turbine engine.

 ------------- Copyright (C) 2002  Jon S. Berndt (jsb@hal-pc.org) --------------

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

This class descends from the FGEngine class and models a Turbine engine based
on parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
08/23/2002  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGTurbine.h"


static const char *IdSrc = "$Id: FGTurbine.cpp,v 1.4 2002/10/01 12:04:07 apeden Exp $";
static const char *IdHdr = ID_TURBINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGTurbine::FGTurbine(FGFDMExec* exec, FGConfigFile* cfg) : FGEngine(exec)
{
  Load(cfg);
  PowerCommand=0;
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTurbine::~FGTurbine()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Calculate(double dummy)
{
  double idle,mil,aug;
  double throttle=FCS->GetThrottlePos(EngineNumber);
  double dt=State->Getdt();
  if( dt > 0 ) {
    PowerCommand+=dt*PowerLag( PowerCommand, 
                         ThrottleToPowerCommand(throttle) );
    if(PowerCommand > 100 )
      PowerCommand=100;
    else if(PowerCommand < 0 )
      PowerCommand=0;
                    
  } else {
    PowerCommand=ThrottleToPowerCommand(throttle);
  }                         
  
  mil=MaxMilThrust*ThrustTables[1]->TotalValue();
  
  if( PowerCommand <= 50 ) {
    idle=MaxMilThrust*ThrustTables[0]->TotalValue();
    Thrust = idle + (mil-idle)*PowerCommand*0.02;
  } else {
    aug=MaxAugThrust*ThrustTables[2]->TotalValue();
    Thrust = mil + (aug-mil)*(PowerCommand-50)*0.02;
  }    
  
  ConsumeFuel();
  
  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::ThrottleToPowerCommand(double throttle) {
  if( throttle <= 0.77 ) 
    return 64.94*throttle;
  else
    return 217.38*throttle - 117.38;
}      

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::PowerLag(double actual_power, double power_command) {
  double t, p2;
  if( power_command >= 50 ) {
    if( actual_power >= 50 ) {
      t=5;
      p2=power_command;
    } else {
      p2=60;
      t=rtau(p2-actual_power);
    }
  } else {
    if( actual_power >= 50 ) {
      t=5;
      p2=40;
    } else {
      p2=power_command;
      t=rtau(p2-actual_power);
    }
  }
  return t*(p2-actual_power);
}    
 
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::rtau(double delta_power) {
  if( delta_power <= 25 ) 
    return 1.0;
  else if ( delta_power >= 50)
    return 0.1;
  else
    return 1.9-0.036*delta_power;
}
        
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doInlet(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doCompressor(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doBleedDuct(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doCombustor(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doTurbine(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doConvergingNozzle(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::doTransition(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


bool FGTurbine::Load(FGConfigFile *Eng_cfg)
{
  int i;
  string token;
  Name = Eng_cfg->GetValue("NAME");
  cout << Name << endl;
  Eng_cfg->GetNextConfigLine();
  *Eng_cfg >> token >> MaxMilThrust;
  *Eng_cfg >> token >> MaxAugThrust;
  i=0;
  while( Eng_cfg->GetValue() != string("/FG_TURBINE") && i < 10){
    ThrustTables.push_back( new FGCoefficient(FDMExec) );
    ThrustTables.back()->Load(Eng_cfg);
    i++;
  }

  return true;
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

void FGTurbine::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTurbine" << endl;
    if (from == 1) cout << "Destroyed:    FGTurbine" << endl;
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

