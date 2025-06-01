/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGThruster.h
 Author:       Jon S. Berndt
 Date started: 08/23/00

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
08/24/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTHRUSTER_H
#define FGTHRUSTER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGForce.h"
#include "math/FGColumnVector3.h"
#include <string>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;
class FGPropertyManager;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for specific thrusting devices such as propellers, nozzles, etc.

<h3>reverser angle:</h3>

    "Reverser angle" as used here is a way to manipulate the thrust vector,
    along the thrust axis ONLY, during run time.  This should not be confused
    with a thrust vectoring nozzle.  The angle is defined in radians, and is
    used thus:  Final_thrust = cosine( reverser_angle ) * unmodified_thrust.  
    Therefore a reverser angle of 0 results in no change, and a reverser angle
    of 3.14 (pi) results in a completely reversed thrust vector.  An angle of
    1.57 (pi/2) results in no thrust at all.
 
    @author Jon Berndt
    */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGThruster : public FGForce {

public:
  /// Constructor
  FGThruster(FGFDMExec *FDMExec, Element *el, int num );
  /// Destructor
  virtual ~FGThruster();

  enum eType {ttNozzle, ttRotor, ttPropeller, ttDirect};

  virtual double Calculate(double tt) {
       Thrust = cos(ReverserAngle)*tt;
       vFn(1) = Thrust;
       return Thrust;
  }
  void SetName(std::string name) {Name = name;}
  virtual void SetRPM(double rpm) {};
  virtual void SetEngineRPM(double rpm) {};
  virtual double GetPowerRequired(void) {return 0.0;}
  double GetThrust(void) const {return Thrust;}
  eType GetType(void) {return Type;}
  std::string GetName(void) {return Name;}
  void SetReverserAngle(double angle) {ReverserAngle = angle;}
  double GetReverserAngle(void) const {return ReverserAngle;}
  virtual double GetRPM(void) const { return 0.0; };
  virtual double GetEngineRPM(void) const { return 0.0; };
  double GetGearRatio(void) {return GearRatio; }
  virtual std::string GetThrusterLabels(int id, const std::string& delimeter);
  virtual std::string GetThrusterValues(int id, const std::string& delimeter);

  virtual void ResetToIC(void);

  struct Inputs {
    double TotalDeltaT;
    double H_agl;
    FGColumnVector3 PQRi;
    FGColumnVector3 AeroPQR;
    FGColumnVector3 AeroUVW;
    double Density;
    double Pressure;
    double Soundspeed;
    double Alpha;
    double Beta;
    double Vt;
  } in;

protected:
  eType Type;
  std::string Name;
  double Thrust;
  double PowerRequired;
  double GearRatio;
  double ThrustCoeff;
  double ReverserAngle;
  int EngineNum;
  virtual void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
