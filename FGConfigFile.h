/*******************************************************************************

 Header:       FGConfigFile.h
 Author:       Jon Berndt
 Date started: 03/29/00

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
03/29/00   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGCONFIGFILE_H
#define FGCONFIGFILE_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
#else
#  include <fstream>
#endif

#include <string>

/*******************************************************************************
DEFINES
*******************************************************************************/

#ifndef FGFS
using std::string;
using std::ifstream;
#endif

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGConfigFile
{
public:
  FGConfigFile(string);
  ~FGConfigFile(void);

  string GetLine(void);
  string GetNextConfigLine(void);
  string GetValue(string);
  string GetValue(void);
  bool IsCommentLine(void);
  bool IsOpen(void) {return Opened;}
  FGConfigFile& operator>>(double&);
  FGConfigFile& operator>>(float&);
  FGConfigFile& operator>>(int&);
  FGConfigFile& operator>>(string&);
  void ResetLineIndexToZero(void);

protected:

private:
  ifstream cfgfile;
  string   CurrentLine;
  bool     CommentsOn;
  bool     Opened;
  unsigned int      CurrentIndex;
};

/******************************************************************************/
#endif
