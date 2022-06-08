/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 File:         FGXMLElement.h
 Author:       Jon S. Berndt
 Date started: 9/28/04

 ------------- Copyright (C) 2004  Jon S. Berndt (jon@jsbsim.org) -------------

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

#ifndef XMLELEMENT_H
#define XMLELEMENT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <map>
#include <vector>

#include "simgear/structure/SGSharedPtr.hxx"
#include "math/FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an XML element.
    This class handles the creation, storage, and manipulation of XML elements.
    This class also can convert supplied values as follows:

    convert ["from"]["to"] = factor, so: from * factor = to
    - convert["M"]["FT"] = 3.2808399;
    - convert["FT"]["M"] = 1.0/convert["M"]["FT"];
    - convert["M2"]["FT2"] = convert["M"]["FT"]*convert["M"]["FT"];
    - convert["FT2"]["M2"] = 1.0/convert["M2"]["FT2"];
    - convert["FT"]["IN"] = 12.0;
    - convert["IN"]["FT"] = 1.0/convert["FT"]["IN"];
    - convert["LBS"]["KG"] = 0.45359237;
    - convert["KG"]["LBS"] = 1.0/convert["LBS"]["KG"];
    - convert["SLUG*FT2"]["KG*M2"] = 1.35594;
    - convert["KG*M2"]["SLUG*FT2"] = 1.0/convert["SLUG*FT2"]["KG*M2"];
    - convert["RAD"]["DEG"] = 360.0/(2.0*3.1415926);
    - convert["DEG"]["RAD"] = 1.0/convert["RAD"]["DEG"];
    - convert["LBS/FT"]["N/M"] = 14.5939;
    - convert["LBS/FT/SEC"]["N/M/SEC"] = 14.5939;
    - convert["N/M"]["LBS/FT"] = 1.0/convert["LBS/FT"]["N/M"];
    - convert["N/M/SEC"]["LBS/FT/SEC"] = 1.0/convert["LBS/FT/SEC"]["N/M/SEC"];
    - convert["WATTS"]["HP"] = 0.001341022;
    - convert["HP"]["WATTS"] = 1.0/convert["WATTS"]["HP"];
    - convert["N"]["LBS"] = 0.22482;
    - convert["LBS"]["N"] = 1.0/convert["N"]["LBS"];
    - convert["KTS"]["FT/SEC"] = ktstofps;
    - convert["KG/MIN"]["LBS/MIN"] = convert["KG"]["LBS"];
    - convert["LBS/HP*HR"]["KG/KW*HR"] = 0.6083;
    - convert["KG/KW*HR"]["LBS/HP*HR"] = 1/convert["LBS/HP*HR"]["KG/KW*HR"];
    - convert["KG/L"]["LBS/GAL"] = 8.3454045;

    - convert["M"]["M"] = 1.00;
    - convert["FT"]["FT"] = 1.00;
    - convert["IN"]["IN"] = 1.00;
    - convert["DEG"]["DEG"] = 1.00;
    - convert["RAD"]["RAD"] = 1.00;
    - convert["M2"]["M2"] = 1.00;
    - convert["FT2"]["FT2"] = 1.00;
    - convert["KG*M2"]["KG*M2"] = 1.00;
    - convert["SLUG*FT2"]["SLUG*FT2"] = 1.00;
    - convert["KG"]["KG"] = 1.00;
    - convert["LBS"]["LBS"] = 1.00;
    - convert["LBS/FT"]["LBS/FT"] = 1.00;
    - convert["N/M"]["N/M"] = 1.00;
    - convert["LBS/FT/SEC"]["LBS/FT/SEC"] = 1.00;
    - convert["N/M/SEC"]["N/M/SEC"] = 1.00;
    - convert["PSI"]["PSI"] = 1.00;
    - convert["INHG"]["INHG"] = 1.00;
    - convert["HP"]["HP"] = 1.00;
    - convert["N"]["N"] = 1.00;
    - convert["WATTS"]["WATTS"] = 1.00;
    - convert["KTS"]["KTS"] = 1.0;
    - convert["FT/SEC"]["FT/SEC"] = 1.0;
    - convert["KG/MIN"]["KG/MIN"] = 1.0;
    - convert["LBS/MIN"]["LBS/MIN"] = 1.0;
    - convert["LBS/HP*HR"]["LBS/HP*HR"] = 1.0;
    - convert["KG/KW*HR"]["KG/KW*HR"] = 1.0;
    - convert["KG/L"]["KG/L"] = 1.0;
    - convert["LBS/GAL"]["LBS/GAL"] = 1.0;

    Where:
    - N = newtons
    - M = meters
    - M2 = meters squared
    - KG = kilograms
    - LBS = pounds force
    - FT = feet
    - FT2 = feet squared
    - SEC = seconds
    - MIN = minutes
    - SLUG = slug
    - DEG = degrees
    - RAD = radians
    - WATTS = watts
    - HP = horsepower
    - HR = hour
    - L = liter
    - GAL = gallon (U.S. liquid) 

    @author Jon S. Berndt
*/

class Element;
typedef SGSharedPtr<Element> Element_ptr;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API Element : public SGReferenced {
public:
  /** Constructor
      @param nm the name of this element (if given)
      */
  Element(const std::string& nm);
  /// Destructor
  ~Element(void);

  /** Determines if an element has the supplied attribute.
      @param key specifies the attribute key to retrieve the value of.
      @return true or false. */
  bool HasAttribute(const std::string& key) {return attributes.find(key) != attributes.end();}

  /** Retrieves an attribute.
      @param key specifies the attribute key to retrieve the value of.
      @return the key value (as a string), or the empty string if no such
              attribute exists. */
  std::string GetAttributeValue(const std::string& key);

  /** Modifies an attribute.
      @param key specifies the attribute key to modify the value of.
      @param value new key value (as a string).
      @return false if it did not find any attribute with the requested key,
              true otherwise.
   */
  bool SetAttributeValue(const std::string& key, const std::string& value);

  /** Retrieves an attribute value as a double precision real number.
      @param key specifies the attribute key to retrieve the value of.
      @return the key value (as a number), or the HUGE_VAL if no such
              attribute exists. */
  double GetAttributeValueAsNumber(const std::string& key);

  /** Retrieves the element name.
      @return the element name, or the empty string if no name has been set.*/
  const std::string& GetName(void) const {return name;}
  void ChangeName(const std::string& _name) { name = _name; }

  /** Gets a line of data belonging to an element.
      @param i the index of the data line to return (0 by default).
      @return a string representing the data line requested, or the empty string
              if none exists.*/
  std::string GetDataLine(unsigned int i=0);

  /// Returns the number of lines of data stored
  unsigned int GetNumDataLines(void) {return (unsigned int)data_lines.size();}

  /// Returns the number of child elements for this element.
  unsigned int GetNumElements(void) {return (unsigned int)children.size();}

  /// Returns the number of named child elements for this element.
  unsigned int GetNumElements(const std::string& element_name);

  /** Converts the element data to a number.
      This function attempts to convert the first (and presumably only) line of
      data "owned" by the element into a real number. If there is not exactly one
      line of data owned by the element, then HUGE_VAL is returned.
      @return the numeric value of the data owned by the element.*/
  double GetDataAsNumber(void);

  /** Returns a pointer to the element requested by index.
      This function also resets an internal counter to the index, so that
      subsequent calls to GetNextElement() will return the following
      elements sequentially, until the last element is reached. At that point,
      GetNextElement() will return NULL.
      @param el the index of the requested element (0 by default)
      @return a pointer to the Element, or 0 if no valid element exists. */
  Element* GetElement(unsigned int el=0);

  /** Returns a pointer to the next element in the list.
      The function GetElement() must be called first to be sure that this
      function will return the correct element. The call to GetElement() resets
      the internal counter to zero. Subsequent calls to GetNextElement() return
      a pointer to subsequent elements in the list. When the final element is
      reached, 0 is returned.
      @return a pointer to the next Element in the sequence, or 0 if no valid
              Element is present. */
  Element* GetNextElement(void);

  /** Returns a pointer to the parent of an element.
      @return a pointer to the parent Element, or 0 if this is the top level Element. */
  Element* GetParent(void) {return parent;}

  /** Returns the line number at which the element has been defined.
      @return the line number
   */
  int GetLineNumber(void) const { return line_number; }

  /** Returns the name of the file in which the element has been read.
      @return the file name
  */
  const std::string& GetFileName(void) const { return file_name; }

  /** Searches for a specified element.
      Finds the first element that matches the supplied string, or simply the first
      element if no search string is supplied. This function call resets the internal
      element counter to the first element.
      @param el the search string (empty string by default).
      @return a pointer to the first element that matches the supplied search string. */
  Element* FindElement(const std::string& el="");

  /** Searches for the next element as specified.
      This function would be called after FindElement() is first called (in order to
      reset the internal counter). If no argument is supplied (or the empty string)
      a pointer to the very next element is returned. Otherwise, the next occurence
      of the named element is returned. If the end of the list is reached, 0 is
      returned.
      @param el the name of the next element to find.
      @return the pointer to the found element, or 0 if no appropriate element us
              found.*/
  Element* FindNextElement(const std::string& el="");

  /** Searches for the named element and returns the string data belonging to it.
      This function allows the data belonging to a named element to be returned
      as a string. If no element is found, the empty string is returned. If no
      argument is supplied, the data string for the first element is returned.
      @param el the name of the element being searched for (the empty string by
      default)
      @return the data value for the named element as a string, or the empty
              string if the element cannot be found. */
  std::string FindElementValue(const std::string& el="");

  /** Searches for the named element and returns the data belonging to it as a number.
      This function allows the data belonging to a named element to be returned
      as a double. If no element is found, HUGE_VAL is returned. If no
      argument is supplied, the data for the first element is returned.
      @param el the name of the element being searched for (the empty string by
      default)
      @return the data value for the named element as a double, or HUGE_VAL if the
              data is missing. */
  double FindElementValueAsNumber(const std::string& el="");

  /** Searches for the named element and returns the data belonging to it as a bool.
      This function allows the data belonging to a named element to be returned
      as a bool. If no element is found, false is returned. If no
      argument is supplied, the data for the first element is returned.
      @param el the name of the element being searched for (the empty string by
      default)
      @return the data value for the named element as a bool, or false if the
              data is missing. Zero will be false, while any other number will be true. */
  bool FindElementValueAsBoolean(const std::string& el="");
  
  /** Searches for the named element and converts and returns the data belonging to it.
      This function allows the data belonging to a named element to be returned
      as a double. If no element is found, HUGE_VAL is returned. If no
      argument is supplied, the data for the first element is returned. Additionally,
      this function converts the value from the units specified in the config file (via
      the UNITS="" attribute in the element definition) to the native units used by
      JSBSim itself, as specified by the target_units parameter. The currently
      allowable unit conversions are seen in the source file FGXMLElement.cpp.
      Also, see above in the main documentation for this class.
      @param el the name of the element being searched for (the empty string by
      default)
      @param target_units the string representing the native units used by JSBSim
             to which the value returned will be converted.
      @return the unit-converted data value for the named element as a double,
              or HUGE_VAL if the data is missing. */
  double FindElementValueAsNumberConvertTo(const std::string& el, const std::string& target_units);

  /** Searches for the named element and converts and returns the data belonging to it.
      This function allows the data belonging to a named element to be returned
      as a double. If no element is found, HUGE_VAL is returned. If no
      argument is supplied, the data for the first element is returned. Additionally,
      this function converts the value from the units specified in the supplied_units
      parameter to the units specified in the target_units parameter. JSBSim itself,
      as specified by the target_units parameter. The currently allowable unit
      conversions are seen in the source file FGXMLElement.cpp. Also, see above
      in the main documentation for this class.
      @param el the name of the element being searched for (the empty string by
      default)
      @param supplied_units the string representing the units of the value as
             supplied by the config file.
      @param target_units the string representing the native units used by JSBSim
             to which the value returned will be converted.
      @return the unit-converted data value for the named element as a double,
              or HUGE_VAL if the data is missing. */
  double FindElementValueAsNumberConvertFromTo( const std::string& el,
                                                const std::string& supplied_units,
                                                const std::string& target_units);

  /** Composes a 3-element column vector for the supplied location or orientation.
      This function processes a LOCATION or ORIENTATION construct, returning a
      filled-out 3-element column vector containing the X, Y, Z or ROLL, PITCH,
      YAW elements found in the supplied element. If one of the mentioned components
      is not found, that component is set to zero and a warning message is printed.
      All three elements should be supplied.
      @param target_units the string representing the native units used by JSBSim
             to which the value returned will be converted.
      @return a column vector object built from the LOCATION or ORIENT components. */
  FGColumnVector3 FindElementTripletConvertTo( const std::string& target_units);

  double DisperseValue(Element *e, double val, const std::string& supplied_units="",
                       const std::string& target_units="");

  /** This function sets the value of the parent class attribute to the supplied
      Element pointer.
      @param p pointer to the parent Element. */
  void SetParent(Element* p) {parent = p;}

  /** Adds a child element to the list of children stored for this element.
  *   @param el Child element to add. */
  void AddChildElement(Element* el) {children.push_back(el);}

  /** Stores an attribute belonging to this element.
  *   @param name The string name of the attribute.
  *   @param value The string value of the attribute. */
  void AddAttribute(const std::string& name, const std::string& value);

  /** Stores data belonging to this element.
  *   @param d the data to store. */
  void AddData(std::string d);

  /** Prints the element.
  *   Prints this element and calls the Print routine for child elements.
  *   @param d The tab level. A level corresponds to a single space. */
  void Print(unsigned int level=0);

  /** Set the line number at which the element has been read.
   *  @param line line number.
   */
  void SetLineNumber(int line) { line_number = line; }

  /** Set the name of the file in which the element has been read.
   *  @param name file name
   */
  void SetFileName(const std::string& name) { file_name = name; }

  /** Return a string that contains a description of the location where the
   *  current XML element was read from.
   *  @return a string describing the file name and line number where the
   *          element was read.
   */
  std::string ReadFrom(void) const;

  /** Merges the attributes of the current element with another element. The
   *  attributes from the current element override the element that is passed
   *  as a parameter. In other words if the two elements have an attribute with
   *  the same name, the attribute from the current element is kept and the
   *  corresponding attribute of the other element is ignored.
   *  @param el element with which the current element will merge its attributes.
   */
  void MergeAttributes(Element* el);

private:
  std::string name;
  std::map <std::string, std::string> attributes;
  std::vector <std::string> data_lines;
  std::vector <Element_ptr> children;
  Element *parent;
  unsigned int element_index;
  std::string file_name;
  int line_number;
  typedef std::map <std::string, std::map <std::string, double> > tMapConvert;
  static tMapConvert convert;
  static bool converterIsInitialized;
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
