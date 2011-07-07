// coremag.hxx -- compute local magnetic variation given position,
//                altitude, and date
//
// This is an implimentation of the NIMA WMM 2000
//
//    http://www.nima.mil/GandG/ngdc-wmm2000.html
//
// Copyright (C) 2000  Edward A Williams <Ed_Williams@compuserve.com>
//
// Adapted from Excel 3.0 version 3/27/94 EAW
// Recoded in C++ by Starry Chan
// WMM95 added and rearranged in ANSI-C EAW 7/9/95
// Put shell around program and made Borland & GCC compatible EAW 11/22/95
// IGRF95 added 2/96 EAW
// WMM2000 IGR2000 added 2/00 EAW
// Released under GPL 3/26/00 EAW
// Adaptions and modifications for the SimGear project  3/27/2000 CLO
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id: coremag.hxx,v 1.1 2009/09/03 12:27:07 jberndt Exp $


#ifndef SG_MAGVAR_HXX
#define SG_MAGVAR_HXX

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/* Convert date to Julian day    1950-2049 */
unsigned long int yymmdd_to_julian_days( int yy, int mm, int dd );

/* return variation (in degrees) given geodetic latitude (radians), longitude
(radians) ,height (km) and (Julian) date
N and E lat and long are positive, S and W negative
*/
double calc_magvar( double lat, double lon, double h, long dat, double* field );


#endif // SG_MAGVAR_HXX
