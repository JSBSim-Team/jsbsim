/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPosition.h
 Author:       Jon S. Berndt
 Date started: 1/5/99
 
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
 
HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPOSITION_H
#define FGPOSITION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGMatrix.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_POSITION "$Id: FGPosition.h,v 1.35 2001/04/26 23:46:46 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the lateral and longitudinal translational EOM.
    @author Jon S. Berndt
    @version $Id: FGPosition.h,v 1.35 2001/04/26 23:46:46 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPosition : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGPosition(FGFDMExec*);
  /// Destructor
  ~FGPosition();

  /** Runs the Position model; called by the Executive
      @see JSBSim.cpp documentation
      @return false if no error */
  bool Run(void);
  
  inline FGColumnVector GetVel(void) { return vVel; }
  inline FGColumnVector GetVelDot(void) { return vVelDot; }
  inline FGColumnVector GetUVW(void) { return vUVW; }
  inline double GetVn(void)  { return vVel(eX); }
  inline double GetVe(void)  { return vVel(eY); }
  inline double GetVd(void)  { return vVel(eZ); }
  inline double GetVground(void) { return Vground; }
  inline double GetGroundTrack(void) { return psigt; }
  inline double Geth(void)  { return h; }
  inline double Gethdot(void) { return RadiusDot; }
  inline double GetLatitude(void) { return Latitude; }
  inline double GetLatitudeDot(void) { return LatitudeDot; }
  inline double GetLongitude(void) { return Longitude; }
  inline double GetLongitudeDot(void) { return LongitudeDot; }
  inline double GetRunwayRadius(void) { return RunwayRadius; }
  inline double GetDistanceAGL(void)  { return DistanceAGL; }
  inline double GetRadius(void) { return Radius; }
  inline FGColumnVector GetRunwayNormal(void) { return vRunwayNormal; }
  
  inline double GetGamma(void) { return gamma; }
  inline void SetGamma(float tt) { gamma = tt; }
  inline double GetHOverB(void) { return hoverb; }
  void SetvVel(const FGColumnVector& v) { vVel = v; }
  void SetLatitude(float tt) { Latitude = tt; }
  void SetLongitude(double tt) { Longitude = tt; }
  void Seth(double tt);
  void SetRunwayRadius(double tt) { RunwayRadius = tt; }
  void SetSeaLevelRadius(double tt) { SeaLevelRadius = tt;}
  void SetDistanceAGL(double tt);
  inline void SetRunwayNormal(double fgx, double fgy, double fgz ) {
      vRunwayNormal << fgx << fgy << fgz;
  }
  
private:  
  FGColumnVector vUVW;
  FGColumnVector vVel;
  FGColumnVector vVelDot;
  FGColumnVector vRunwayNormal;
  
  double Vee, invMass, invRadius;
  double Radius, h;
  double LatitudeDot, LongitudeDot, RadiusDot;
  double lastLatitudeDot, lastLongitudeDot, lastRadiusDot;
  double Longitude, Latitude;
  float dt;
  double RunwayRadius;
  double DistanceAGL;
  double SeaLevelRadius;
  double gamma;
  double Vt, Vground;
  double hoverb,b;

  double psigt;

  void GetState(void);
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

