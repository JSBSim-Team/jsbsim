/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputFile.cpp
 Author:       Bertrand Coconnier
 Date started: 09/10/11
 Purpose:      Manage output of sim parameters to a file
 Called by:    FGOutput

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
09/10/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sstream>

#include "FGOutputFile.h"
#include "FGFDMExec.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutputFile.cpp,v 1.1 2012/09/05 21:49:19 bcoconni Exp $";
static const char *IdHdr = ID_OUTPUTFILE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutputFile::FGOutputFile(FGFDMExec* fdmex, Element* element, int idx) :
  FGOutputType(fdmex, element, idx),
  runID_postfix(0)
{
  BaseFilename = Filename = FDMExec->GetRootDir() + element->GetAttributeValue("name");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutputFile::FGOutputFile(FGFDMExec* fdmex, int idx, int subSystems,
                           std::string name, double outRate,
                           std::vector<FGPropertyManager *> & outputProperties) :
  FGOutputType(fdmex, idx, subSystems, outRate, outputProperties),
  runID_postfix(0)
{
  BaseFilename = Filename = FDMExec->GetRootDir() + name;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputFile::InitModel(void)
{
  if (FGOutputType::InitModel()) {
    OpenFile();
    return true;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputFile::SetStartNewOutput(void)
{
  if (Filename.size() > 0) {
    ostringstream buf;
    string::size_type dot = BaseFilename.find_last_of('.');
    if (dot != string::npos) {
      buf << BaseFilename.substr(0, dot) << '_' << runID_postfix++ << BaseFilename.substr(dot);
    } else {
      buf << BaseFilename << '_' << runID_postfix++;
    }
    Filename = buf.str();
    CloseFile();
  }
}
}
