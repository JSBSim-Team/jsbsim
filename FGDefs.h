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

#define MAX_ENGINES     10
#define MAX_TANKS       30
#define GRAVITY         32.174
#define INVGRAVITY      0.031081
#define EARTHRAD        20898908.00       // feet
#define OMEGAEARTH      7.2685E-3         // rad/sec
#define EARTHRADSQRD    437882827922500.0
#define ONESECOND       4.848136811E-6
#define ECCENT          0.996647186
#define ECCENTSQRD      0.99330561
#define INVECCENTSQRD   1.0067395
#define INVECCENTSQRDM1 0.0067395
#define EPS             0.081819221
#define Reng            1716             //Specific Gas Constant,ft^2/(sec^2*R)
#define SHRATIO         1.4              //Specific Heat Ratio
#define RADTODEG        57.29578
#define DEGTORAD        1.745329E-2
#define KTSTOFPS        1.68781
#define FPSTOKTS        0.592484
#define NEEDED_CFG_VERSION "1.30"

#define HPTOFTLBSSEC 550
#define METERS_TO_FEET 3.2808

#define FG_QBAR           1
#define FG_WINGAREA       2
#define FG_WINGSPAN       4
#define FG_CBAR           8
#define FG_ALPHA          16
#define FG_ALPHADOT       32
#define FG_BETA           64
#define FG_BETADOT        128
#define FG_PITCHRATE      256
#define FG_ROLLRATE       512
#define FG_YAWRATE        1024
#define FG_MACH           2048
#define FG_ALTITUDE       4096
#define FG_BI2VEL         8192
#define FG_CI2VEL         16384
#define FG_ELEVATOR_POS   32768L
#define FG_AILERON_POS    65536L
#define FG_RUDDER_POS     131072L
#define FG_SPDBRAKE_POS   262144L
#define FG_SPOILERS_POS   524288L
#define FG_FLAPS_POS      1048576L
#define FG_ELEVATOR_CMD   2097152L
#define FG_AILERON_CMD    4194304L
#define FG_RUDDER_CMD     8388608L
#define FG_SPDBRAKE_CMD   16777216L
#define FG_SPOILERS_CMD   33554432L
#define FG_FLAPS_CMD      67108864L
#define FG_SPARE3         134217728L
#define FG_SPARE4         268435456L
#define FG_SPARE5         536870912L
#define FG_SPARE6         1073741824L

/******************************************************************************/
#endif

