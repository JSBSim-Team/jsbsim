/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGXMLParse.h
 Author:       Jon S. Berndt
 Date started: 8/20/04

 ------------- Copyright (C) 2004  Jon S. Berndt (jsb@hal-pc.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGXMLPARSE_H
#define FGXMLPARSE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
#  include STL_MAP
#else
#  include <string>
#  include <map>
#  include <iostream>
   using std::string;
   using std::map;
   using std::cout;
   using std::cerr;
   using std::endl;
#endif

#include "simgear/xml/easyxml.hxx"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_XMLPARSE "$Id: FGXMLParse.h,v 1.2 2004/09/28 11:38:59 jberndt Exp $"
#define VALID_CHARS """`!@#$%^&*()_+`1234567890-={}[];':,.<>/?abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element {
public:
  Element(string nm) {name = nm; parent=0L; element_index=0;}
  ~Element(void) {
    for (int i=0; i<children.size(); i++) delete children[i];
    data_lines.clear();
    attributes.clear();
    attribute_key.clear();
  }

  string GetAttributeValue(string key) {return attributes[key];}
  string GetName(void) {return name;}

  string GetDataLine(int i) {return data_lines[i];}
  Element* GetElement(int el=0) {
    if (children.size() > el) {
      element_index = el;
      return children[el];
    }
    else {
      element_index = 0;
      return 0L;
    }
  }
  Element* GetNextElement(void) {
    if (children.size() > element_index+1) {
      element_index++;
      return children[element_index];
    } else {
      element_index = 0;
      return 0L;
    }
  }
  Element* FindElement(string el="") {
    if (el.empty() && children.size() >= 1) {
      element_index = 1;
      return children[0];
    }
    for (int i=0; i<children.size(); i++) {
      if (el == children[i]->GetName()) {
        element_index = i;
        return children[i];
      }
    }
    element_index = 0;
    return 0L;
  }
  Element* FindNextElement(string el="") {
    if (el.empty()) {
      if (element_index < children.size()) {
        return children[element_index++];
      } else {
        element_index = 0;
        return 0L;
      }
    }
    for (int i=element_index; i<children.size(); i++) {
      if (el == children[i]->GetName()) {
        element_index = i+1;
        return children[i];
      }
    }
    element_index = 0;
    return 0L;
  }
  Element* GetParent(void) {return parent;}

  void SetParent(Element* p) {parent = p;}
  void AddChildElement(Element* el) {children.push_back(el);}
  void AddAttribute(string name, string value) {
    attribute_key.push_back(name);
    attributes[name] = value;
  }
  void AddData(string d) {
    int string_start = d.find_first_not_of(" ");
    if (string_start > 0) d.erase(0,string_start-1);
    data_lines.push_back(d);
  }

  void Print(int level=0) {
    level+=2;
    for (int spaces=0; spaces<=level; spaces++) cout << " "; // format output
    cout << "Element Name: " << name;
    for (int i=0; i<attributes.size(); i++) {
      cout << "  " << attribute_key[i] << " = " << attributes[attribute_key[i]];
    }
    cout << endl;
    for (int i=0; i<data_lines.size(); i++) {
      for (int spaces=0; spaces<=level; spaces++) cout << " "; // format output
      cout << data_lines[i] << endl;
    }
    for (int i=0; i<children.size(); i++) {
      children[i]->Print(level);
    }
  }

private:
  string name;
  map <string, string> attributes;
  vector <string> data_lines;
  vector <Element*> children;
  vector <string> attribute_key;
  Element *parent;
  int element_index;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an XML parser based on the EasyXML parser from the SimGear library.
    @author Jon S. Berndt
    @version $Id: FGXMLParse.h,v 1.2 2004/09/28 11:38:59 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGXMLParse : public XMLVisitor
{
public:

  FGXMLParse(void);
  virtual ~FGXMLParse(void);

  Element* GetDocument(void) {return document;}

  virtual void startXML();
  virtual void endXML();
  virtual void startElement (const char * name, const XMLAttributes &atts);
  virtual void endElement (const char * name);
  virtual void data (const char * s, int length);
  virtual void pi (const char * target, const char * data);
  virtual void warning (const char * message, int line, int column);

private:
  bool first_element_read;
  string data_string;
  Element *document;
  Element *current_element;
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
