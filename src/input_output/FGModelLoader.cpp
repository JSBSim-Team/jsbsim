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

#include "FGJSBBase.h"
#include "FGModelLoader.h"
#include "FGXMLFileRead.h"
#include "models/FGModel.h"

using namespace std;

namespace JSBSim {

IDENT(IdSrc, "$Id: FGModelLoader.cpp,v 1.3 2015/07/12 12:41:55 bcoconni Exp $");
IDENT(IdHdr, ID_MODELLOADER);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Element_ptr FGModelLoader::Open(Element *el)
{
  Element_ptr document = el;
  string fname = el->GetAttributeValue("file");

  if (!fname.empty()) {
    FGXMLFileRead XMLFileRead;
    string file;

    try {
      file = model->FindFullPathName(fname);
      if (file.empty()) throw string("File does not exist.");
    }
    catch(string& e) {
      cerr << endl << el->ReadFrom()
           << "Could not open file: " << fname << endl << e << endl;
      return NULL;
    }

    if (CachedFiles.find(file) != CachedFiles.end())
      document = CachedFiles[file];
    else {
      document = XMLFileRead.LoadXMLDocument(file);
      if (document == 0L) {
        cerr << endl << el->ReadFrom()
             << "Could not open file: " << file << endl;
        return NULL;
      }
      CachedFiles[file] = document;
    }

    if (document->GetName() != el->GetName()) {
      document->SetParent(el);
      el->AddChildElement(document);
    }
  }

  return document;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string CheckFullPathName(const string& path, const string& fname)
{
  string name = path + "/" + fname;

  if (name.length() <=4 || name.substr(name.length()-4, 4) != ".xml")
    name.append(".xml");

  ifstream file(name.c_str());
  if (!file.is_open())
    return string();

  return name;
}
}
