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
#  if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
     SG_USING_STD(ostream);
     SG_USING_STD(istream);
     SG_USING_STD(ifstream);
     SG_USING_STD(cerr);
     SG_USING_STD(endl);
     SG_USING_STD(ios);
     SG_USING_STD(cout);
#  endif
#else
#  include <string>
#  if defined(sgi) && !defined(__GNUC__)
#    include <fstream.h>
#    include <iostream.h>
#  else
#    include <fstream>
#    include <iostream>
     using std::ostream;
     using std::istream;
     using std::ifstream;
     using std::ios;
     using std::cerr;
     using std::endl;
     using std::cout;
#  endif
    using std::string;
#endif

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_CONFIGFILE "$Id: FGConfigFile.h,v 1.35 2002/09/22 18:10:05 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates reading a JSBSim config file.
    JSBSim config files are in XML format.
    @author Jon S. Berndt
    @version $Id: FGConfigFile.h,v 1.35 2002/09/22 18:10:05 apeden Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGConfigFile.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGConfigFile.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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
      @param 
      @return */
  string GetValue(string);
  string GetValue(void);
  string GetCommentString(void) {return CommentString;}
  string GetLineComment(void) {return LineComment;}
  bool IsOpen(void) {return Opened;}
  FGConfigFile& operator>>(double&);
  FGConfigFile& operator>>(int&);
  FGConfigFile& operator>>(string&);
  FGConfigFile& operator>>(eParam&);
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

