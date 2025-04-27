/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropertyManager.cpp
 Author:       Tony Peden
               Based on work originally by David Megginson
 Date:         2/2002

 ------------- Copyright (C) 2002 -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <assert.h>
#include "FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using namespace std;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Unbind(void)
{
  for(auto& property: tied_properties)
    property.untie();

  tied_properties.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Unbind(const void* instance)
{
  auto it = tied_properties.begin();

  while(it != tied_properties.end()) {
    auto property = it++;
    if (property->BindingInstance == instance) {
      property->untie();
      tied_properties.erase(property);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyManager::mkPropertyName(string name, bool lowercase) {

  /* do this two pass to avoid problems with characters getting skipped
     because the index changed */
  unsigned i;
  for(i=0;i<name.length();i++) {
    if( lowercase && isupper(name[i]) )
      name[i]=tolower(name[i]);
    else if( isspace(name[i]) )
      name[i]='-';
  }

  return name;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string GetPrintableName(const SGPropertyNode* node)
{
  string temp_string(node->getNameString());
  size_t initial_location=0;
  size_t found_location;

  found_location = temp_string.rfind("/");
  if (found_location != string::npos)
  temp_string = temp_string.substr(found_location);

  found_location = temp_string.find('_',initial_location);
  while (found_location != string::npos) {
    temp_string.replace(found_location,1," ");
    initial_location = found_location+1;
    found_location = temp_string.find('_',initial_location);
  }
  return temp_string;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string GetFullyQualifiedName(const SGPropertyNode* node)
{
  string fqname = node->getDisplayName(true);
  const SGPropertyNode* parent = node->getParent();
  if (!parent) return "/";  // node is the root.

  while(parent) {
    fqname = parent->getDisplayName(true) + "/" + fqname;
    parent = parent->getParent();
  }

  return fqname;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string GetRelativeName(const SGPropertyNode* node, const string &path)
{
  string temp_string = GetFullyQualifiedName(node);
  size_t len = path.length();
  if ( (len > 0) && (temp_string.substr(0,len) == path) ) {
    temp_string = temp_string.erase(0,len);
  }
  return temp_string;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Untie(const string &name)
{
  SGPropertyNode* property = root->getNode(name.c_str());
  if (!property) {
    cerr << "Attempt to untie a non-existant property." << name << endl;
    return;
  }

  Untie(property);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Untie(SGPropertyNode *property)
{
  const string& name = property->getNameString();

  assert(property->isTied());

  for (auto it = tied_properties.begin(); it != tied_properties.end(); ++it) {
    if (it->node.ptr() == property) {
      it->untie();
      tied_properties.erase(it);
      if (FGJSBBase::debug_lvl & 0x20) cout << "Untied " << name << endl;
      return;
    }
  }

  cerr << "Failed to untie property " << name << endl
       << "JSBSim is not the owner of this property." << endl;
}
} // namespace JSBSim
