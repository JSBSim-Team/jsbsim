/*******************************************************************************

 Header:       FGGFCS.h
 Author:       Jon S. Berndt
 Date started: 12/12/98

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
12/12/98   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFCS_H
#define FGFCS_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
   FG_USING_STD(string);
#else
#  include <string>
#endif

#include "FGModel.h"
#include "FGConfigFile.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

using namespace std;

class FGFCS : public FGModel
{
private:
  /// elevator, rudder, aileron, flaps, throttle, slats scaling
  float eScale, rScale, aScale, fScale, tScale, sScale;
public:
	FGFCS(FGFDMExec*);
	~FGFCS(void);

	bool Run(void);

	inline float GetDa(void) {return Da;}
	inline float GetDe(void) {return De;}
	inline float GetDr(void) {return Dr;}
	inline float GetDf(void) {return Df;}
	inline float GetDs(void) {return Ds;}
	inline float GetThrottle(int ii) {return Throttle[ii];}

	inline void SetDa(float tt) {Da = tt*aScale;} //0.3
	inline void SetDe(float tt) {De = tt*eScale;} //0.6
	inline void SetDr(float tt) {Dr = tt*rScale;} //-1.0 * 0.5
	inline void SetDf(float tt) {Df = tt*fScale;}
	inline void SetDs(float tt) {Ds = tt*sScale;}
	void SetThrottle(int ii, float tt);
  LoadFCS(FGConfigFile* AC_cfg);
  string FCSName;

protected:

private:
  float Da, De, Dr, Df, Ds;
  float Throttle[MAX_ENGINES];
};

/******************************************************************************/
#endif
