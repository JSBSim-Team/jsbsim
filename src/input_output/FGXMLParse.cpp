/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGXMLParse.cpp
 Author:       Jon Berndt
 Date started: 08/20/2004
 Purpose:      Config file read-in class and XML parser
 Called by:    Various

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGXMLParse.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void FGXMLParse::reset(void) {
  current_element = document = nullptr;
  working_string.erase();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::dumpDataLines(void) {

  if (!working_string.empty()) {
    //The following change was made to the XML parse
    // 1. This implemenation iterates over the input string in O(n) time.
    //    where n is the length of the input string.
    // 2. The built in JSBSim split() method runs in O(n^2) for n is the string size, with an additional
    //  O(m) [for m = n/rows of a matrix] from the outer for loop.
    //  Overall it runs in  O(n^3) time making parsing inviable for files around 100MB in size.
    //  Regression tested against 2 jsbsims with identical inputs, and validated no difference
    //    within epsilon = 1E-8.
    size_t index = 0;
    index = working_string.find('\n');
    if (index != std::string::npos && index != 0) {
      size_t curr = 0;
      std::string temp = "";
      while (curr < working_string.length()) {
        if (working_string[curr] == '\n') {
          trim(temp);
          current_element->AddData(temp);
          temp = "";
        } else {
          temp.push_back(working_string[curr]);
        }
        curr++;
      }
    } else {
      trim(working_string);
      current_element->AddData(working_string);
    }
    working_string.erase();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::startElement(const char *name, const XMLAttributes &atts) {
  if (!document) {
    document = new Element(name);
    current_element = document;
  } else {
    dumpDataLines();

    Element *temp_element = new Element(name);
    if (temp_element) {
      temp_element->SetParent(current_element);
      current_element->AddChildElement(temp_element);
    }
    current_element = temp_element;
  }

  if (!current_element) {
    cerr << "In file " << getPath() << ": line " << getLine() << endl
         << "No current element read (running out of memory?)" << endl;
    throw("Fatal error");
  }

  current_element->SetLineNumber(getLine());
  current_element->SetFileName(getPath());

  for (int i = 0; i < atts.size(); i++) {
    current_element->AddAttribute(atts.getName(i), atts.getValue(i));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::endElement(const char *name) {
  dumpDataLines();
  current_element = current_element->GetParent();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::data(const char *s, int length) {
  working_string += string(s, length);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGXMLParse::warning(const char *message, int line, int column) {
  cerr << "Warning: " << message << " line: " << line << " column: " << column
       << endl;
}

} // end namespace JSBSim
