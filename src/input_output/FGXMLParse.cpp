/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGXMLParse.cpp
 Author:       Jon Berndt
 Date started: 08/20/2004
 Purpose:      Config file read-in class and XML parser
 Called by:    Various

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLParse.h"
#include <cstdlib>

namespace JSBSim {

static const char *IdSrc = "$Id: FGXMLParse.cpp,v 1.7 2008/07/22 02:42:17 jberndt Exp $";
static const char *IdHdr = ID_XMLPARSE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLParse.h"

using namespace std;

FGXMLParse::FGXMLParse(void)
{
  first_element_read = false;
  current_element = document = 0L;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGXMLParse::~FGXMLParse(void)
{
  delete document;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::startXML(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::reset(void)
{
  delete document;
  first_element_read = false;
  current_element = document = 0L;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::endXML(void)
{
  // At this point, document should equal current_element ?
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::startElement (const char * name, const XMLAttributes &atts)
{
  string Name(name);
  Element *temp_element;

  working_string.erase();

  if (!first_element_read) {
    document = new Element(Name);
    current_element = document;
    first_element_read = true;
  } else {
    temp_element = new Element(Name);
    temp_element->SetParent(current_element);
    current_element->AddChildElement(temp_element);
    current_element = temp_element;
  }

  if (current_element == 0L) {
    cerr << "No current element read (no top-level element in XML file?)" << endl;
    exit (-1);
  }

  for (int i=0; i<atts.size();i++) {
    current_element->AddAttribute(atts.getName(i), atts.getValue(i));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::endElement (const char * name)
{
  string local_work_string;

  while (!working_string.empty()) {
    // clear leading newlines and spaces
    string::size_type pos = working_string.find_first_not_of( " \n");
    if (pos > 0)
      working_string.erase(0, pos);

    // remove spaces (only) from end of string
    pos = working_string.find_last_not_of( " ");
    if (pos != string::npos)
      working_string.erase( ++pos);

    if (!working_string.empty()) {
      pos = working_string.find("\n");
      if (pos != string::npos) local_work_string = working_string.substr(0,pos);
      else local_work_string = working_string;
      current_element->AddData(local_work_string);
      working_string.erase(0, pos);
    }
  }

  current_element = current_element->GetParent();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::data (const char * s, int length)
{
  working_string += string(s, length);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::pi (const char * target, const char * data)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::warning (const char * message, int line, int column)
{
  cerr << "Warning: " << message << " line: " << line << " column: " << column << endl;
}

} // end namespace JSBSim
