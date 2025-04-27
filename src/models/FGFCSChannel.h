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
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

  /** Represents a <channel> in a control system definition.
      The <channel> may be defined within a <system>, <autopilot> or <flight_control>
      element. Channels are a way to group sets of components that perform
      a specific purpose or algorithm.
      Created within a <system> tag, the channel is defined as follows
      <channel name="name" [execute="property"] [execrate="rate"]>
      name is the name of the channel - in the old way this would also be used to bind elements
      execute [optional] is the property that defines when to execute this channel; an on/off switch
      execrate [optional] is the rate at which the channel should execute.
               A value of 0 or 1 will execute the channel every frame, a value of 2
               every other frame (half rate), a value of 4 is every 4th frame (quarter rate)
      */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

typedef std::vector <FGFCSComponent*> FCSCompVec;

class FGFCSChannel {
public:
  /// Constructor
  FGFCSChannel(FGFCS* FCS, const std::string &name, int execRate,
               SGPropertyNode* node=nullptr)
    : fcs(FCS), OnOffNode(node), Name(name)
  {
    ExecRate = execRate < 1 ? 1 : execRate;
    // Set ExecFrameCountSinceLastRun so that each components are initialized
    ExecFrameCountSinceLastRun = ExecRate;
  }

  /// Destructor
  ~FGFCSChannel() {
    for (unsigned int i=0; i<FCSComponents.size(); i++) delete FCSComponents[i];
    FCSComponents.clear();
  }
  /// Retrieves the name of the channel
  std::string GetName() {return Name;}

  /// Adds a component to a channel
  void Add(FGFCSComponent* comp) {
    FCSComponents.push_back(comp);
  }
  /// Returns the number of components in the channel.
  size_t GetNumComponents() {return FCSComponents.size();}
  /// Retrieves a specific component.
  FGFCSComponent* GetComponent(unsigned int i) {
    if (i < GetNumComponents()) {
      return FCSComponents[i];
    } else {
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::ERROR);
      log << "Tried to get nonexistent component\n";
      return nullptr;
    }
  }
  /// Reset the components that can be reset
  void Reset() {
    for (unsigned int i=0; i<FCSComponents.size(); i++)
      FCSComponents[i]->ResetPastStates();

    // Set ExecFrameCountSinceLastRun so that each components are initialized
    // after a reset.
    ExecFrameCountSinceLastRun = ExecRate;
  }
  /// Executes all the components in a channel.
  void Execute() {
    // If there is an on/off property supplied for this channel, check
    // the value. If it is true, permit execution to continue. If not, return
    // and do not execute the channel.
    if (OnOffNode && !OnOffNode->getBoolValue()) return;

    if (fcs->GetDt() != 0.0) {
      if (ExecFrameCountSinceLastRun >= ExecRate) {
        ExecFrameCountSinceLastRun = 0;
      }

      ++ExecFrameCountSinceLastRun;
    }

    // channel will be run at rate 1 if trimming, or when the next execrate
    // frame is reached
    if (fcs->GetTrimStatus() || ExecFrameCountSinceLastRun >= ExecRate) {
      for (unsigned int i=0; i<FCSComponents.size(); i++)
        FCSComponents[i]->Run();
    }
  }
  /// Get the channel rate
  int GetRate(void) const { return ExecRate; }

  private:
    FGFCS* fcs;
    FCSCompVec FCSComponents;
    SGConstPropertyNode_ptr OnOffNode;
    std::string Name;

    int ExecRate;        // rate at which this system executes, 0 or 1 every frame, 2 every second frame etc..
    int ExecFrameCountSinceLastRun;
};

}

#endif
