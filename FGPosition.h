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
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_POSITION "$Id: FGPosition.h,v 1.49 2003/01/22 15:53:34 jberndt Exp $"

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
    @version $Id: FGPosition.h,v 1.49 2003/01/22 15:53:34 jberndt Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGPosition.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGPosition.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGPosition : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGPosition(FGFDMExec*);

  /// Destructor
  ~FGPosition();

  bool InitModel(void);

  /** Runs the Position model; called by the Executive
      @see JSBSim.cpp documentation
      @return false if no error */
  bool Run(void);
  
  inline FGColumnVector3& GetVel(void) { return vVel; }
  inline FGColumnVector3& GetVelDot(void) { return vVelDot; }
  inline double GetVn(void)  const { return vVel(eX); }
  inline double GetVe(void)  const { return vVel(eY); }
  inline double GetVd(void)  const { return vVel(eZ); }
  inline double GetVground(void) const { return Vground; }
  inline double GetGroundTrack(void) const { return psigt; }
  inline double Geth(void)  const { return h; }
  inline double Gethdot(void) const { return RadiusDot; }
  inline double GetLatitude(void) const { return Latitude; }
  inline double GetLatitudeDot(void) const { return LatitudeDot; }
  inline double GetLongitude(void) const { return Longitude; }
  inline double GetLongitudeDot(void) const { return LongitudeDot; }
  inline double GetRunwayRadius(void) const { return RunwayRadius; }
  inline double GetDistanceAGL(void)  const { return DistanceAGL; }
  inline double GetRadius(void) const { return Radius; }
  inline FGColumnVector3& GetRunwayNormal(void) { return vRunwayNormal; }
  
  inline double GetGamma(void) const { return gamma; }
  inline void SetGamma(double tt) { gamma = tt; }
  inline double GetHOverBCG(void) const { return hoverbcg; }
  inline double GetHOverBMAC(void) const { return hoverbmac; }
  void SetvVel(const FGColumnVector3& v) { vVel = v; }
  void SetLatitude(double tt) { Latitude = tt; }
  void SetLongitude(double tt) { Longitude = tt; }
  void Seth(double tt);
  void SetRunwayRadius(double tt) { RunwayRadius = tt; }
  void SetSeaLevelRadius(double tt) { SeaLevelRadius = tt;}
  void SetDistanceAGL(double tt);
  inline void SetRunwayNormal(double fgx, double fgy, double fgz ) {
      vRunwayNormal << fgx << fgy << fgz;
  }
  
  void bind(void);
  void unbind(void);
  
private:  
  FGColumnVector3 vVel;
  FGColumnVector3 vVelDot;
  FGColumnVector3 vRunwayNormal;
  
  double Radius, h;
  double LatitudeDot, LongitudeDot, RadiusDot;
  double LatitudeDot_prev[3], LongitudeDot_prev[3], RadiusDot_prev[3];
  double Longitude, Latitude;
  double dt;
  double RunwayRadius;
  double DistanceAGL;
  double SeaLevelRadius;
  double gamma;
  double Vt, Vground;
  double hoverbcg,hoverbmac,b;

  double psigt;

  void GetState(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

