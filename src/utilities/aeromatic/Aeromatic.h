/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       Aeromatic.h
 Author:       David Culp
               Implementation of the Aero-Matic web application
 Date started: 04/26/05
 
 ------------- Copyright (C) 20005  David Culp (davidculp2@comcast.net)---------
 
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
 
HISTORY
--------------------------------------------------------------------------------
04/26/05   DPC   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef Aeromatic_H
#define Aeromatic_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#include <iostream>
#include <math.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AEROMATIC
#define AEROMATIC_VERSION 0.8

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//namespace JSBSim {
using namespace std;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class encapsulates the Aero-Matic web application, which is used to
    generate plausible JSBSim configuration files based on a few questions
    about an aircraft's type and metrics. 

    To use this class you must first supply it with useful data, for example,
    to get an engine configuration file for a CFM-56:

       Aeromatic* am = new Aeromatic();
       am->SetEngineName("CFM-56");
       am->SetEngineType(Aeromatic::etTurbine);
       am->SetEngineThrust(20000.0);
       string engine_filename = am->PrintEngine();

  
    @author David Culp
    @version $Id: Aeromatic.h
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class Aeromatic {
public:

  /// Constructor
  Aeromatic();
  /// Destructor
  ~Aeromatic();

  /// Resets all variables to their initial state
  void Reset();

  enum aircraftType { atGlider, atLtSingle, atLtTwin, atRacer, atSEFighter, 
          at2EFighter, at2ETransport, at3ETransport, at4ETransport, atMEProp };

  enum engineType { etPiston, etTurbine, etTurboprop, etRocket };

  enum engineLayoutType { elFwd_Fuselage, elMid_Fuselage, elAft_Fuselage, elWings,
          elWings_Tail, elWings_Nose }; 

  // Setters for user input
  void SetAircraftName(string n) { AircraftName=n; }
  void SetEngineName(string n) { EngineName=n; }
  void SetPropName(string n) { PropName=n; }
  void SetAircraftType(aircraftType t) { aType=t; }
  void SetMTOW(double mtow) { MTOW=mtow; }
  void SetWingspan(double s) { wingspan=s; }
  void SetLength(double len) { length=len; }
  void SetWingArea(double S) { wingarea=S; }
  void SetTricycle(bool t) { tricycle=t; }
  void SetRetractable(bool r) { retractable=r; }
  void SetNumEngines(int n) { engines=n; }
  void SetEngineType(engineType et) { eType=et; }
  void SetEngineLayout(engineLayoutType el) { elType=el; }
  void SetYawDamper(bool d) { yawdamper=d; }
  void SetEnginePower(double p) { enginePower=p; }
  void SetEngineRPM(double r) { engineRPM=r; }
  void SetFixedPitch(bool f) { fixedpitch=f; }
  void SetPropDiameter(double d) { diameter=d; }
  void SetEngineThrust(double t) { engineThrust=t; }
  void SetAugmented(bool a) { augmentation=a; }
  void SetInjected(bool i) { injection=i; }
 
  // Functions to retrieve Aero-Matic output (print to XML file).
  // Return value is file name.
  string PrintEngine();
  string PrintProp();
  string PrintAero();  

private:

  string       AircraftName, EngineName, PropName;
  aircraftType aType;
  double       MTOW;      // Maximum Takeoff Weight, in pounds
  double       wingspan;  // Wing span, in feet
  double       length;    // Aircraft length, in feet  
  double       wingarea;  // Planform area of wing, in square feet
  bool         tricycle;  // True if aircraft has tricycle landing gear.
                          // False implies a tail-dragger configuration.
  bool      retractable;  // True if gear is retractable.
  int          engines;   // Number of engines.
  engineType   eType;     // Type of engines.  One type for all!
  engineLayoutType elType;  // Engine locations.
  bool      yawdamper;    // True if yaw damper installed

  double   enginePower;   // Engine horsepower. Max at sea level.
  double   engineRPM;     // Maximum rated engine rpm.
  bool     fixedpitch;    // True if prop pitch is fixed.
                          // False implies variable pitch.
  double     diameter;    // Propeller diameter in feet.

  double   engineThrust;  // Engine thrust in pounds, static at sea level,
                          // without afterburning.
  bool     augmentation;  // True if augmentation (afterburner) installed.
  bool     injection;     // True if water or nitrous injection installed.

};

// } // namespace

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

