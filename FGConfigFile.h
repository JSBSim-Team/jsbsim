/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGCONFIGFILE_H
#define FGCONFIGFILE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
#  include STL_FSTREAM
#  include STL_IOSTREAM
   SG_USING_STD(string);
   SG_USING_STD(ostream);
   SG_USING_STD(istream);
   SG_USING_STD(ifstream);
   SG_USING_STD(cerr);
   SG_USING_STD(endl);
   SG_USING_STD(ios);
   SG_USING_STD(cout);
#else
#  include <string>
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <fstream.h>
#    include <iostream.h>
#  else
#    include <fstream>
#    include <iostream>
     using std::ostream;
     using std::istream;
     using std::ios;
     using std::cerr;
     using std::cout;
     using std::ifstream;
     using std::endl;
#  endif
    using std::string;
#endif

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_CONFIGFILE "$Id: FGConfigFile.h,v 1.43 2003/12/29 10:57:39 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates reading a JSBSim config file.
    JSBSim config files are in XML format.
    @author Jon S. Berndt
    @version $Id: FGConfigFile.h,v 1.43 2003/12/29 10:57:39 ehofman Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGConfigFile : public FGJSBBase
{
public:
  /** Constructor
      @param Filename the name of the config file to be read. */
  FGConfigFile(string Filename);
  /// Destructor
  ~FGConfigFile();

  /** Returns the next line from the currently open config file.
      Comments are bypassed and ignored.
      @return the next valid line from the config file OR "EOF" if end of file is
      reached.*/
  string GetNextConfigLine(void);
  
  string GetCurrentLine(void) { return CurrentLine; }

  /** Returns the value of the tag supplied.
      @param tag the tag for the value that is desired.
      @return tthe value of the tag supplied.*/
  string GetValue(string tag);

  string GetValue(void);
  string GetCommentString(void) {return CommentString;}
  string GetLineComment(void) {return LineComment;}
  bool IsOpen(void) {return Opened;}
  FGConfigFile& operator>>(double&);
  FGConfigFile& operator>>(int&);
  FGConfigFile& operator>>(string&);
  void ResetLineIndexToZero(void);

private:
  ifstream cfgfile;
  string   CurrentLine;
  string   CommentString;
  string   LineComment;
  bool     CommentsOn;
  bool     Opened;
  unsigned int CurrentIndex;
  string GetLine(void);

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

