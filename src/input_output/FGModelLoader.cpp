/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGModelLoader.cpp
 Author:       Bertrand Coconnier
 Date started: 12/14/13
 Purpose:      Read and manage XML data for models definition

  ------------- Copyright (C) 2013 Bertrand Coconnier -------------

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
This is the place where the XML data is loaded in memory for an access during
the models initialization.

HISTORY
--------------------------------------------------------------------------------
12/14/13   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGModelLoader.h"
#include "FGXMLFileRead.h"
#include "models/FGModel.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Element_ptr FGModelLoader::Open(Element *el)
{
  Element_ptr document = el;
  string fname = el->GetAttributeValue("file");

  if (!fname.empty()) {
    FGXMLFileRead XMLFileRead;
    SGPath path(SGPath::fromUtf8(fname.c_str()));

    if (path.isRelative())
      path = model->FindFullPathName(path);

    if (CachedFiles.find(path.utf8Str()) != CachedFiles.end())
      document = CachedFiles[path.utf8Str()];
    else {
      document = XMLFileRead.LoadXMLDocument(path);
      if (document == 0L) {
        FGXMLLogging log(model->GetExec()->GetLogger(), el, LogLevel::ERROR);
        log << "Could not open file: " << fname << endl;
        return NULL;
      }
      CachedFiles[path.utf8Str()] = document;
    }

    if (document->GetName() != el->GetName()) {
      document->SetParent(el);
      el->AddChildElement(document);
    }
  }

  return document;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPath CheckPathName(const SGPath& path, const SGPath& filename) {
  SGPath fullName = path/filename.utf8Str();

  if (fullName.extension() != "xml")
    fullName.concat(".xml");

  return fullName.exists() ? fullName : SGPath();
}
}
