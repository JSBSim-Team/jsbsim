/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGXMLParse.h
 Author:       Jon Berndt
 Date started: 08/20/2004
 Purpose:      Config file read-in class and XML parser
 Called by:    Various

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLParse.h"
#include <stdlib.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGXMLParse.cpp,v 1.2 2005/06/13 00:54:42 jberndt Exp $";
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
  if (document) delete document;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::startXML(void)
{
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
  int size, pos;
  string local_work_string;

  while (!working_string.empty()) {
     // clear leading newlines and spaces
    while (working_string[0] == '\n' || working_string[0] == ' ')
      working_string.erase(0,1);

    // remove spaces (only) from end of string
    size = working_string.size();
    while (working_string[size-1] == ' ')
    {
      working_string.erase(size-1,1);
      size = working_string.size();
    }

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
