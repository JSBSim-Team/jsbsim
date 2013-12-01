/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGXMLFileRead.h
 Author:       Jon S. Berndt
 Date started: 02/04/07
 Purpose:      Shared base class that wraps the XML file reading logic

 ------------- Copyright (C) 2007  Jon S. Berndt (jon@jsbsim.org) -------------

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
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGXMLFILEREAD_HEADER_H
#define FGXMLFILEREAD_HEADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "input_output/FGXMLParse.h"
#include <iostream>
#include <fstream>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_XMLFILEREAD "$Id: FGXMLFileRead.h,v 1.9 2013/12/01 14:33:51 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGXMLFileRead {
public:
  FGXMLFileRead(void) {}
  ~FGXMLFileRead(void) {}

  Element* LoadXMLDocument(std::string XML_filename, bool verbose=true)
  {
    return LoadXMLDocument(XML_filename, file_parser, verbose);
  }

  Element* LoadXMLDocument(std::string XML_filename, FGXMLParse& fparse, bool verbose=true)
  {
    std::ifstream infile;

    if ( !XML_filename.empty() ) {
      if (XML_filename.find(".xml") == std::string::npos) XML_filename += ".xml";
      infile.open(XML_filename.c_str());
      if ( !infile.is_open()) {
        if (verbose) std::cerr << "Could not open file: " << XML_filename << std::endl;
        return 0L;
      }
    } else {
      std::cerr << "No filename given." << std::endl;
      return 0L;
    }

    readXML(infile, fparse, XML_filename);
    Element* document = fparse.GetDocument();
    infile.close();

    return document;
  }

  void ResetParser(void) {file_parser.reset();}

private:
  FGXMLParse file_parser;
};
}
#endif
