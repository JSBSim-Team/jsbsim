/*******************************************************************************
 
 Header:       FGDefs.h
 Author:       Jon S. Berndt
 Date started: 02/01/99
 
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
02/01/99  JSB   Created
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGDEFS_H
#define FGDEFS_H

#define Reng            1716             //Specific Gas Constant,ft^2/(sec^2*R)
#define SHRATIO         1.4              //Specific Heat Ratio
#define KTSTOFPS        1.68781
#define FPSTOKTS        0.592484
#define INCHTOFT        0.08333333
#define NEEDED_CFG_VERSION "1.55"
#define JSBSIM_VERSION  "0.9.1"

#if defined ( sgi ) && !defined( __GNUC__ )
#define __STL_FUNCTION_TMPL_PARTIAL_ORDER
#endif

enum eParam {
  FG_UNDEF = 0,
  FG_TIME,
  FG_QBAR,
  FG_WINGAREA,
  FG_WINGSPAN,
  FG_CBAR,
  FG_ALPHA,
  FG_ALPHADOT,
  FG_BETA,
  FG_ABETA,
  FG_BETADOT,
  FG_PHI,
  FG_THT,
  FG_PSI,
  FG_PITCHRATE,
  FG_ROLLRATE,
  FG_YAWRATE,
  FG_CL_SQRD,
  FG_MACH,
  FG_ALTITUDE,
  FG_BI2VEL,
  FG_CI2VEL,
  FG_ELEVATOR_POS,
  FG_AILERON_POS,
  FG_RUDDER_POS,
  FG_SPDBRAKE_POS,
  FG_SPOILERS_POS,
  FG_FLAPS_POS,
  FG_ELEVATOR_CMD,
  FG_AILERON_CMD,
  FG_RUDDER_CMD,
  FG_SPDBRAKE_CMD,
  FG_SPOILERS_CMD,
  FG_FLAPS_CMD,
  FG_THROTTLE_CMD,
  FG_THROTTLE_POS,
  FG_MIXTURE_CMD,
  FG_MIXTURE_POS,
  FG_MAGNETO_CMD,
  FG_STARTER_CMD,
  FG_ACTIVE_ENGINE,
  FG_HOVERB,
  FG_PITCH_TRIM_CMD,
  FG_YAW_TRIM_CMD,
  FG_ROLL_TRIM_CMD,
  FG_LEFT_BRAKE_CMD,
  FG_CENTER_BRAKE_CMD,
  FG_RIGHT_BRAKE_CMD,
  FG_SET_LOGGING,
  FG_ALPHAH,
  FG_ALPHAW,
  FG_LBARH,     //normalized horizontal tail arm
  FG_LBARV,     //normalized vertical tail arm
  FG_HTAILAREA,
  FG_VTAILAREA,
  FG_VBARH,    //horizontal tail volume 
  FG_VBARV     //vertical tail volume 
};

enum eAction {
  FG_RAMP  = 1,
  FG_STEP  = 2,
  FG_EXP   = 3
};

enum eType {
  FG_VALUE = 1,
  FG_DELTA = 2,
  FG_BOOL  = 3
};

/******************************************************************************/
#endif

