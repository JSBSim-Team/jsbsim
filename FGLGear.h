/*******************************************************************************

 Header:       FGLGear.h
 Author:       Jon S. Berndt
 Date started: 11/18/99

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
11/18/99   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGLGEAR_H
#define FGLGEAR_H

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
	   Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
	   School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
	   JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
	   NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
	   Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
	   1982 ISBN 0-471-08936-2

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
   FG_USING_STD(string);
#else
#  include <string>
#endif

#include "FGConfigFile.h"

/*******************************************************************************
DEFINITIONS
*******************************************************************************/

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGLGear
{
public:
  FGLGear(FGConfigFile*);
  ~FGLGear(void);

  float Force(void);
private:
  float X, Y, Z;
  float kSpring, bDamp, compressLength;
  float statFCoeff, rollFCoeff, skidFCoeff;
  float frictionForce, compForce;
  float brakePct, brakeForce, brakeCoeff;
  string name;
};

/******************************************************************************/
#endif