/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGJSBBase.cpp
 Author:       Jon S. Berndt
 Date started: 07/01/01
 Purpose:      Encapsulates the JSBBase object

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
07/01/01  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"

static const char *IdSrc = "$Id: FGJSBBase.cpp,v 1.1 2001/08/11 21:16:53 jberndt Exp $";
static const char *IdHdr = ID_JSBBASE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGJSBBase::FGJSBBase(void) {
/*
  highint[5]  = {27, '[', '1', 'm', '\0'      };
  halfint[5]  = {27, '[', '2', 'm', '\0'      };
  normint[6]  = {27, '[', '2', '2', 'm', '\0' };
  reset[5]    = {27, '[', '0', 'm', '\0'      };
  underon[5]  = {27, '[', '4', 'm', '\0'      };
  underoff[6] = {27, '[', '2', '4', 'm', '\0' };
  fgblue[6]   = {27, '[', '3', '4', 'm', '\0' };
  fgcyan[6]   = {27, '[', '3', '6', 'm', '\0' };
  fgred[6]    = {27, '[', '3', '1', 'm', '\0' };
  fggreen[6]  = {27, '[', '3', '2', 'm', '\0' };
  fgdef[6]    = {27, '[', '3', '9', 'm', '\0' };
*/
}

