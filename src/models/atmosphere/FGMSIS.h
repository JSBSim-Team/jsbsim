/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       FGMSIS.h
 Description:  MSIS-00 Atmosphere
 Author:       David Culp
 Date started: 12/14/03
 
 ------------- Copyright (C) 2003  David P. Culp (davidculp2@comcast.net) ------
 
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
12/14/03   DPC   Created
01/11/04   DPC   Derive from FGAtmosphere
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMSIS_H
#define FGMSIS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <models/FGAtmosphere.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MSIS "$Id: FGMSIS.h,v 1.5 2009/03/25 13:52:44 dpculp Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the MSIS-00 atmosphere.
    This is a wrapper for the NRL-MSIS-00 model 2001:

    This C++ format model wraps the NRLMSISE-00 C source code package - release
    20020503
 
    The NRLMSISE-00 model was developed by Mike Picone, Alan Hedin, and
    Doug Drob. They also wrote a NRLMSISE-00 distribution package in 
    FORTRAN which is available at
    http://uap-www.nrl.navy.mil/models_web/msis/msis_home.htm
 
    Dominik Brodowski implemented and maintains this C version. You can
    reach him at devel@brodo.de. See the file "DOCUMENTATION" for details,
    and check http://www.brodo.de/english/pub/nrlmsise/index.html for
    updated releases of this package.
    @author David Culp
    @version $Id: FGMSIS.h,v 1.5 2009/03/25 13:52:44 dpculp Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
STRUCT DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

struct nrlmsise_flags {
  int switches[24];
  double sw[24];
  double swc[24];
};

struct ap_array {
  double a[7];   
};

struct nrlmsise_input {
  int year;      /* year, currently ignored                           */
  int doy;       /* day of year                                       */
  double sec;    /* seconds in day (UT)                               */
  double alt;    /* altitude in kilometers                            */
  double g_lat;  /* geodetic latitude                                 */
  double g_long; /* geodetic longitude                                */
  double lst;    /* local apparent solar time (hours), see note below */
  double f107A;  /* 81 day average of F10.7 flux (centered on DOY)    */
  double f107;   /* daily F10.7 flux for previous day                 */
  double ap;     /* magnetic index(daily)                             */
  struct ap_array *ap_a; /* see above */
};

struct nrlmsise_output {
  double d[9];   /* densities    */
  double t[2];   /* temperatures */
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MSIS : public FGAtmosphere
{
public:

  /// Constructor
  MSIS(FGFDMExec*);
  /// Destructor
  ~MSIS();
  /** Runs the MSIS-00 atmosphere model; called by the Executive
      @return false if no error */
  bool Run(void);

  bool InitModel(void);

  /// Does nothing. External control is not allowed.
  void UseExternal(void);

private:

  void Calculate(int day,      // day of year (1 to 366) 
                 double sec,   // seconds in day (0.0 to 86400.0)
                 double alt,   // altitude, feet
                 double lat,   // geodetic latitude, degrees
                 double lon    // geodetic longitude, degrees
                );

  void Debug(int from);

  nrlmsise_flags flags;
  nrlmsise_input input;
  nrlmsise_output output;
  ap_array aph;

  /* PARMB */
  double gsurf;
  double re;

  /* GTS3C */
  double dd;

  /* DMIX */
  double dm04, dm16, dm28, dm32, dm40, dm01, dm14;

  /* MESO7 */
  double meso_tn1[5];
  double meso_tn2[4];
  double meso_tn3[5];
  double meso_tgn1[2];
  double meso_tgn2[2];
  double meso_tgn3[2];

  /* LPOLY */
  double dfa;
  double plg[4][9];
  double ctloc, stloc;
  double c2tloc, s2tloc;
  double s3tloc, c3tloc;
  double apdf, apt[4];

  void tselec(struct nrlmsise_flags *flags);
  void glatf(double lat, double *gv, double *reff);
  double ccor(double alt, double r, double h1, double zh);
  double ccor2(double alt, double r, double h1, double zh, double h2);
  double scalh(double alt, double xm, double temp);
  double dnet(double dd, double dm, double zhm, double xmm, double xm);
  void splini(double *xa, double *ya, double *y2a, int n, double x, double *y);
  void splint(double *xa, double *ya, double *y2a, int n, double x, double *y);
  void spline(double *x, double *y, int n, double yp1, double ypn, double *y2);
  double zeta(double zz, double zl);
  double densm(double alt, double d0, double xm, double *tz, int mn3, double *zn3,
               double *tn3, double *tgn3, int mn2, double *zn2, double *tn2,
               double *tgn2);
  double densu(double alt, double dlb, double tinf, double tlb, double xm, 
               double alpha, double *tz, double zlb, double s2, int mn1, 
               double *zn1, double *tn1, double *tgn1);
  double g0(double a, double *p);
  double sumex(double ex);
  double sg0(double ex, double *p, double *ap);
  double globe7(double *p, nrlmsise_input *input, nrlmsise_flags *flags);
  double glob7s(double *p, nrlmsise_input *input, nrlmsise_flags *flags);
  void gtd7(nrlmsise_input *input, nrlmsise_flags *flags, nrlmsise_output *output);
  void gtd7d(nrlmsise_input *input, nrlmsise_flags *flags, nrlmsise_output *output); 
  void ghp7(nrlmsise_input *input, nrlmsise_flags *flags, nrlmsise_output *output, double press);
  void gts7(nrlmsise_input *input, nrlmsise_flags *flags, nrlmsise_output *output); 

};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

