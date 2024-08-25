/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGJSBBase.cpp
 Author:       Jon S. Berndt
 Date started: 07/01/01
 Purpose:      Encapsulates the JSBBase object

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
07/01/01  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "models/FGAtmosphere.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

char FGJSBBase::highint[5]  = {27, '[', '1', 'm', '\0'      };
char FGJSBBase::halfint[5]  = {27, '[', '2', 'm', '\0'      };
char FGJSBBase::normint[6]  = {27, '[', '2', '2', 'm', '\0' };
char FGJSBBase::reset[5]    = {27, '[', '0', 'm', '\0'      };
char FGJSBBase::underon[5]  = {27, '[', '4', 'm', '\0'      };
char FGJSBBase::underoff[6] = {27, '[', '2', '4', 'm', '\0' };
char FGJSBBase::fgblue[6]   = {27, '[', '3', '4', 'm', '\0' };
char FGJSBBase::fgcyan[6]   = {27, '[', '3', '6', 'm', '\0' };
char FGJSBBase::fgred[6]    = {27, '[', '3', '1', 'm', '\0' };
char FGJSBBase::fggreen[6]  = {27, '[', '3', '2', 'm', '\0' };
char FGJSBBase::fgdef[6]    = {27, '[', '3', '9', 'm', '\0' };

const string FGJSBBase::needed_cfg_version = "2.0";
const string FGJSBBase::JSBSim_version = JSBSIM_VERSION " " __DATE__ " " __TIME__ ;

short FGJSBBase::debug_lvl  = 1;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::disableHighLighting(void)
{
  highint[0]='\0';
  halfint[0]='\0';
  normint[0]='\0';
  reset[0]='\0';
  underon[0]='\0';
  underoff[0]='\0';
  fgblue[0]='\0';
  fgcyan[0]='\0';
  fgred[0]='\0';
  fggreen[0]='\0';
  fgdef[0]='\0';
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGJSBBase::CreateIndexedPropertyName(const string& Property, int index)
{
  ostringstream buf;
  buf << Property << '[' << index << ']';
  return buf.str();
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim

