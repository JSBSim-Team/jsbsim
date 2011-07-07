/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       Aeromatic.cpp
 Author:       David Culp
 Date started: 04/26/05
 Purpose:      Encapsulization of the Aero-Matic web application
 Called by:    user application

 ------------ Copyright (C) 2005  David Culp (davidculp2@comcast.net)----------

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
Encapsulates the Aero-Matic web application. This class will generate config-
uration files for JSBSim.

HISTORY
--------------------------------------------------------------------------------
04/26/05   DPC   Created
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
This class may eventually get out of sync with the Aero-Matic web application on
which it is based.  Improvements to the web application may not be added here,
and vice-versa.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "Aeromatic.h"
#include <fstream>
#include <iomanip>
//namespace JSBSim {

//static const char *IdSrc = "$Id: Aeromatic.cpp,v 1.1.1.1 
//static const char *IdHdr = ID_AEROMATIC;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


Aeromatic::Aeromatic() {
  Reset();
}

Aeromatic::~Aeromatic() {
}

string Aeromatic::PrintEngine() {
  string filename = EngineName + ".xml";
  ofstream f;
  f.open(filename.c_str(), ios::out);
  f << fixed << setw(7) << setprecision(2);
  f << "<?xml version=\"1.0\"?>\n";
  f << "<!--\n  File:     " << filename << endl;
  f << "  Author:   Aero-Matic v " << AEROMATIC_VERSION << "\n\n";
  f << "  Inputs:\n";
  f << "    name:           " << EngineName << endl;
  switch(eType) {
    case etPiston:    f << "    type:           piston\n"; break;
    case etTurbine:   f << "    type:           turbine\n"; break;
    case etTurboprop: f << "    type:           turboprop\n"; break;
    case etRocket:    f << "    type:           rocket\n"; break;
  }  
  switch(eType) {
    case etPiston:    f << "    power:          " << enginePower << " hp\n"; break;
    case etTurbine:   f << "    thrust:         " << engineThrust << " lb\n"; break;
    case etTurboprop: f << "    power:          " << enginePower << " hp\n"; break;
    case etRocket:    f << "    thrust:         " << engineThrust << " lb\n"; break;
  }  
  if(augmentation)
    f << "    augmented?      yes\n";
  else
    f << "    augmented?      no\n";
  if(injection)
    f << "    injected?       yes\n";
  else
    f << "    injected?       no\n";
  f << "-->\n\n"; 


//************************************************
//*  MakePiston                                  *
//************************************************
switch (eType) {
  case etPiston: {
  double displacement = enginePower * 1.6;

  f << setw(7) << setprecision(2);
  f << "<piston_engine name=\"" << EngineName << "\">\n";
  f << "  <minmp unit=\"INHG\">      " << 6.0 << " </minmp>\n";
  f << "  <maxmp unit=\"INHG\">      " << 30.0 << " </maxmp>\n";
  f << "  <displacement unit=\"IN3\">  " << displacement << " </displacement>\n";
  f << "  <maxhp>        " << enginePower << " </maxhp>\n";
  f << "  <cycles>         2.0 </cycles>\n";
  f << "  <idlerpm>      700.0 </idlerpm>\n";
  f << "  <maxthrottle>    1.0 </maxthrottle>\n";
  f << "  <minthrottle>    0.2 </minthrottle>\n";
  f << "</piston_engine>\n";
  break;
  }


//************************************************
//*  MakeTurbine                                 *
//************************************************
  case etTurbine: {

  double maxthrust = engineThrust * 1.5;

  f << "<turbine_engine name=\"" <<EngineName<< "\">\n";  
  f << "  <milthrust>   " <<engineThrust<< " </milthrust>\n";
  if (augmentation) f << "  <maxthrust>   " <<maxthrust<< " </maxthrust>\n";
  f << "  <bypassratio>     0.0 </bypassratio>\n";
  f << "  <tsfc>            0.8 </tsfc>\n";
  f << "  <atsfc>           1.7 </atsfc>\n";
  f << "  <idlen1>         30.0 </idlen1>\n";
  f << "  <idlen2>         60.0 </idlen2>\n";
  f << "  <maxn1>         100.0 </maxn1>\n";
  f << "  <maxn2>         100.0 </maxn2>\n";
  if (augmentation) {
    f << "  <augmented>         1 </augmented>\n";
    f << "  <augmethod>         1 </augmethod>\n";
  } else {
    f << "  <augmented>         0 </augmented>\n";
    f << "  <augmethod>         1 </augmethod>\n";
  }
  if (injection) {
    f << "  <injected>          1 </injected>\n";
  } else {
    f << "  <injected>          0 </injected>\n";
  }
  f << endl;

  f << "  <function name=\"IdleThrust\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "         -10000     0     10000   20000   30000   40000   50000\n";
  f << "     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.1467\n";
  f << "     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342\n";
  f << "     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203\n";
  f << "     0.6  0.0     0.0     0.0     0.0     0.0276  0.0718  0.1073\n";
  f << "     0.8  0.0     0.0     0.0     0.0     0.0474  0.0868  0.0900\n";
  f << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0552  0.0800\n";
  f << "    </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";

  f << "  <function name=\"MilThrust\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "          -10000       0   10000   20000   30000   40000   50000\n";
  f << "     0.0   1.2600  1.0000  0.7400  0.5340  0.3720  0.2410  0.1490\n";
  f << "     0.2   1.1710  0.9340  0.6970  0.5060  0.3550  0.2310  0.1430\n";
  f << "     0.4   1.1500  0.9210  0.6920  0.5060  0.3570  0.2330  0.1450\n";
  f << "     0.6   1.1810  0.9510  0.7210  0.5320  0.3780  0.2480  0.1540\n";
  f << "     0.8   1.2580  1.0200  0.7820  0.5820  0.4170  0.2750  0.1700\n";
  f << "     1.0   1.3690  1.1200  0.8710  0.6510  0.4750  0.3150  0.1950\n";
  f << "     1.2   1.4850  1.2300  0.9750  0.7440  0.5450  0.3640  0.2250\n";
  f << "     1.4   1.5941  1.3400  1.0860  0.8450  0.6280  0.4240  0.2630\n";
  f << "    </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";

 if (augmentation) {
  f << "  <function name=\"AugThrust\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "           -10000       0   10000   20000   30000   40000   50000\n";
  f << "     0.0    1.1816  1.0000  0.8184  0.6627  0.5280  0.3756  0.2327\n";
  f << "     0.2    1.1308  0.9599  0.7890  0.6406  0.5116  0.3645  0.2258\n";
  f << "     0.4    1.1150  0.9474  0.7798  0.6340  0.5070  0.3615  0.2240\n";
  f << "     0.6    1.1284  0.9589  0.7894  0.6420  0.5134  0.3661  0.2268\n";
  f << "     0.8    1.1707  0.9942  0.8177  0.6647  0.5309  0.3784  0.2345\n";
  f << "     1.0    1.2411  1.0529  0.8648  0.7017  0.5596  0.3983  0.2467\n";
  f << "     1.2    1.3287  1.1254  0.9221  0.7462  0.5936  0.4219  0.2614\n";
  f << "     1.4    1.4365  1.2149  0.9933  0.8021  0.6360  0.4509  0.2794\n";
  f << "     1.6    1.5711  1.3260  1.0809  0.8700  0.6874  0.4860  0.3011\n";
  f << "     1.8    1.7301  1.4579  1.1857  0.9512  0.7495  0.5289  0.3277\n";
  f << "     2.0    1.8314  1.5700  1.3086  1.0474  0.8216  0.5786  0.3585\n";
  f << "     2.2    1.9700  1.6900  1.4100  1.2400  0.9100  0.6359  0.3940\n";
  f << "     2.4    2.0700  1.8000  1.5300  1.3400  1.0000  0.7200  0.4600\n";
  f << "     2.6    2.2000  1.9200  1.6400  1.4400  1.1000  0.8000  0.5200\n";
  f << "    </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";
 }

 if (injection) {
  f << "  <function name=\"WaterFactor\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "            0       50000\n";
  f << "     0.0    1.2000  1.2000\n";
  f << "     1.0    1.2000  1.2000\n";
  f << "    </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";
 }

 f << "</turbine_engine>\n";
 break;
 }


//************************************************
//*  MakeTurboprop                               *
//************************************************
  case etTurboprop: {


  // estimate thrust from power
  engineThrust = 2.24 * enginePower;

  f << "<turbine_engine name=\"" <<EngineName<< "\">\n";  
  f << "  <milthrust>    " << engineThrust << " </milthrust>\n";
  f << "  <bypassratio>     0.0  </bypassratio>\n";
  f << "  <tsfc>            0.55 </tsfc>\n";
  f << "  <idlen2>         60.0  </idlen2>\n";
  f << "  <maxn2>         100.0  </maxn2>\n";
  f << "  <augmented>         0  </augmented>\n";
  f << "  <injected>          0  </injected>\n\n";

  f << "  <function name=\"IdleThrust\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "         -10000       0   10000   20000   30000   40000   50000\n";
  f << "     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.1467\n";
  f << "     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342\n";
  f << "     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203\n";
  f << "     0.6  0.0     0.0     0.0     0.0276  0.0718  0.0430  0.0\n";
  f << "     0.8  0.0     0.0     0.0     0.0     0.0174  0.0086  0.0\n";
  f << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0\n";
  f << "   </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";

  f << "  <function name=\"MilThrust\">\n";
  f << "   <table>\n";
  f << "    <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";  
  f << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n";  
  f << "    <tableData>\n";
  f << "         -10000       0   10000   20000   30000   40000   50000\n";
  f << "     0.0  1.1260  1.0000  0.7400  0.5340  0.3720  0.2410  0.1490\n";
  f << "     0.2  1.1000  0.9340  0.6970  0.5060  0.3550  0.2310  0.1430\n";
  f << "     0.4  1.0000  0.6410  0.6120  0.4060  0.3570  0.2330  0.1450\n";
  f << "     0.6  0.4430  0.3510  0.2710  0.2020  0.1780  0.1020  0.0640\n";
  f << "     0.8  0.0240  0.0200  0.0160  0.0130  0.0110  0.0100  0.0\n";
  f << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0\n";
  f << "    </tableData>\n";
  f << "   </table>\n";
  f << "  </function>\n\n";

  f << "</turbine_engine>\n";
  break;
  }


//************************************************
//*  MakeRocket                                  *
//************************************************
  case etRocket: {

  f << "<rocket_engine name=\"" << EngineName << "\">\n";
  f << "  <shr>              1.23 </shr>\n";
  f << "  <max_pc>       86556.00 </max_pc>\n";
  f << "  <variance>         0.10 </variance>\n";
  f << "  <prop_eff>         0.67 </prop_eff>\n";
  f << "  <maxthrottle>      1.00 </maxthrottle>\n";
  f << "  <minthrottle>      0.40 </minthrottle>\n";
  f << "  <slfuelflowmax>   91.50 </slfuelflowmax>\n";
  f << "  <sloxiflowmax>   105.20 </sloxiflowmax>\n";
  f << "</rocket_engine>\n";
  }
  }//switch
  
  f << flush;
  f.close();
  return filename;
}

string Aeromatic::PrintProp() {

  // find propeller rpm which gives a tip mach of 0.88
  // (static at sea level)
  double maxrpm = 18763.0 / diameter;

  double gearratio = engineRPM / maxrpm;

  double maxrps = maxrpm / 60.0;
  double rps2 = maxrps * maxrps;
  double rps3 = rps2 * maxrps;
  double d4 = diameter * diameter * diameter * diameter;
  double d5 = d4 * diameter;
  double rho = 0.002378;

  // static power and thrust coefficients
  double cp0 = enginePower * 550.0 / rho / rps3 / d5;
  double ct0 = cp0 * 0.86;

  double static_thrust = ct0 * rho * rps2 * d4;

  // estimate number of blades
  int blades;
  if (cp0 < 0.035) {
    blades = 2;
  } else if (cp0 >= 0.06) {
    blades = 4;
  } else {
    blades = 3; 
  }

  // estimate moment of inertia
  double L = diameter / 2;       // length each blade (feet)
  double M = L * 0.09317;        // mass each blade (slugs)
  if (L < 1) { M = L * 0.003; }  // mass for tiny props
  double ixx = blades * (0.33333 * M * L * L);

  //*****************************************************
  //                                                    *
  //  Print XML file                                    *
  //                                                    *
  //*****************************************************
  string filename = PropName + ".xml";
  ofstream f;
  f.open(filename.c_str(), ios::out);
  f << fixed << setw(7) << setprecision(2);
  f << "<xml version=\"1.0\"?>\n";
  f << "<!-- Generated by Aero-Matic v " <<AEROMATIC_VERSION<< "\n\n";
  f << "     Inputs:\n"; 
  f << "                horsepower: " << enginePower << endl;
  if (fixedpitch)
    f << "                     pitch: fixed\n";
  else
    f << "                     pitch: variable\n";
  f << "            max engine rpm: " << engineRPM << endl;
  f << "        prop diameter (ft): " << diameter << endl;
  f << endl << "     Outputs:\n";
  f << "              max prop rpm: " << maxrpm << endl;
  f << "                gear ratio: " << gearratio << endl;
  f << fixed << setw(7) << setprecision(4);
  f << "                       Cp0: " << cp0 << endl;
  f << "                       Ct0: " << ct0 << endl;
  f << fixed << setw(7) << setprecision(2);
  f << "       static thrust (lbs): " << static_thrust << endl;
  f << "-->\n\n";

  f << "<propeller name=\"" << PropName << "\">\n";
  f << "  <ixx> " << ixx << " </ixx>\n";
  f << "  <diameter unit=\"IN\"> " << diameter * 12 << " </diameter>\n";
  f << "  <numblades> " << blades << " </numblades>\n";
  f << "  <gearratio> " << gearratio << " </gearratio>\n";

  if (fixedpitch) {
    f << "  <minpitch> 20 </minpitch>\n";
    f << "  <maxpitch> 20 </maxpitch>\n";
  } else {
    f << "  <minpitch> 10 </minpitch>\n";
    f << "  <maxpitch> 45 </maxpitch>\n";
  }

  if (!fixedpitch) {
    f << "  <minrpm> " << maxrpm * 0.8 << " </minrpm>\n";
    f << "  <maxrpm> " << maxrpm << " </maxrpm>\n";
  }

  f << endl;
  f << setprecision(4);

  if (fixedpitch) {          
    f << "  <function name=\"C_THRUST\">\n";
    f << "    <table>\n";
    f << "      <independentVar>propulsion/advance-ratio</independentVar>\n";
    f << "      <tableData>\n";
    f << "       0.0  " << ct0 * 1.0 << endl;
    f << "       0.1  " << ct0 * 0.959 << endl;
    f << "       0.2  " << ct0 * 0.917 << endl;
    f << "       0.3  " << ct0 * 0.844 << endl;
    f << "       0.4  " << ct0 * 0.758 << endl;
    f << "       0.5  " << ct0 * 0.668 << endl;
    f << "       0.6  " << ct0 * 0.540 << endl;
    f << "       0.7  " << ct0 * 0.410 << endl;
    f << "       0.8  " << ct0 * 0.222 << endl;
    f << "       1.0  " << ct0 * -0.075 << endl;
    f << "       1.2  " << ct0 * -0.394 << endl;
    f << "       1.4  " << ct0 * -0.708 << endl;
    f << "      </tableData>\n";
    f << "    </table>\n";
    f << "  </function>\n";
  } else {                      // variable pitch
    f << "  <function name=\"C_THRUST\">\n";
    f << "    <table>\n";
    f << "      <independentVar lookup=\"row\">propulsion/advance-ratio</independentVar>\n";
    f << "      <independentVar lookup=\"column\">propulsion/blade-angle</independentVar>\n";
    f << "      <tableData>\n";
    f << "                10         15         20         25         30         35         40         45\n";
    f << "       0.0     "<< ct0*1.000 << "     " << ct0*1.286 << "     " << ct0*1.435 << "     " << ct0*1.455 << "     " << ct0*1.527 << "     " << ct0*1.583 << "     " << ct0*1.619 << "     " << ct0*1.637 << endl;
    
    f << "       0.1     "<< ct0*0.882 << "     " << ct0*1.182 << "     " << ct0*1.419 << "     " << ct0*1.436 << "     " << ct0*1.509 << "     " << ct0*1.573 << "     " << ct0*1.610 << "     " << ct0*1.637 << endl;
    
    f << "       0.2     "<< ct0*0.727 << "     " << ct0*1.054 << "     " << ct0*1.363 << "     " << ct0*1.419 << "     " << ct0*1.491 << "     " << ct0*1.555 << "     " << ct0*1.601 << "     " << ct0*1.628 << endl;
    
    f << "       0.3     "<< ct0*0.555 << "     " << ct0*0.908 << "     " << ct0*1.273 << "     " << ct0*1.391 << "     " << ct0*1.473 << "     " << ct0*1.537 << "     " << ct0*1.573 << "     " << ct0*1.624 << endl;
    
    f << "       0.4     "<< ct0*0.373 << "     " << ct0*0.754 << "     " << ct0*1.155 << "     " << ct0*1.373 << "     " << ct0*1.455 << "     " << ct0*1.519 << "     " << ct0*1.555 << "     " << ct0*1.619 << endl;
    
    f << "       0.5     "<< ct0*0.173 << "     " << ct0*0.591 << "     " << ct0*1.000 << "     " << ct0*1.337 << "     " << ct0*1.427 << "     " << ct0*1.501 << "     " << ct0*1.539 << "     " << ct0*1.615 << endl;
    
    f << "       0.6     "<< ct0*0.000 << "     " << ct0*0.422 << "     " << ct0*0.836 << "     " << ct0*1.218 << "     " << ct0*1.399 << "     " << ct0*1.465 << "     " << ct0*1.524 << "     " << ct0*1.609 << endl;
    
    f << "       0.7    "<< ct0*-0.227 << "     " << ct0*0.218 << "     " << ct0*0.655 << "     " << ct0*1.137 << "     " << ct0*1.368 << "     " << ct0*1.445 << "     " << ct0*1.483 << "     " << ct0*1.591 << endl;
    
    f << "       0.8    "<< ct0*-0.373 << "     " << ct0*0.028 << "     " << ct0*0.463 << "     " << ct0*0.908 << "     " << ct0*1.296 << "     " << ct0*1.427 << "     " << ct0*1.455 << "     " << ct0*1.568 << endl;
    
    f << "       0.9    "<< ct0*-0.637 << "    " << ct0*-0.033 << "     " << ct0*0.264 << "     " << ct0*0.727 << "     " << ct0*1.173 << "     " << ct0*1.391 << "     " << ct0*1.437 << "     " << ct0*1.563 << endl;
    
    f << "       1.0    "<< ct0*-0.808 << "    " << ct0*-0.363 << "     " << ct0*0.064 << "     " << ct0*0.545 << "     " << ct0*1.000 << "     " << ct0*1.337 << "     " << ct0*1.401 << "     " << ct0*1.545 << endl;
    
    f << "       1.6    "<< ct0*-1.997 << "    " << ct0*-1.545 << "    " << ct0*-1.178 << "    " << ct0*-0.545 << "    " << ct0*-0.092 << "     " << ct0*0.399 << "     " << ct0*0.890 << "     " << ct0*1.381 << endl;
    
    f << "       2.0    "<< ct0*-2.728 << "    " << ct0*-2.438 << "    " << ct0*-2.095 << "    " << ct0*-1.319 << "    " << ct0*-0.864 << "    " << ct0*-0.273 << "     " << ct0*0.273 << "     " << ct0*0.908 << endl;
    
    f << "       3.0    "<< ct0*-3.764 << "    " << ct0*-3.437 << "    " << ct0*-3.093 << "    " << ct0*-2.307 << "    " << ct0*-1.866 << "    " << ct0*-1.272 << "    " << ct0*-0.709 << "    " << ct0*-0.098 << endl;
    f << "      </tableData>\n";
    f << "    </table>\n";
    f << "  </function>\n";
  }

  f << endl;
  if (fixedpitch) {       
    f << "  <function name=\"C_POWER\">\n";
    f << "    <table>\n";
    f << "      <independentVar>propulsion/advance-ratio</independentVar>\n";
    f << "      <tableData>\n";
    f << "       0.0  " << cp0 * 1.0 << endl;
    f << "       0.1  " << cp0 * 0.990 << endl;
    f << "       0.2  " << cp0 * 0.976 << endl;
    f << "       0.3  " << cp0 * 0.953 << endl;
    f << "       0.4  " << cp0 * 0.898 << endl;
    f << "       0.5  " << cp0 * 0.823 << endl;
    f << "       0.6  " << cp0 * 0.755 << endl;
    f << "       0.7  " << cp0 * 0.634 << endl;
    f << "       0.8  " << cp0 * 0.518 << endl;
    f << "       1.0  " << cp0 * 0.185 << endl;
    f << "       1.2  " << cp0 * -0.296 << endl;
    f << "       1.4  " << cp0 * -0.890 << endl;
    f << "       1.6  " << cp0 * -1.511 << endl;
    f << "      </tableData>\n";
    f << "    </table>\n";
    f << "  </function>\n";
  } else {                      // variable pitch
    f << "  <function name=\"C_POWER\">\n";
    f << "    <table>\n";
    f << "      <independentVar lookup=\"row\">propulsion/advance-ratio</independentVar>\n";
    f << "      <independentVar lookup=\"column\">propulsion/blade-angle</independentVar>\n";
    f << "      <tableData>\n";
    f << "                10         45\n";
    f << "       0.0    " << cp0 * 1.0 << "     " << cp0 * 3.0 << endl;
    f << "       0.1    " << cp0 * 1.0 << "     " << cp0 * 3.0 << endl;
    f << "       0.2    " << cp0 * 0.953 << "     " << cp0 * 2.859 << endl;
    f << "       0.3    " << cp0 * 0.906 << "     " << cp0 * 2.718 << endl;
    f << "       0.4    " << cp0 * 0.797 << "     " << cp0 * 2.391 << endl;
    f << "       0.5    " << cp0 * 0.656 << "     " << cp0 * 1.968 << endl;
    f << "       0.6    " << cp0 * 0.531 << "     " << cp0 * 1.593 << endl;
    f << "       0.7    " << cp0 * 0.313 << "     " << cp0 * 0.939 << endl;
    f << "       0.8    " << cp0 * 0.125 << "     " << cp0 * 0.375 << endl;
    f << "       1.0   " << cp0 * -0.375 << "     " << cp0 * 0.144 << endl;
    f << "       1.2   " << cp0 * -1.093 << "     " << cp0 * 0.000 << endl;
    f << "       1.4   " << cp0 * -2.030 << "     " << cp0 * 0.250 << endl;
    f << "       1.6   " << cp0 * -3.0 << "    " << cp0 * -0.022 << endl;
    f << "       1.8   " << cp0 * -4.0 << "    " << cp0 * -0.610 << endl;
    f << "       2.0   " << cp0 * -5.0 << "    " << cp0 * -1.220 << endl;
    f << "       2.2   " << cp0 * -6.0 << "    " << cp0 * -1.830 << endl;
    f << "       2.4   " << cp0 * -7.0 << "    " << cp0 * -2.440 << endl;
    f << "      </tableData>\n";
    f << "    </table>\n";
    f << "  </function>\n";
  }

  f << "\n</propeller>\n";
  f << flush;
  f.close();
  return filename;
}

string Aeromatic::PrintAero() {

  int i,j,k;

  // first, estimate wing loading in psf
  double wingloading;
  switch(aType) { 
    case atGlider:      wingloading = 7.0; break;   
    case atLtSingle:    wingloading = 14.0; break;  
    case atLtTwin:      wingloading = 29.0; break;  
    case atRacer:       wingloading = 45.0; break;  
    case atSEFighter:   wingloading = 95.0; break;  
    case at2EFighter:   wingloading = 100.0; break; 
    case at2ETransport: wingloading = 110.0; break; 
    case at3ETransport: wingloading = 110.0; break; 
    case at4ETransport: wingloading = 110.0; break; 
    case atMEProp:      wingloading = 57.0; break;  
  }

  // if no wing area given, use wing loading to estimate
  bool wingarea_input = false;
  if (wingarea == 0.0) {
     wingarea = MTOW / wingloading;
   }
   else {
     wingarea_input = true;
     wingloading = MTOW / wingarea;
   }

  // calculate wing chord
  double wingchord = wingarea / wingspan;

  // calculate aspect ratio
  double aspectratio = wingspan / wingchord;

  // calculate half-span
  double halfspan = wingspan / 2.0;

  // estimate horizontal tail area
  double htailarea = 0.0;
  if (htailarea == 0.0) {
    switch(aType) {
      case atGlider:      htailarea = wingarea * 0.12; break;
      case atLtSingle:    htailarea = wingarea * 0.16; break;
      case atLtTwin:      htailarea = wingarea * 0.16; break;
      case atRacer:       htailarea = wingarea * 0.17; break;
      case atSEFighter:   htailarea = wingarea * 0.20; break;
      case at2EFighter:   htailarea = wingarea * 0.20; break;
      case at2ETransport: htailarea = wingarea * 0.25; break;
      case at3ETransport: htailarea = wingarea * 0.25; break;
      case at4ETransport: htailarea = wingarea * 0.25; break;
      case atMEProp:      htailarea = wingarea * 0.16; break;
    }
  }

  // estimate distance from CG to horizontal tail aero center
  double htailarm = 0.0;
  if (htailarm == 0.0) {
    switch(aType) {
      case atGlider:      htailarm = length * 0.60; break;
      case atLtSingle:    htailarm = length * 0.52; break;
      case atLtTwin:      htailarm = length * 0.50; break;
      case atRacer:       htailarm = length * 0.60; break;
      case atSEFighter:   htailarm = length * 0.40; break;
      case at2EFighter:   htailarm = length * 0.40; break;
      case at2ETransport: htailarm = length * 0.45; break;
      case at3ETransport: htailarm = length * 0.45; break;
      case at4ETransport: htailarm = length * 0.45; break;
      case atMEProp:      htailarm = length * 0.50; break;
    }
  }

  // estimate vertical tail area
  double vtailarea = 0.0;
  if (vtailarea == 0.0) {
    switch(aType) {
      case atGlider:      vtailarea = wingarea * 0.10; break;
      case atLtSingle:    vtailarea = wingarea * 0.10; break;
      case atLtTwin:      vtailarea = wingarea * 0.18; break;
      case atRacer:       vtailarea = wingarea * 0.10; break;
      case atSEFighter:   vtailarea = wingarea * 0.12; break;
      case at2EFighter:   vtailarea = wingarea * 0.18; break;
      case at2ETransport: vtailarea = wingarea * 0.20; break;
      case at3ETransport: vtailarea = wingarea * 0.20; break;
      case at4ETransport: vtailarea = wingarea * 0.20; break;
      case atMEProp:      vtailarea = wingarea * 0.18; break;
    }
  }

  // estimate distance from CG to vertical tail aero center
  double vtailarm = 0.0;
  if (vtailarm == 0.0) {
    switch(aType) {
      case atGlider:      vtailarm = length * 0.60; break;
      case atLtSingle:    vtailarm = length * 0.50; break;
      case atLtTwin:      vtailarm = length * 0.50; break;
      case atRacer:       vtailarm = length * 0.60; break;
      case atSEFighter:   vtailarm = length * 0.40; break;
      case at2EFighter:   vtailarm = length * 0.40; break;
      case at2ETransport: vtailarm = length * 0.45; break;
      case at3ETransport: vtailarm = length * 0.45; break;
      case at4ETransport: vtailarm = length * 0.45; break;
      case atMEProp:      vtailarm = length * 0.50; break;
    }
  }

//***** MOMENTS OF INERTIA ******************************

// use Roskam's formulae to estimate moments of inertia
  double Rx, Ry, Rz;
  switch(aType) {       // moment-of-inertia factors 
    case atGlider:      Rx = 0.34; Ry = 0.33; Rz = 0.47; break;
    case atLtSingle:    Rx = 0.27; Ry = 0.36; Rz = 0.42; break;
    case atLtTwin:      Rx = 0.27; Ry = 0.35; Rz = 0.45; break;
    case atRacer:       Rx = 0.27; Ry = 0.36; Rz = 0.42; break;
    case atSEFighter:   Rx = 0.27; Ry = 0.35; Rz = 0.40; break;
    case at2EFighter:   Rx = 0.29; Ry = 0.34; Rz = 0.41; break;
    case at2ETransport: Rx = 0.25; Ry = 0.38; Rz = 0.46; break;
    case at3ETransport: Rx = 0.25; Ry = 0.36; Rz = 0.47; break;
    case at4ETransport: Rx = 0.32; Ry = 0.34; Rz = 0.47; break;
    case atMEProp:      Rx = 0.32; Ry = 0.35; Rz = 0.47; break;
  }

  double rawixx = (MTOW / 32.2)* pow((Rx * wingspan / 2.0), 2);
  double rawiyy = (MTOW / 32.2)* pow((Ry * length / 2.0), 2);
  double rawizz = (MTOW / 32.2)* pow((Rz * ((wingspan + length)/2.0) / 2.0), 2);
  // assume 4 degree angle between longitudinal and inertial axes
  // double rawixz = abs(rawizz - rawixx) * 0.06975647;

  // originally I increased the raw MOI's to make up for lack of
  // joystick feel, but here I'll use the raw values
  double ixx = rawixx;
  double iyy = rawiyy;
  double izz = rawizz;
  double ixz = 0;


  //***** EMPTY WEIGHT *********************************

  // estimate empty weight
  double emptyweight = 0.0;
  if (emptyweight == 0.0) {
    switch(aType) {
      case atGlider:      emptyweight = MTOW * .84;
      case atLtSingle:    emptyweight = MTOW * .62;
      case atLtTwin:      emptyweight = MTOW * .61;
      case atRacer:       emptyweight = MTOW * .61;
      case atSEFighter:   emptyweight = MTOW * .53;
      case at2EFighter:   emptyweight = MTOW * .50;
      case at2ETransport: emptyweight = MTOW * .55;
      case at3ETransport: emptyweight = MTOW * .52;
      case at4ETransport: emptyweight = MTOW * .49;
      case atMEProp:      emptyweight = MTOW * .60;
    }
  }

  //***** CG LOCATION ***********************************

  double cglocx = (length - htailarm) * 12.0;
  double cglocy = 0;
  double cglocz = -(length / 40.0) * 12;

  //***** AERO REFERENCE POINT **************************

  double aerorpx = cglocx;
  double aerorpy = 0;
  double aerorpz = 0;

  //***** PILOT EYEPOINT *********************************

  // place pilot's eyepoint based on airplane type
  double eyeptlocx, eyeptlocy, eyeptlocz;
  switch(aType) {
    case atGlider:      { eyeptlocx = (length * 0.19) * 12.0;
                          eyeptlocy = 0.0;
                          eyeptlocz = 9.0; break; }
    case atLtSingle:    { eyeptlocx = (length * 0.13) * 12.0;
                          eyeptlocy = -18.0;
                          eyeptlocz = 45.0; break; }
    case atLtTwin:      { eyeptlocx = (length * 0.17) * 12.0;
                          eyeptlocy = -18.0;
                          eyeptlocz = 45.0; break; }
    case atRacer:       { eyeptlocx = (length * 0.28) * 12.0;
                          eyeptlocy = 0.0;
                          eyeptlocz = 40.0; break; }
    case atSEFighter:   { eyeptlocx = (length * 0.20) * 12.0;
                          eyeptlocy = 0.0;
                          eyeptlocz = 36.0; break; }
    case at2EFighter:   { eyeptlocx = (length * 0.20) * 12.0;
                          eyeptlocy = 0.0;
                          eyeptlocz = 38.0; break; }
    case at2ETransport: { eyeptlocx = (length * 0.07) * 12.0;
                          eyeptlocy = -30.0;
                          eyeptlocz = 70.0; break; }
    case at3ETransport: { eyeptlocx = (length * 0.07) * 12.0;
                          eyeptlocy = -30.0;
                          eyeptlocz = 75.0; break; }
    case at4ETransport: { eyeptlocx = (length * 0.07) * 12.0;
                          eyeptlocy = -32.0;
                          eyeptlocz = 80.0; break; }
    case atMEProp:      { eyeptlocx = (length * 0.08) * 12.0;
                          eyeptlocy = -24.0;
                          eyeptlocz = 65.0; break; }
  }

  //***** LANDING GEAR *********************************

  // set main gear longitudinal location relative to CG
  double gearlocx_main;
  if (tricycle) gearlocx_main = cglocx * 1.04;
  else  gearlocx_main = cglocx * 0.91;
  

  // set main gear lateral location
  double gearlocy_main;
  switch(aType) {
    case atGlider:       gearlocy_main = wingspan * 0.005* 12; break;
    case atLtSingle:     gearlocy_main = wingspan * 0.09 * 12; break;
    case atLtTwin:       gearlocy_main = wingspan * 0.09 * 12; break;
    case atRacer:        gearlocy_main = wingspan * 0.15 * 12; break;
    case atSEFighter:    gearlocy_main = wingspan * 0.09 * 12; break;
    case at2EFighter:    gearlocy_main = wingspan * 0.09 * 12; break;
    case at2ETransport:  gearlocy_main = wingspan * 0.09 * 12; break;
    case at3ETransport:  gearlocy_main = wingspan * 0.09 * 12; break;
    case at4ETransport:  gearlocy_main = wingspan * 0.09 * 12; break;
    case atMEProp:       gearlocy_main = wingspan * 0.11 * 12; break;
   }

  // set main gear length (from aircraft centerline, extended)
  double gearlocz_main;
  if (tricycle) gearlocz_main = -(length * 0.12 * 12);
  else gearlocz_main = -(length * 0.20 * 12); 

  if (aType == atGlider) gearlocz_main = -(length / 10.0 * 12); 

  double gearlocx_nose = length * 0.13 * 12;
  double gearlocy_nose = 0.0;
  double gearlocz_nose = gearlocz_main;
  if (aType == atGlider) gearlocz_nose = gearlocz_main * 0.6; 

  double gearlocx_tail = length * 0.91 * 12;
  double gearlocy_tail = 0.0;
  double gearlocz_tail = gearlocz_main * 0.30;

  double gearspring_main = MTOW * 1.0;
  double gearspring_nose = MTOW * 0.3;
  double gearspring_tail = MTOW * 1.0;

  double geardamp_main = MTOW * 0.2;
  double geardamp_nose = MTOW * 0.1;
  double geardamp_tail = MTOW * 0.8;

  double geardynamic = 0.5;
  double gearstatic  = 0.8;
  double gearrolling = 0.02;
  if (aType == atGlider) gearrolling = 0.5; 

  string gearsteerable_nose = "STEERABLE";
  string gearsteerable_main = "FIXED";
  string gearsteerable_tail = "CASTERED";
  double gearmaxsteer = 5.0;
  string retract;
  if (retractable) retract = "RETRACT";
  else retract = "FIXED";

  //***** PROPULSION ************************************

  // spread engines out in reasonable locations

  double englocx[16], englocy[16], englocz[16], leftmost;
  double engfeed[16], thrusterlocx[16], thrusterlocy[16], thrusterlocz[16];

  if (elType == elFwd_Fuselage) {
    leftmost = ((double)engines * -20.0) + 20.0;
    for(i=0; i<engines; i++) {   
      englocx[i] = 36.0;
      englocy[i] = leftmost + (i * 40.0);
      englocz[i] = 0.0; 
     }
  }

  if (elType == elMid_Fuselage) {
    leftmost = ((double)engines * -20.0) + 20.0;
    for(i=0; i<engines; i++) {   
      englocx[i] = cglocx;
      englocy[i] = leftmost + (i * 40.0);
      englocz[i] = -12.0; 
     }
  } 

  if (elType == elAft_Fuselage) {
    leftmost = (engines * -20.0) + 20.0;
    for(i=0; i<engines; i++) {   
      englocx[i] = (length * 12.0) - 60.0;
      englocy[i] = leftmost + (i * 40.0);
      englocz[i] = 0.0; 
     }
  } 

  // wing engines (odd one goes in middle)
  if (elType == elWings) {
    int halfcount = (int)(engines / 2);
    for(i=0; i<halfcount; i++) {                 //left wing
       englocx[i] = cglocx;
       englocy[i] = wingspan * -2.0;             // span/-2/3*12
       englocz[i] = -40.0; 
     }    
    englocx[halfcount] = cglocx;                 // center
    englocy[halfcount] = 0.0;
    englocz[halfcount] = -20; 
    for(k=halfcount+1; k<engines; k++) {         //right wing
       englocx[k] = cglocx;
       englocy[k] = wingspan * 2.0;              // span/2/3*12
       englocz[k] = -40.0; 
     }    
  }

  // wing and tail engines (odd one goes in tail)
  if (elType == elWings_Tail) {
   int halfcount = (int)(engines / 2);
   for(i=0; i<halfcount; i++) {                  //left wing
      englocx[i] = cglocx;
      englocy[i] = wingspan * -2.0;              // span/-2/3*12
      englocz[i] = -40.0; 
     }    
   englocx[halfcount] = length - 60;             // center
   englocy[halfcount] = 0.0;
   englocz[halfcount] = 60.0; 
   for(k=halfcount+1; k<engines; k++) {          //right wing
      englocx[k] = cglocx;
      englocy[k] = wingspan * 2.0;               // span/2/3*12 
      englocz[k] = -40.0; 
     }    
  }

  // wing and nose engines (odd one goes in nose)     
  if (elType == elWings_Nose) {
   int halfcount = (int)(engines / 2);
   for(i=0; i<halfcount; i++) {                  //left wing
      englocx[i] = cglocx;
      englocy[i] = wingspan * -2.0;              //span/-2/3*12
      englocz[i] = -40.0; 
     }    
   englocx[halfcount] = 36.0;                    //center 
   englocy[halfcount] = 0.0;
   englocz[halfcount] = 0.0; 
   for(k=halfcount+1; k<engines; k++) {          //right wing
      englocx[k] = cglocx;
      englocy[k] = wingspan * 2.0;               //span/2/3*12
      englocz[k] = -40.0; 
     }    
  }

  // thruster goes where engine is
  double engpitch = 0.0;
  double engyaw = 0.0;
  double thrusterpitch = 0.0;
  double thrusteryaw = 0.0;
  for(i=0; i<engines; i++) {
    engfeed[i] = i;
    thrusterlocx[i] = englocx[i];
    thrusterlocy[i] = englocy[i];
    thrusterlocz[i] = englocz[i];
  }

  // thruster type (note: only piston engine gets a propeller)
  string thrustertype;
  switch(eType) {
    case etPiston:     thrustertype = "prop"; break;
    case etTurbine:    thrustertype = "direct"; break;
    case etTurboprop:  thrustertype = "direct"; break;
    case etRocket:     thrustertype = "direct"; break;
  }

  //***** FUEL TANKS **********************************

  // an N-engined airplane will have N+1 fuel tanks
  // all tanks located at CG and are half full
  double tanklocx = cglocx;
  double tanklocy = cglocy;
  double tanklocz = cglocz;
  double tankcapacity, tankcontents;
  double tankradius = 0.0;
  switch(aType) {
    case atGlider:      tankcapacity =   0.0; break;
    case atLtSingle:    tankcapacity =  20.0; break;
    case atLtTwin:      tankcapacity =  50.0; break;
    case atRacer:       tankcapacity = 200.0; break;
    case atSEFighter:   tankcapacity = 500.0; break;
    case at2EFighter:   tankcapacity = 700.0; break;
    case at2ETransport: tankcapacity = MTOW/23.0/(engines + 1); break;
    case at3ETransport: tankcapacity = MTOW/16.0/(engines + 1); break;
    case at4ETransport: tankcapacity = MTOW/16.0/(engines + 1); break;
    case atMEProp:      tankcapacity = MTOW/18.0/(engines + 1); break;
  }
  tankcontents = tankcapacity/2;

  //***** LIFT ****************************************

  // estimate slope of lift curve based on airplane type
  // units: per radian
  double CLalpha;
  switch(aType) {
    case atGlider:      CLalpha = 5.5; break;
    case atLtSingle:    CLalpha = 5.0; break;
    case atLtTwin:      CLalpha = 4.8; break;
    case atRacer:       CLalpha = 4.5; break;
    case atSEFighter:   CLalpha = 3.5; break;
    case at2EFighter:   CLalpha = 3.6; break;
    case at2ETransport: CLalpha = 4.4; break;
    case at3ETransport: CLalpha = 4.4; break;
    case at4ETransport: CLalpha = 4.4; break;
    case atMEProp:      CLalpha = 4.9; break;
  }

  // estimate CL at zero alpha
  double CL0;
  switch(aType) {
    case atGlider:      CL0 = 0.25; break;
    case atLtSingle:    CL0 = 0.25; break;
    case atLtTwin:      CL0 = 0.24; break;
    case atRacer:       CL0 = 0.17; break;
    case atSEFighter:   CL0 = 0.08; break;
    case at2EFighter:   CL0 = 0.08; break;
    case at2ETransport: CL0 = 0.20; break;
    case at3ETransport: CL0 = 0.20; break;
    case at4ETransport: CL0 = 0.20; break;
    case atMEProp:      CL0 = 0.24; break;
  }

  // estimate stall CL (no flaps), based on airplane type
  double CLmax;
  switch(aType) {
    case atGlider:      CLmax = 1.40; break;
    case atLtSingle:    CLmax = 1.40; break;
    case atLtTwin:      CLmax = 1.30; break;
    case atRacer:       CLmax = 1.20; break;
    case atSEFighter:   CLmax = 1.00; break;
    case at2EFighter:   CLmax = 1.00; break;
    case at2ETransport: CLmax = 1.20; break;
    case at3ETransport: CLmax = 1.20; break;
    case at4ETransport: CLmax = 1.20; break;
    case atMEProp:      CLmax = 1.40; break;
  }

  // estimate delta-CL-flaps
  double dCLflaps;
  switch(aType) {
    case atGlider:      dCLflaps = 0.20; break;
    case atLtSingle:    dCLflaps = 0.40; break;
    case atLtTwin:      dCLflaps = 0.40; break;
    case atRacer:       dCLflaps = 0.30; break;
    case atSEFighter:   dCLflaps = 0.35; break;
    case at2EFighter:   dCLflaps = 0.35; break;
    case at2ETransport: dCLflaps = 1.50; break;
    case at3ETransport: dCLflaps = 1.50; break;
    case at4ETransport: dCLflaps = 1.50; break;
    case atMEProp:      dCLflaps = 0.60; break;
  }

  // some types have speedbrakes in wings, affecting lift
  double dCLspeedbrake;
  switch(aType) {
    case atGlider:      dCLspeedbrake = -0.05; break;
    case atLtSingle:    dCLspeedbrake =  0.00; break;
    case atLtTwin:      dCLspeedbrake =  0.00; break;
    case atRacer:       dCLspeedbrake =  0.00; break;
    case atSEFighter:   dCLspeedbrake =  0.00; break;
    case at2EFighter:   dCLspeedbrake =  0.00; break;
    case at2ETransport: dCLspeedbrake = -0.10; break;
    case at3ETransport: dCLspeedbrake = -0.09; break;
    case at4ETransport: dCLspeedbrake = -0.08; break;
    case atMEProp:      dCLspeedbrake =  0.00; break;
  }

  // estimate lift due to elevator deflection
  double CLde = 0.2;

  //***** DRAG *****************************************

  // estimate drag at zero lift, based on airplane type
  // NOT including landing gear
  double CD0;
  switch(aType) {
    case atGlider:      CD0 = 0.010; break;
    case atLtSingle:    CD0 = 0.024; break;
    case atLtTwin:      CD0 = 0.025; break;
    case atRacer:       CD0 = 0.020; break;
    case atSEFighter:   CD0 = 0.021; break;
    case at2EFighter:   CD0 = 0.024; break;
    case at2ETransport: CD0 = 0.020; break;
    case at3ETransport: CD0 = 0.019; break;
    case at4ETransport: CD0 = 0.017; break;
    case atMEProp:      CD0 = 0.025; break;
  }

  // add gear drag if fixed gear
  if(!retractable) {
   switch(aType) {
    case atGlider:      CD0 += 0.002; break;
    case atLtSingle:    CD0 += 0.004; break;
    case atLtTwin:      CD0 += 0.004; break;
    case atRacer:       CD0 += 0.004; break;
    case atSEFighter:   CD0 += 0.005; break;
    case at2EFighter:   CD0 += 0.005; break;
    case at2ETransport: CD0 += 0.002; break;
    case at3ETransport: CD0 += 0.002; break;
    case at4ETransport: CD0 += 0.002; break;
    case atMEProp:      CD0 += 0.003; break;
   }
  }

  // estimate induced drag coefficient K
  double K;
  switch(aType) {
    case atGlider:      K = 0.023; break;
    case atLtSingle:    K = 0.040; break;
    case atLtTwin:      K = 0.041; break;
    case atRacer:       K = 0.045; break;
    case atSEFighter:   K = 0.090; break;
    case at2EFighter:   K = 0.090; break;
    case at2ETransport: K = 0.043; break;
    case at3ETransport: K = 0.042; break;
    case at4ETransport: K = 0.042; break;
    case atMEProp:      K = 0.039; break;
  }

  // CD flaps
  double CDflaps;
  switch(aType) {
    case atGlider:      CDflaps = 0.024; break;
    case atLtSingle:    CDflaps = 0.030; break;
    case atLtTwin:      CDflaps = 0.039; break;
    case atRacer:       CDflaps = 0.040; break;
    case atSEFighter:   CDflaps = 0.080; break;
    case at2EFighter:   CDflaps = 0.075; break;
    case at2ETransport: CDflaps = 0.059; break;
    case at3ETransport: CDflaps = 0.057; break;
    case at4ETransport: CDflaps = 0.055; break;
    case atMEProp:      CDflaps = 0.035; break;
  }

  // estimate drag from landing gear down
  double CDgear;
  switch(aType) {
    case atGlider:      CDgear = 0.012; break;
    case atLtSingle:    CDgear = 0.030; break;
    case atLtTwin:      CDgear = 0.030; break;
    case atRacer:       CDgear = 0.030; break;
    case atSEFighter:   CDgear = 0.020; break;
    case at2EFighter:   CDgear = 0.020; break;
    case at2ETransport: CDgear = 0.015; break;
    case at3ETransport: CDgear = 0.013; break;
    case at4ETransport: CDgear = 0.011; break;
    case atMEProp:      CDgear = 0.023; break;
  }

  double CDde = 0.04;             // elevator deflection
  double CDbeta = 0.2;            // sideslip
  double CDspeedbrake = CD0;      // speedbrake

  // estimate critical mach
  double Mcrit;
  switch(aType) {
    case atGlider:      Mcrit = 0.70; break;
    case atLtSingle:    Mcrit = 0.70; break;
    case atLtTwin:      Mcrit = 0.72; break;
    case atRacer:       Mcrit = 0.73; break;
    case atSEFighter:   Mcrit = 0.81; break;
    case at2EFighter:   Mcrit = 0.81; break;
    case at2ETransport: Mcrit = 0.79; break;
    case at3ETransport: Mcrit = 0.79; break;
    case at4ETransport: Mcrit = 0.79; break;
    case atMEProp:      Mcrit = 0.70; break;
  }

  //***** SIDE *************************************

  // estimate side force due to sideslip (beta)
  double CYbeta = -1.0;

  //***** ROLL *************************************

  // estimate roll coefficients
  double Clbeta = -0.1;     // sideslip
  double Clp = -0.4;        // roll rate
  double Clr = 0.15;        // yaw rate

  // estimate aileron power
  double Clda;
  switch(aType) {
    case atGlider:      Clda = 0.06; break;
    case atLtSingle:    Clda = 0.17; break;
    case atLtTwin:      Clda = 0.17; break;
    case atRacer:       Clda = 0.18; break;
    case atSEFighter:   Clda = 0.11; break;
    case at2EFighter:   Clda = 0.12; break;
    case at2ETransport: Clda = 0.10; break;
    case at3ETransport: Clda = 0.10; break;
    case at4ETransport: Clda = 0.10; break;
    case atMEProp:      Clda = 0.15; break;
  }

  double Cldr = 0.01;       // rudder roll

  //***** PITCH ************************************

  double Cmalpha, Cmde, Cmq, Cmadot;

  // estimate pitch coefficients
  switch(aType) {
  case  atGlider:
    Cmalpha = -0.5;    // per radian alpha
    Cmde = -0.8;       // elevator deflection
    Cmq = -9.0;        // pitch rate
    Cmadot = -12.0;    // alpha-dot
    break;
  case  atLtSingle:
    Cmalpha = -0.5;    // per radian alpha
    Cmde = -1.1;       // elevator deflection
    Cmq = -12.0;       // pitch rate
    Cmadot = -7.0;     // alpha-dot
    break;
  case  atLtTwin:
    Cmalpha = -0.4;    // per radian alpha
    Cmde = -1.0;       // elevator deflection
    Cmq = -22.0;       // pitch rate
    Cmadot = -8.0;     // alpha-dot
    break;
  case  atRacer: 
    Cmalpha = -0.5;    // per radian alpha
    Cmde = -1.0;       // elevator deflection
    Cmq = -15.0;       // pitch rate
    Cmadot = -7.0;     // alpha-dot
    break;
  case  atSEFighter:
    Cmalpha = -0.3;    // per radian alpha
    Cmde = -0.8;       // elevator deflection
    Cmq = -18.0;       // pitch rate
    Cmadot = -9.0;     // alpha-dot
    break;
  case  at2EFighter: 
    Cmalpha = -0.3;    // per radian alpha
    Cmde = -0.8;       // elevator deflection
    Cmq = -18.0;       // pitch rate
    Cmadot = -9.0;     // alpha-dot
    break;
  case  at2ETransport: 
    Cmalpha = -0.6;    // per radian alpha
    Cmde = -1.2;       // elevator deflection
    Cmq = -17.0;       // pitch rate
    Cmadot = -6.0;     // alpha-dot
    break;
  case  at3ETransport:
    Cmalpha = -0.6;    // per radian alpha
    Cmde = -1.2;       // elevator deflection
    Cmq = -17.0;       // pitch rate
    Cmadot = -6.0;     // alpha-dot
    break;
  case  at4ETransport: 
    Cmalpha = -0.7;    // per radian alpha
    Cmde = -1.3;       // elevator deflection
    Cmq = -21.0;       // pitch rate
    Cmadot = -4.0;     // alpha-dot
    break;
  case  atMEProp: 
    Cmalpha = -0.4;    // per radian alpha
    Cmde = -1.0;       // elevator deflection
    Cmq = -22.0;       // pitch rate
    Cmadot = -8.0;     // alpha-dot
    break;
  }

  //***** YAW **************************************

  // estimate yaw coefficients
  double Cnbeta = 0.12;     // sideslip
  double Cnr = -0.15;       // yaw rate
  double Cndr = -0.10;      // rudder deflection
  if(aType == atGlider) Cndr = -0.03; 

  // estimate adverse yaw
  double Cnda;
  switch(aType) {
    case atGlider:      Cnda = -0.02; break;
    case atLtSingle:    Cnda = -0.01; break;
    case atLtTwin:      Cnda = -0.01; break;
    case atRacer:       Cnda = -0.003; break;
    case atSEFighter:   Cnda =  0.0; break;
    case at2EFighter:   Cnda =  0.0; break;
    case at2ETransport: Cnda =  0.0; break;
    case at3ETransport: Cnda =  0.0; break;
    case at4ETransport: Cnda =  0.0; break;
    case atMEProp:      Cnda = -0.008; break;
  }


//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

  string filename = AircraftName + ".xml";
  ofstream f;
  f.open(filename.c_str(), ios::out);
  f << fixed << setw(7) << setprecision(2);

  f << "<?xml version=\"1.0\"?>\n";
  f << "<?xml-stylesheet href=\"JSBSim.xsl\" type=\"application/xml\"?>\n";
  f << "<fdm_config name=\"" << AircraftName << "\" version=\"2.0\" release=\"ALPHA\">\n";

  f << " <fileheader>\n";
  f << "  <author>Aeromatic v " << AEROMATIC_VERSION << "</author>\n";
  f << "  <filecreationdate>now</filecreationdate>\n";
  f << "  <description>Models a " << AircraftName << "</description>\n";
  f << "  <reference refID=\"None\" author=\"n/a\" title=\"n/a\" date=\"n/a\" />\n";
  f << " </fileheader>\n\n";
 
  f << "<!--\n  File:     " << filename << endl;
  f << "  Inputs:\n";
  f << "    name:          " << AircraftName << endl;
  switch(aType) {
    case atGlider:      f << "    type:          glider\n"; break;
    case atLtSingle:    f << "    type:          light single\n"; break;
    case atLtTwin:      f << "    type:          light twin\n"; break;
    case atRacer:       f << "    type:          WWII fighter, subsonic sport, aerobatic\n"; break;
    case atSEFighter:   f << "    type:          single-engine transonic/supersonic fighter\n"; break;
    case at2EFighter:   f << "    type:          two-engine transonic/supersonic fighter\n"; break;
    case at2ETransport: f << "    type:          two-engine transonic transport\n"; break;
    case at3ETransport: f << "    type:          three-engine transonic transport\n"; break;
    case at4ETransport: f << "    type:          four-engine transonic transport\n"; break;
    case atMEProp:      f << "    type:          multi-engine prop transport\n"; break;
  }
  f << "    max weight:    " << MTOW << " lb\n";
  f << "    wing span:     " << wingspan << " ft\n";
  f << "    length:        " << length << " ft\n";
  
  if(!wingarea_input)
    f << "    wing area:     unspecified\n";
  else
    f << "    wing area:     " << wingarea << " sq-ft\n";
  
  if (tricycle) f << "    gear type:     tricycle\n";
  else  f << "    gear type:     taildragger\n"; 
  
  if (retractable)  f << "    retractable?:  yes\n";
  else  f << "    retractable?:  no\n"; 

  f << "    # engines:     " << engines << endl;
  switch(eType) {
    case etPiston:     f << "    engine type:   piston\n"; break; 
    case etTurbine:    f << "    engine type:   turbine\n"; break; 
    case etTurboprop:  f << "    engine type:   turboprop\n"; break; 
    case etRocket:     f << "    engine type:   rocket\n"; break; 
  }

  switch(elType) {
    case elFwd_Fuselage: f << "    engine layout: forward fuselage\n"; break; 
    case elMid_Fuselage: f << "    engine layout: middle fuselage\n"; break; 
    case elAft_Fuselage: f << "    engine layout: aft fuselage\n"; break; 
    case elWings:        f << "    engine layout: wings\n"; break; 
    case elWings_Tail:   f << "    engine layout: wings and tail\n"; break; 
    case elWings_Nose:   f << "    engine layout: wings and nose\n"; break; 
  }

  if(yawdamper)  f << "    yaw damper?    yes\n";
  else           f << "    yaw damper?    no\n\n";
  
  f << fixed << setw(7) << setprecision(4);
  f << "  Outputs:\n";
  f << "    wing loading:  " << wingloading << " lb/sq-ft\n";
  f << "    CL-alpha:      " << CLalpha << " per radian\n";
  f << "    CL-0:          " << CL0 << endl;
  f << "    CL-max:        " << CLmax << endl;
  f << "    CD-0:          " << CD0 << endl;
  f << "    K:             " << K << endl; 
  f << "\n-->\n\n"; 
  f << fixed << setw(7) << setprecision(2);
  
//***** METRICS **********************************

  f << " <metrics>\n";
  f << "   <wingarea  unit=\"FT2\"> " << wingarea << " </wingarea>\n";
  f << "   <wingspan  unit=\"FT\" > " << wingspan << " </wingspan>\n";
  f << "   <chord     unit=\"FT\" > " << wingchord << " </chord>\n";
  f << "   <htailarea unit=\"FT2\"> " << htailarea << " </htailarea>\n";
  f << "   <htailarm  unit=\"FT\" > " << htailarm  << " </htailarm>\n";
  f << "   <vtailarea unit=\"FT2\"> " << vtailarea << " </vtailarea>\n";
  f << "   <vtailarm  unit=\"FT\" > " << vtailarm << " </vtailarm>\n";
  f << "   <location name=\"AERORP\" unit=\"IN\">\n";
  f << "     <x> " << aerorpx << " </x>\n";
  f << "     <y> " << aerorpy << " </y>\n";
  f << "     <z> " << aerorpz << " </z>\n";
  f << "   </location>\n";
  f << "   <location name=\"EYEPOINT\" unit=\"IN\">\n";
  f << "     <x> " << eyeptlocx << " </x>\n";
  f << "     <y> " << eyeptlocy << " </y>\n";
  f << "     <z> " << eyeptlocz << " </z>\n";
  f << "   </location>\n";
  f << "   <location name=\"VRP\" unit=\"IN\">\n";
  f << "     <x>0</x>\n";
  f << "     <y>0</y>\n";
  f << "     <z>0</z>\n";
  f << "   </location>\n";
  f << " </metrics>\n\n";

  f << " <mass_balance>\n";
  f << "   <ixx unit=\"SLUG*FT2\">  " << ixx << " </ixx>\n";
  f << "   <iyy unit=\"SLUG*FT2\">  " << iyy << " </iyy>\n";
  f << "   <izz unit=\"SLUG*FT2\">  " << izz << " </izz>\n";
  f << "   <ixz unit=\"SLUG*FT2\">  " << ixz << " </ixz>\n";
  f << "   <emptywt unit=\"LBS\" >  " << emptyweight << " </emptywt>\n";
  f << "   <location name=\"CG\" unit=\"IN\">\n";
  f << "     <x> " << cglocx << " </x>\n";
  f << "     <y> " << cglocy << " </y>\n";
  f << "     <z> " << cglocz << " </z>\n";
  f << "   </location>\n";
  f << " </mass_balance>\n\n";


  //***** LANDING GEAR ******************************

  f << " <ground_reactions>\n\n";

  if(aType == atGlider) { 

    f << "  <contact type=\"BOGEY\" name=\"LEFT_MAIN\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "      <x> " <<  gearlocx_main << " </x>\n";
    f << "      <y> " << -gearlocy_main << " </y>\n";
    f << "      <z> " <<  gearlocz_main << " </z>\n";
    f << "    </location>\n";
    f << "    <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "    <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "    <rolling_friction> " << gearrolling << " </rolling_friction>\n";
    f << "    <spring_coeff  unit=\"LBS/FT\">     " << gearspring_main << " </spring_coeff>\n";
    f << "    <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << "    <max_steer unit=\"DEG\">0</max_steer>\n";
    f << "    <brake_group>NONE</brake_group>\n";
    f << "    <retractable>" << retract << "</retractable>\n";
    f << "  </contact>\n\n";
  
    f << "  <contact type=\"BOGEY\" name=\"RIGHT_MAIN\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << gearlocx_main << " </x>\n";
    f << "     <y> " << gearlocy_main << " </y>\n";
    f << "     <z> " << gearlocz_main << " </z>\n";
    f << "   </location>\n";
    f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "   <rolling_friction> " << gearrolling << " </rolling_friction>\n";
    f << "   <spring_coeff unit=\"LBS/FT\">     " << gearspring_main << " </spring_coeff>\n";
    f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << "   <max_steer unit=\"DEG\">0</max_steer>\n";
    f << "   <brake_group>NONE</brake_group>\n";
    f << "   <retractable> " << retract << "</retractable>\n";
    f << " </contact>\n\n";

    f << "  <contact type=\"BOGEY\" name=\"NOSE\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << gearlocx_nose << " </x>\n";
    f << "     <y> " << gearlocy_nose << " </y>\n";
    f << "     <z> " << gearlocz_nose << " </z>\n";
    f << "   </location>\n";
    f << "  <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "  <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "  <rolling_friction> " << gearrolling << " </rolling_friction>\n";
    f << "  <spring_coeff unit=\"LBS/FT\">      " << gearspring_nose << " </spring_coeff>\n";
    f << "  <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_nose << " </damping_coeff>\n";
    f << "  <max_steer unit=\"DEG\">0</max_steer>\n";
    f << "  <brake_group>NONE</brake_group>\n";
    f << "  <retractable>" << retract << "</retractable>\n";
    f << " </contact>\n\n";

    f << "  <contact type=\"STRUCTURE\" name=\"LEFT_WING\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << cglocx << " </x>\n";
    f << "     <y> " << -halfspan << " </y>\n";
    f << "     <z> " << cglocz << " </z>\n";
    f << "   </location>\n";
    f << "  <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "  <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "  <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
    f << "  <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << " </contact>\n\n";

    f << "  <contact type=\"STRUCTURE\" name=\"RIGHT_WING\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << cglocx << " </x>\n";
    f << "     <y> " << halfspan << " </y>\n";
    f << "     <z> " << cglocz << " </z>\n";
    f << "   </location>\n";
    f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
    f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << " </contact>\n\n";
 }
 else {                // not a glider
   if (tricycle) {  

      f << "  <contact type=\"BOGEY\" name=\"NOSE\">\n";
      f << "   <location unit=\"IN\">\n";
      f << "     <x> " << gearlocx_nose << " </x>\n";
      f << "     <y> " << gearlocy_nose << " </y>\n";
      f << "     <z> " << gearlocz_nose << " </z>\n";
      f << "   </location>\n";
      f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
      f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
      f << "   <rolling_friction> " << gearrolling << " </rolling_friction>\n";
      f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_nose << " </spring_coeff>\n";
      f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_nose << " </damping_coeff>\n";
      f << "   <max_steer unit=\"DEG\"> " << gearmaxsteer << " </max_steer>\n";
      f << "   <brake_group>NONE</brake_group>\n";
      f << "   <retractable>" << retract << "</retractable>\n";
      f << " </contact>\n\n";
   }

      f << "  <contact type=\"BOGEY\" name=\"LEFT_MAIN\">\n";
      f << "   <location unit=\"IN\">\n";
      f << "     <x> " <<  gearlocx_main << " </x>\n";
      f << "     <y> " << -gearlocy_main << " </y>\n";
      f << "     <z> " <<  gearlocz_main << " </z>\n";
      f << "   </location>\n";
      f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
      f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
      f << "   <rolling_friction> " << gearrolling << " </rolling_friction>\n";
      f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
      f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
      f << "   <max_steer unit=\"DEG\">0</max_steer>\n";
      f << "   <brake_group>LEFT</brake_group>\n";
      f << "   <retractable>" << retract << "</retractable>\n";
      f << " </contact>\n\n";
  
      f << "  <contact type=\"BOGEY\" name=\"RIGHT_MAIN\">\n";
      f << "   <location unit=\"IN\">\n";
      f << "     <x> " << gearlocx_main << " </x>\n";
      f << "     <y> " << gearlocy_main << " </y>\n";
      f << "     <z> " << gearlocz_main << " </z>\n";
      f << "   </location>\n";
      f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
      f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
      f << "   <rolling_friction> " << gearrolling << " </rolling_friction>\n";
      f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
      f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
      f << "   <max_steer unit=\"DEG\">0</max_steer>\n";
      f << "   <brake_group>RIGHT</brake_group>\n";
      f << "   <retractable>" << retract << "</retractable>\n";
      f << " </contact>\n\n";

   if (!tricycle) { 

      f << "  <contact type=\"BOGEY\" name=\"TAIL\">\n";
      f << "   <location unit=\"IN\">\n";
      f << "     <x> " << gearlocx_tail << " </x>\n";
      f << "     <y> " << gearlocy_tail << " </y>\n";
      f << "     <z> " << gearlocz_tail << " </z>\n";
      f << "   </location>\n";
      f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
      f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
      f << "   <rolling_friction> " << gearrolling << " </rolling_friction>\n";
      f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_tail << " </spring_coeff>\n";
      f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_tail << " </damping_coeff>\n";
      f << "   <max_steer unit=\"DEG\"> " << gearmaxsteer << " </max_steer>\n";
      f << "   <brake_group>NONE</brake_group>\n";
      f << "   <retractable>" << retract << "</retractable>\n";
      f << " </contact>\n\n";
   }

    f << "  <contact type=\"STRUCTURE\" name=\"LEFT_WING\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << cglocx << " </x>\n";
    f << "     <y> " << -halfspan << " </y>\n";
    f << "     <z> " << cglocz << " </z>\n";
    f << "   </location>\n";
    f << "  <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "  <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "  <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
    f << "  <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << " </contact>\n\n";

    f << "  <contact type=\"STRUCTURE\" name=\"RIGHT_WING\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "     <x> " << cglocx << " </x>\n";
    f << "     <y> " << halfspan << " </y>\n";
    f << "     <z> " << cglocz << " </z>\n";
    f << "   </location>\n";
    f << "   <static_friction>  " << gearstatic << " </static_friction>\n";
    f << "   <dynamic_friction> " << geardynamic << " </dynamic_friction>\n";
    f << "   <spring_coeff unit=\"LBS/FT\">      " << gearspring_main << " </spring_coeff>\n";
    f << "   <damping_coeff unit=\"LBS/FT/SEC\"> " << geardamp_main << " </damping_coeff>\n";
    f << " </contact>\n\n";

 }
  f << " </ground_reactions>\n\n";


//***** PROPULSION ***************************************

  f << " <propulsion>\n\n";
  if(aType == atGlider) { // if glider, do nothing here
  }
  else {
  for(i=0; i<engines; i++) {

    f << "   <engine file=\"" << EngineName << "\">\n";
    f << "    <location unit=\"IN\">\n";
    f << "      <x> " << englocx[i] << " </x>\n";
    f << "      <y> " << englocy[i] << " </y>\n";
    f << "      <z> " << englocz[i] << " </z>\n";
    f << "    </location>\n";
    f << "    <orient unit=\"DEG\">\n";
    f << "      <pitch> " << engpitch << " </pitch>\n";
    f << "      <roll>   0.00 </roll>\n";
    f << "      <yaw>   " << engyaw << " </yaw>\n";
    f << "    </orient>\n";
    f << "    <feed>" << engfeed[i] << "</feed>\n";

    if (eType == etPiston)  f << "    <thruster file=\"" << PropName << "\">\n";
    else                    f << "    <thruster file=\"direct\">\n";
    
    f << "     <location unit=\"IN\">\n";
    f << "       <x> " << thrusterlocx[i] << " </x>\n";
    f << "       <y> " << thrusterlocy[i] << " </y>\n";
    f << "       <z> " << thrusterlocz[i] << " </z>\n";
    f << "     </location>\n";
    f << "     <orient unit=\"DEG\">\n";
    f << "       <pitch> " << thrusterpitch << " </pitch>\n";
    f << "       <roll>   0.00 </roll>\n";
    f << "       <yaw>   " << thrusteryaw << " </yaw>\n";
    f << "     </orient>\n";

    f << "    </thruster>\n";
    f << "  </engine>\n\n";
  }

 //***** FUEL TANKS **************************************

 for(i=0; i<(engines + 1); i++) {
    f << "  <tank type=\"FUEL\" number=\"" << i << "\">\n";
    f << "     <location unit=\"IN\">\n";
    f << "       <x> " << tanklocx << " </x>\n";
    f << "       <y> " << tanklocy << " </y>\n";
    f << "       <z> " << tanklocz << " </z>\n";
    f << "     </location>\n";
    f << "     <capacity unit=\"LBS\"> " << tankcapacity << " </capacity>\n";
    f << "     <contents unit=\"LBS\"> " << tankcontents << " </contents>\n";
    f << "  </tank>\n\n";
 }
}
  f << " </propulsion>\n\n";


//***** FLIGHT CONTROL SYSTEM ***************************

  f << " <flight_control name=\"" << AircraftName << "\">\n\n";

  f << "   <component name=\"Pitch Trim Sum\" type=\"SUMMER\">\n";
  f << "      <input>fcs/elevator-cmd-norm</input>\n";
  f << "      <input>fcs/pitch-trim-cmd-norm</input>\n";
  f << "      <clipto>\n";
  f << "        <min> -1 </min>\n";
  f << "        <max>  1 </max>\n";
  f << "      </clipto>\n";
  f << "   </component>\n\n";

  f << "   <component name=\"Elevator Control\" type=\"AEROSURFACE_SCALE\">\n";
  f << "      <input>fcs/pitch-trim-sum</input>\n";
  f << "      <limit>\n";
  f << "        <min> -0.35 </min>\n";
  f << "        <max>  0.30 </max>\n";
  f << "      </limit>\n";
  f << "      <output>fcs/elevator-pos-rad</output>\n";
  f << "   </component>\n\n";

  f << "   <component name=\"Roll Trim Sum\" type=\"SUMMER\">\n";
  f << "      <input>fcs/aileron-cmd-norm</input>\n";
  f << "      <input>fcs/roll-trim-cmd-norm</input>\n";
  f << "      <clipto>\n";
  f << "        <min> -1 </min>\n";
  f << "        <max>  1 </max>\n";
  f << "      </clipto>\n";
  f << "   </component>\n\n";

  f << "   <component name=\"Left Aileron Control\" type=\"AEROSURFACE_SCALE\">\n";
  f << "      <input>fcs/roll-trim-sum</input>\n";
  f << "      <limit>\n";
  f << "        <min> -0.35 </min>\n";
  f << "        <max>  0.35 </max>\n";
  f << "      </limit>\n";
  f << "      <output>fcs/left-aileron-pos-rad</output>\n";
  f << "   </component>\n\n";

  f << "   <component name=\"Right Aileron Control\" type=\"AEROSURFACE_SCALE\">\n";
  f << "      <input>fcs/roll-trim-sum</input>\n";
  f << "      <limit>\n";
  f << "        <min> -0.35 </min>\n";
  f << "        <max>  0.35 </max>\n";
  f << "      </limit>\n";
  f << "      <output>fcs/right-aileron-pos-rad</output>\n";
  f << "   </component>\n\n";

  f << "   <component name=\"Rudder Command Sum\" type=\"SUMMER\">\n";
  f << "      <input>fcs/rudder-cmd-norm</input>\n";
  f << "      <input>fcs/yaw-trim-cmd-norm</input>\n";
  f << "      <limit>\n";
  f << "        <min> -0.35 </min>\n";
  f << "        <max>  0.35 </max>\n";
  f << "      </limit>\n";
  f << "   </component>\n\n";

  if (yawdamper) {
    f << "   <component name=\"Yaw Damper Rate\" type=\"SCHEDULED_GAIN\">\n";
    f << "      <input>velocities/r-aero-rad_sec</input>\n";
    f << "      <table>\n";
    f << "        <independentVar lookup=\"row\">velocities/ve-kts</independentVar>\n";
    f << "         <tableData>\n";
    f << "            30     0.00\n";
    f << "            60     2.00\n";
    f << "         </tableData>\n";
    f << "      </table>\n";
    f << "   </component>\n\n";

    f << "   <component name=\"Yaw Damper Beta\" type=\"SCHEDULED_GAIN\">\n";
    f << "      <input>aero/beta-rad</input>\n";
    f << "      <table>\n";
    f << "        <independentVar lookup=\"row\">velocities/ve-kts</independentVar>\n";
    f << "        <tableData>\n";
    f << "           30     0.00\n";
    f << "           60     0.00\n";
    f << "        </tableData>\n";
    f << "      </table>\n";
    f << "   </component>\n\n";

    f << "   <component name=\"Yaw Damper Sum\" type=\"SUMMER\">\n";
    f << "      <input>fcs/yaw-damper-beta</input>\n";
    f << "      <input>fcs/yaw-damper-rate</input>\n";
    f << "      <limit>\n";
    f << "        <min> -0.1 </min>\n";
    f << "        <max>  0.1 </max>\n";
    f << "      </limit>\n";
    f << "   </component>\n\n";

    f << "   <component name=\"Yaw Damper Final\" type=\"SCHEDULED_GAIN\">\n";
    f << "      <input>fcs/yaw-damper-sum</input>\n";
    f << "      <table>\n";
    f << "        <independentVar lookup=\"row\">velocities/ve-kts</independentVar>\n";
    f << "        <tableData>\n";
    f << "           30         0.0\n";
    f << "           31         1.0\n";
    f << "        </tableData>\n";
    f << "      </table>\n";
    f << "   </component>\n\n";

    f << "   <component name=\"Rudder Sum\" type=\"SUMMER\">\n";
    f << "      <input>fcs/rudder-command-sum</input>\n";
    f << "      <input>fcs/yaw-damper-final</input>\n";
    f << "      <limit>\n";
    f << "        <min> -1 </min>\n";
    f << "        <max>  1 </max>\n";
    f << "      </limit>\n";
    f << "   </component>\n\n";

    f << "   <component name=\"Rudder Control\" type=\"AEROSURFACE_SCALE\">\n";
    f << "      <input>fcs/rudder-sum</input>\n";
    f << "      <limit>\n";
    f << "        <min> -0.35 </min>\n";
    f << "        <max>  0.35 </max>\n";
    f << "      </limit>\n";
    f << "      <output>fcs/rudder-pos-rad</output>\n";
    f << "   </component>\n\n"; 
  }
  else {
     f << "   <component name=\"Rudder Control\" type=\"AEROSURFACE_SCALE\">\n";
     f << "      <input>fcs/rudder-command-sum</input>\n";
     f << "      <limit>\n";
     f << "        <min> -0.35 </min>\n";
     f << "        <max>  0.35 </max>\n";
     f << "      </limit>\n";
     f << "      <output>fcs/rudder-pos-rad</output>\n";
     f << "   </component>\n\n"; 
  }

  f << "   <component name=\"Flaps Control\" type=\"KINEMAT\">\n";
  f << "     <input>fcs/flap-cmd-norm</input>\n";
  f << "     <traverse>\n";
  f << "       <setting>\n";
  f << "          <position>  0 </position>\n";
  f << "          <time>      0 </time>\n";
  f << "       </setting>\n";
  f << "       <setting>\n";
  f << "          <position> 15 </position>\n";
  f << "          <time>      4 </time>\n";
  f << "       </setting>\n";
  f << "       <setting>\n";
  f << "          <position> 30 </position>\n";
  f << "          <time>      3 </time>\n";
  f << "       </setting>\n";
  f << "     </traverse>\n";
  f << "     <output>fcs/flap-pos-deg</output>\n";
  f << "   </component>\n\n";

  if (retractable) {
    f << "   <component name=\"Gear Control\" type=\"KINEMAT\">\n";
    f << "     <input>gear/gear-cmd-norm</input>\n";
    f << "     <traverse>\n";
    f << "       <setting>\n";
    f << "          <position> 0 </position>\n";
    f << "          <time>     0 </time>\n";
    f << "       </setting>\n";
    f << "       <setting>\n";
    f << "          <position> 1 </position>\n";
    f << "          <time>     5 </time>\n";
    f << "       </setting>\n";
    f << "     </traverse>\n";
    f << "     <output>gear/gear-pos-norm</output>\n";
    f << "   </component>\n\n";
  }

  f << "   <component name=\"Speedbrake Control\" type=\"KINEMAT\">\n";
  f << "     <input>fcs/speedbrake-cmd-norm</input>\n";
  f << "     <traverse>\n";
  f << "       <setting>\n";
  f << "          <position> 0 </position>\n";
  f << "          <time>     0 </time>\n";
  f << "       </setting>\n";
  f << "       <setting>\n";
  f << "          <position> 1 </position>\n";
  f << "          <time>     1 </time>\n";
  f << "       </setting>\n";
  f << "     </traverse>\n";
  f << "     <output>fcs/speedbrake-pos-norm</output>\n";
  f << "   </component>\n\n";

  f << " </flight_control>\n\n";

  //***** AERODYNAMICS ******************************************


  f << " <aerodynamics>\n\n";
  f << "  <axis name=\"LIFT\">\n\n";

  // build a simple lift curve with four points
  double point;
  f << "    <coefficient name=\"CLalpha\">\n";
  f << "      <description>Lift_due_to_alpha</description>\n";
  f << "      <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>\n";
  f << "            <tableData>\n";
  point = -(CLalpha * 0.2) + CL0;
  f << "              -0.20 " << point << endl;
  f << "               0.00 " << CL0 << endl;
  double alpha = (CLmax - CL0) / CLalpha;
  f << "             " << alpha << "    " << CLmax << endl;
  point = CLmax - (0.6 * alpha * CLalpha);
  f << "               0.60 " << point << endl;
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "      </function>\n";
  f << "    </coefficient>\n\n";

  double dCLflap_per_deg = dCLflaps / 30.0;
  f << "    <coefficient name=\"dCLflap\">\n"; 
  f << "       <description>Delta_Lift_due_to_flaps</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/flap-pos-deg</property>\n";
  f << "           <value> " << dCLflap_per_deg << " </value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"dCLsb\">\n"; 
  f << "       <description>Delta_Lift_due_to_speedbrake</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/speedbrake-pos-norm</property>\n";
  f << "           <value>" << dCLspeedbrake << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"CLde\">\n"; 
  f << "       <description>Lift_due_to_Elevator_Deflection</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/elevator-pos-rad</property>\n";
  f << "           <value>" << CLde << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";

  //***** DRAG ******************************************************

  f << "  <axis name=\"DRAG\">\n\n";

  f << "    <coefficient name=\"CD0\">\n"; 
  f << "       <description>Drag_at_zero_lift</description>\n";
  f << "       <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>\n";
  f << "            <tableData>\n";
  f <<  "             -1.57       1.500\n";
  double CD02 = CD0 * 1.3;
  f << "             -0.26    " << CD02 << endl;   
  f << "              0.00    " << CD0  << endl;
  f << "              0.26    " << CD02 << endl;   
  f <<  "              1.57       1.500\n";
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"CDi\">\n"; 
  f << "       <description>Induced_drag</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>aero/cl-squared-norm</property>\n";
  f << "           <value>" << K << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"CDmach\">\n"; 
  f << "       <description>Drag_due_to_mach</description>\n";
  f << "       <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";
  f << "            <tableData>\n";
  f << "                0.00      0.000\n";
  f << "                " << Mcrit << "      0.000\n";
  f << "                1.10      0.023\n";
  f << "                1.80      0.015\n";
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  double CDflaps_per_deg = CDflaps / 30.0;
  f << "    <coefficient name=\"CDflap\">\n"; 
  f << "       <description>Drag_due_to_flaps</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/flap-pos-deg</property>\n";
  f << "           <value> " << CDflaps_per_deg << " </value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  if (retractable) {
    f << "    <coefficient name=\"CDgear\">\n"; 
    f << "       <description>Drag_due_to_gear</description>\n";
    f << "       <function>\n";
    f << "         <product>\n";
    f << "           <property>aero/qbar-psf</property>\n";
    f << "           <property>metrics/Sw-sqft</property>\n";
    f << "           <property>gear/gear-pos-norm</property>\n";
    f << "           <value>" << CDgear << "</value>\n";
    f << "         </product>\n";
    f << "       </function>\n";
    f << "    </coefficient>\n\n";
  }

  f << "    <coefficient name=\"CDsb\">\n"; 
  f << "       <description>Drag_due_to_speedbrakes</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/speedbrake-pos-norm</property>\n";
  f << "           <value>" << CDspeedbrake << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  double CDb26 = CDbeta * 0.25;  // CD at beta of 0.26 radians
  f << "    <coefficient name=\"CDbeta\">\n"; 
  f << "       <description>Drag_due_to_sideslip</description>\n";
  f << "       <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">aero/beta-rad</independentVar>\n";
  f << "            <tableData>\n";
  f << "              -1.57       1.230\n";
  f << "              -0.26    " << CDb26 << endl;   
  f << "               0.00       0.000\n";
  f << "               0.26    " << CDb26 << endl;   
  f << "               1.57       1.230\n";
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"CDde\">\n"; 
  f << "       <description>Drag_due_to_Elevator_Deflection</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>fcs/elevator-pos-norm</property>\n";
  f << "           <value>" << CDde << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";

  //***** SIDE *************************************************

  f << "  <axis name=\"SIDE\">\n\n";

  f << "    <coefficient name=\"CYb\">\n";
  f << "       <description>Side_force_due_to_beta</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>aero/beta-rad</property>\n";
  f << "           <value>" << CYbeta << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";

  //***** ROLL ************************************************

  f << "  <axis name=\"ROLL\">\n\n";

  f << "    <coefficient name=\"Clb\">\n";
  f << "       <description>Roll_moment_due_to_beta</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>aero/beta-rad</property>\n";
  f << "           <value>" << Clbeta << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Clp\">\n";
  f << "       <description>Roll_moment_due_to_roll_rate</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>aero/bi2vel</property>\n";
  f << "           <property>velocities/p-aero-rad_sec</property>\n";
  f << "           <value>" << Clp << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Clr\">\n";
  f << "       <description>Roll_moment_due_to_yaw_rate</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>aero/bi2vel</property>\n";
  f << "           <property>velocities/r-aero-rad_sec</property>\n";
  f << "           <value>" << Clr << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  double Clda3 = Clda * 0.333;
  f << "    <coefficient name=\"Clda\">\n";
  f << "       <description>Roll_moment_due_to_aileron</description>\n";
  f << "       <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <property>metrics/bw-ft</property>\n";
  f << "          <property>fcs/left-aileron-pos-rad</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";
  f << "            <tableData>\n";
  f << "              0.0    " << Clda << endl;
  f << "              2.0    " << Clda3 << endl;
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cldr\">\n";
  f << "       <description>Roll_moment_due_to_rudder</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>fcs/rudder-pos-rad</property>\n";
  f << "           <value>" << Cldr << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";

  //***** PITCH *****************************************

  f << "  <axis name=\"PITCH\">\n\n";

  f << "    <coefficient name=\"Cmalpha\">\n";
  f << "       <description>Pitch_moment_due_to_alpha</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/cbarw-ft</property>\n";
  f << "           <property>aero/alpha-rad</property>\n";
  f << "           <value>" << Cmalpha << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  double Cmde4 = Cmde * 0.25;
  f << "    <coefficient name=\"Cmde\">\n";
  f << "       <description>Pitch_moment_due_to_elevator</description>\n";
  f << "       <function>\n";
  f << "        <product>\n";
  f << "          <property>aero/qbar-psf</property>\n";
  f << "          <property>metrics/Sw-sqft</property>\n";
  f << "          <property>metrics/cbarw-ft</property>\n";
  f << "          <property>fcs/elevator-pos-rad</property>\n";
  f << "          <table>\n";
  f << "            <independentVar lookup=\"row\">velocities/mach-norm</independentVar>\n";
  f << "            <tableData>\n";
  f << "              0.0     " << Cmde << endl;
  f << "              2.0     " << Cmde4 << endl;
  f << "            </tableData>\n";
  f << "          </table>\n";
  f << "        </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cmq\">\n";
  f << "       <description>Pitch_moment_due_to_pitch_rate</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/cbarw-ft</property>\n";
  f << "           <property>aero/ci2vel</property>\n";
  f << "           <property>velocities/q-aero-rad_sec</property>\n";
  f << "           <value>" << Cmq << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cmadot\">\n";
  f << "       <description>Pitch_moment_due_to_alpha_rate</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/cbarw-ft</property>\n";
  f << "           <property>aero/ci2vel</property>\n";
  f << "           <property>aero/alphadot-rad_sec</property>\n";
  f << "           <value>" << Cmadot << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";

  //***** YAW ***********************************************

  f << "  <axis name=\"YAW\">\n\n";

  f << "    <coefficient name=\"Cnb\">\n";
  f << "       <description>Yaw_moment_due_to_beta</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>aero/beta-rad</property>\n";
  f << "           <value>" << Cnbeta << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cnr\">\n";
  f << "       <description>Yaw_moment_due_to_yaw_rate</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>aero/bi2vel</property>\n";
  f << "           <property>velocities/r-aero-rad_sec</property>\n";
  f << "           <value>" << Cnr << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cndr\">\n";
  f << "       <description>Yaw_moment_due_to_rudder</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>fcs/rudder-pos-rad</property>\n";
  f << "           <value>" << Cndr << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "    <coefficient name=\"Cnda\">\n";
  f << "       <description>Adverse_yaw</description>\n";
  f << "       <function>\n";
  f << "         <product>\n";
  f << "           <property>aero/qbar-psf</property>\n";
  f << "           <property>metrics/Sw-sqft</property>\n";
  f << "           <property>metrics/bw-ft</property>\n";
  f << "           <property>fcs/left-aileron-pos-rad</property>\n";
  f << "           <value>" << Cnda << "</value>\n";
  f << "         </product>\n";
  f << "       </function>\n";
  f << "    </coefficient>\n\n";

  f << "  </axis>\n\n";
  f << " </aerodynamics>\n\n";
  f << "</fdm_config>\n";

  f << flush;
  f.close();
  return filename;
}

void Aeromatic::Reset() {
  AircraftName = EngineName = PropName = "unnamed";
  aType = atLtSingle;
  MTOW = 5000.0;
  wingspan = 30.0;
  length = 30.0;
  wingarea = 0.0;   // this allows aeromatic to estimate wing area
  tricycle = true;
  retractable = true;
  engines = 1;
  eType = etPiston;
  elType = elFwd_Fuselage;
  yawdamper = false;
  enginePower = 400.0;
  engineRPM = 2700.0;
  fixedpitch = false;
  diameter = 6.0;
  engineThrust = 2000.0;
  augmentation = false;
  injection = false;
}

// } // namespace

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  

