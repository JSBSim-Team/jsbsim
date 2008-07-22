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

bool FGPropertyManager::suppress_warning = true;

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

FGPropertyManager*
FGPropertyManager::GetNode (const string &path, bool create)
{
  SGPropertyNode* node=this->getNode(path.c_str(), create);
  if (node == 0 && !suppress_warning) {
    cout << "FGPropertyManager::GetNode() No node found for " << path << endl;
  }
  return (FGPropertyManager*)node;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyManager*
FGPropertyManager::GetNode (const string &relpath, int index, bool create)
{
    return (FGPropertyManager*)getNode(relpath.c_str(),index,create);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


bool FGPropertyManager::HasNode (const string &path)
{
  // Checking if a node exists shouldn't write a warning if it doesn't exist
  suppress_warning = true;
  bool has_node = (GetNode(path, false) != 0);
  suppress_warning = false;
  return has_node;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyManager::GetName( void )
{
  return string( getName() );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyManager::GetPrintableName( void )
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

string FGPropertyManager::GetFullyQualifiedName(void) {
    vector<string> stack;
    stack.push_back( getDisplayName(true) );
    SGPropertyNode* tmpn=getParent();
    bool atroot=false;
    while( !atroot ) {
     stack.push_back( tmpn->getDisplayName(true) );
     if( !tmpn->getParent() )
      atroot=true;
     else
      tmpn=tmpn->getParent();
    }

    string fqname="";
    for(unsigned i=stack.size()-1;i>0;i--) {
      fqname+= stack[i];
      fqname+= "/";
    }
    fqname+= stack[0];
    return fqname;

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::GetBool (const string &name, bool defaultValue)
{
  return getBoolValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPropertyManager::GetInt (const string &name, int defaultValue )
{
  return getIntValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPropertyManager::GetLong (const string &name, long defaultValue )
{
  return getLongValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPropertyManager::GetFloat (const string &name, float defaultValue )
{
  return getFloatValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropertyManager::GetDouble (const string &name, double defaultValue )
{
  return getDoubleValue(name.c_str(), defaultValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropertyManager::GetString (const string &name, string defaultValue )
{
  return string(getStringValue(name.c_str(), defaultValue.c_str()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetBool (const string &name, bool val)
{
  return setBoolValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetInt (const string &name, int val)
{
  return setIntValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetLong (const string &name, long val)
{
  return setLongValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetFloat (const string &name, float val)
{
  return setFloatValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetDouble (const string &name, double val)
{
  return setDoubleValue(name.c_str(), val);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyManager::SetString (const string &name, const string &val)
{
  return setStringValue(name.c_str(), val.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::SetArchivable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cout <<
           "Attempt to set archive flag for non-existant property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::ARCHIVE, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::SetReadable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cout <<
           "Attempt to set read flag for non-existant property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::READ, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::SetWritable (const string &name, bool state )
{
  SGPropertyNode * node = getNode(name.c_str());
  if (node == 0)
    cout <<
           "Attempt to set write flag for non-existant property "
           << name << endl;
  else
    node->setAttribute(SGPropertyNode::WRITE, state);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Untie (const string &name)
{
  if (!untie(name.c_str()))
    cout << "Failed to untie property " << name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Tie (const string &name, bool *pointer, bool useDefault)
{
  if (!tie(name.c_str(), SGRawValuePointer<bool>(pointer), useDefault))
    cout << "Failed to tie property " << name << " to a pointer" << endl;
  else if (debug_lvl & 0x20)
    cout << name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Tie (const string &name, int *pointer,
                                          bool useDefault )
{
  if (!tie(name.c_str(), SGRawValuePointer<int>(pointer), useDefault))
    cout << "Failed to tie property " << name << " to a pointer" << endl;
  else if (debug_lvl & 0x20)
    cout << name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Tie (const string &name, long *pointer,
                                          bool useDefault )
{
  if (!tie(name.c_str(), SGRawValuePointer<long>(pointer), useDefault))
    cout << "Failed to tie property " << name << " to a pointer" << endl;
  else if (debug_lvl & 0x20)
    cout << name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Tie (const string &name, float *pointer,
                                          bool useDefault )
{
  if (!tie(name.c_str(), SGRawValuePointer<float>(pointer), useDefault))
    cout << "Failed to tie property " << name << " to a pointer" << endl;
  else if (debug_lvl & 0x20)
    cout << name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyManager::Tie (const string &name, double *pointer, bool useDefault)
{
  if (!tie(name.c_str(), SGRawValuePointer<double>(pointer), useDefault))
    cout << "Failed to tie property " << name << " to a pointer" << endl;
  else if (debug_lvl & 0x20)
    cout << name << endl;
}

} // namespace JSBSim
