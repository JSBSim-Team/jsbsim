/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       JSBSim_API.h
 Author:       B. Coconnier
 Date started: Jun 05, 2022

 ---------- Copyright (C) 2022  B. Coconnier (bcoconni@users.sf.net) -----------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
Jun 05, 2022  BC  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef JSBSIM_API_H
#define JSBSIM_API_H

#if defined(_WIN32) && !defined(JSBSIM_STATIC_LINK)
  #ifdef JSBSIM_EXPORT
    #define JSBSIM_API __declspec(dllexport)
  #else
    #define JSBSIM_API __declspec(dllimport)
  #endif
#else
  #define JSBSIM_API
#endif

#endif /* JSBSIM_API_H */
