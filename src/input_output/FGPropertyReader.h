/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropertyReader.h
 Author:       Bertrand Coconnier
 Date started: 12/30/13

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

HISTORY
--------------------------------------------------------------------------------
12/30/13   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPERTYREADER_H
#define FGPROPERTYREADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <utility>
#include <list>
#include <map>

#include "simgear/props/props.hxx"
#include "input_output/FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPERTYREADER "$Id: FGPropertyReader.h,v 1.2 2014/05/29 18:46:44 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropertyReader
{
public:
  FGPropertyReader() {}; // Needed because the copy constructor is private
  ~FGPropertyReader();
  void Load(Element* el, FGPropertyManager* PropertyManager, bool override);
  bool ResetToIC(void);

  class const_iterator
  {
  public:
    const_iterator(void) {}
    const_iterator(const std::map<SGPropertyNode_ptr, double>::const_iterator &it) : prop_it(it) {}
    const_iterator& operator++() { ++prop_it; return *this; }
    bool operator!=(const const_iterator& it) const { return prop_it != it.prop_it; }
    FGPropertyNode* operator*() {
      SGPropertyNode* node = prop_it->first;
      return static_cast<FGPropertyNode*>(node);
    }

  private:
    std::map<SGPropertyNode_ptr, double>::const_iterator prop_it;
  };

  const_iterator begin(void) const { return const_iterator(interface_prop_initial_value.begin()); }
  const_iterator end(void) const { return const_iterator(interface_prop_initial_value.end()); }
  bool empty(void) const { return interface_prop_initial_value.empty(); }

private:
  std::map<SGPropertyNode_ptr, double> interface_prop_initial_value;
  std::list<std::pair<SGPropertyNode_ptr, double> > tied_interface_properties;

  /* Because FGPropertyReader stores internally the values its tied properties
   * are refering to, we need to prevent its instances to be copied. This is to
   * prevent the situation where an instance A would be copied to an instance B.
   * Which instance A or B would then keep track of the memory allocated to the
   * tied properties ? There is a number of strategies to handle that and the
   * one chosen here is the simplest: forbid FGPropertyReader instances from
   * being copied.
   * For that purpose the copy constructor and the copy assignment constructor
   * are declared private. Furthermore they are not implemented in order to
   * prevent FGPropertyReader members or friend functions to call them. Doing
   * otherwise would result in the compiler complaining that the code is trying
   * to call a function that is not implemented.
   * (see item 6 from Scott Meyers' 3rd edition of Effective C++)
   */
  FGPropertyReader(const FGPropertyReader&); // Not implemented on purpose (see the comments above)
  FGPropertyReader& operator=(const FGPropertyReader&); // Not implemented on purpose (see the comments above)
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
