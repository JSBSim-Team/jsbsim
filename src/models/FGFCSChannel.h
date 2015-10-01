/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGFCSChannel.h
 Author:       Jon S. Berndt
 Date started: 10/11/12

 ------------- Copyright (C) 2012  Jon S. Berndt (jon@jsbsim.org) -------------

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
10/11/12   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFCSCHANNEL_H
#define FGFCSCHANNEL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCSCHANNEL "$Id: FGFCSChannel.h,v 1.6 2015/09/28 08:57:32 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

  /** Represents a <channel> in a control system definition.
      The <channel> may be defined within a <system>, <autopilot> or <flight_control>
      element. Channels are a way to group sets of components that perform
      a specific purpose or algorithm. */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

typedef std::vector <FGFCSComponent*> FCSCompVec;

class FGFCSChannel {
public:
  /// Constructor
  FGFCSChannel(std::string name, FGPropertyNode* node=0) :
  OnOffNode(node), Name(name)
  {
  }
  /// Destructor
  ~FGFCSChannel() {
    for (unsigned int i=0; i<FCSComponents.size(); i++) delete FCSComponents[i];
    FCSComponents.clear();
  }
  /// Retrieves the name of the channel
  std::string GetName() {return Name;}

  /// Adds a component to a channel
  void Add(FGFCSComponent* comp) {FCSComponents.push_back(comp);}
  /// Returns the number of components in the channel.
  size_t GetNumComponents() {return FCSComponents.size();}
  /// Retrieves a specific component.
  FGFCSComponent* GetComponent(unsigned int i) {
    if (i >= GetNumComponents()) {
      std::cerr << "Tried to get nonexistent component" << std::endl;
      return 0;
    } else {
      return FCSComponents[i];
    }
  }
  /// Reset the components that can be reset
  void Reset() {
    for (unsigned int i=0; i<FCSComponents.size(); i++)
      FCSComponents[i]->ResetPastStates();
  }
  /// Executes all the components in a channel.
  void Execute() {
    // If there is an on/off property supplied for this channel, check
    // the value. If it is true, permit execution to continue. If not, return
    // and do not execute the channel.
    if (OnOffNode != 0)
      if (!OnOffNode->getBoolValue()) return;

    for (unsigned int i=0; i<FCSComponents.size(); i++) FCSComponents[i]->Run();
  }

  private:
    FCSCompVec FCSComponents;
    FGConstPropertyNode_ptr OnOffNode;
    std::string Name;
};

}

#endif
