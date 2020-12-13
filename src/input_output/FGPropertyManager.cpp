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
  for(auto& prop: tied_properties)
    prop->untie();

  tied_properties.clear();
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

FGPropertyNode*
FGPropertyNode::GetNode (const string &path, bool create)
{
  SGPropertyNode* node = getNode(path.c_str(), create);
  if (node == 0) {
    cerr << "FGPropertyManager::GetNode() No node found for " << path << endl;
  }
  return (FGPropertyNode*)node;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyNode*
FGPropertyNode::GetNode (const string &relpath, int index, bool create)
{
  SGPropertyNode* node = getNode(relpath.c_str(), index, create);
  if (node == 0) {
    cerr << "FGPropertyManager::GetNode() No node found for " << relpath
         << "[" << index << "]" << endl;
  }
  return (FGPropertyNode*)node;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::HasNode (const string &path)
{
  const SGPropertyNode* node = getNode(path.c_str(), false);
  return (node != 0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyNode::GetName( void ) const
{
  return string( getName() );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyNode::GetPrintableName( void ) const
{
  string temp_string(getName());
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

string FGPropertyNode::GetFullyQualifiedName(void) const
{
    vector<string> stack;
    stack.push_back( getDisplayName(true) );
    const SGPropertyNode* tmpn=getParent();
    bool atroot=false;
    while( !atroot ) {
     stack.push_back( tmpn->getDisplayName(true) );
     if( !tmpn->getParent() )
      atroot=true;
     else
      tmpn=tmpn->getParent();
    }

    string fqname="";
    for(size_t i=stack.size()-1;i>0;i--) {
      fqname+= stack[i];
      fqname+= "/";
    }
    fqname+= stack[0];
    return fqname;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyNode::GetRelativeName( const string &path ) const
{
  string temp_string = GetFullyQualifiedName();
  size_t len = path.length();
  if ( (len > 0) && (temp_string.substr(0,len) == path) ) {
    temp_string = temp_string.erase(0,len);
  }
  return temp_string;
}



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::GetBool (const string &name, bool defaultValue) const
{
  return getBoolValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPropertyNode::GetInt (const string &name, int defaultValue ) const
{
  return getIntValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPropertyNode::GetLong (const string &name, long defaultValue ) const
{
  return getLongValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPropertyNode::GetFloat (const string &name, float defaultValue ) const
{
  return getFloatValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropertyNode::GetDouble (const string &name, double defaultValue ) const
{
  return getDoubleValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyNode::GetString (const string &name, string defaultValue ) const
{
  return string(getStringValue(name.c_str(), defaultValue.c_str()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetBool (const string &name, bool val)
{
  return setBoolValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetInt (const string &name, int val)
{
  return setIntValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetLong (const string &name, long val)
{
  return setLongValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetFloat (const string &name, float val)
{
  return setFloatValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetDouble (const string &name, double val)
{
  return setDoubleValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyNode::SetString (const string &name, const string &val)
{
  return setStringValue(name.c_str(), val.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyNode::SetArchivable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cerr <<
           "Attempt to set archive flag for non-existent property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::ARCHIVE, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyNode::SetReadable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cerr <<
           "Attempt to set read flag for non-existant property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::READ, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyNode::SetWritable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cerr <<
           "Attempt to set write flag for non-existant property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::WRITE, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Untie (const string &name)
{
  SGPropertyNode* property = root->getNode(name.c_str());
  if (!property) {
    cerr << "Attempt to untie a non-existant property." << name << endl;
    return;
  }

  vector <SGPropertyNode_ptr>::iterator it;
  for (it = tied_properties.begin(); it != tied_properties.end(); ++it) {
    if (*it == property) {
      property->untie();
      tied_properties.erase(it);
      if (FGJSBBase::debug_lvl & 0x20) cout << "Untied " << name << endl;
      return;
    }
  }

  cerr << "Failed to untie property " << name << endl
       << "JSBSim is not the owner of this property." << endl;
}
} // namespace JSBSim
