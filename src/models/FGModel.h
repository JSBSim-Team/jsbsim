/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGModel.h
 Author:       Jon Berndt
 Date started: 11/21/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
11/22/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMODEL_H
#define FGMODEL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <memory>

#include "math/FGModelFunctions.h"
#include "simgear/misc/sg_path.hxx"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;
class Element;
class FGPropertyManager;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for all scheduled JSBSim models

    Each model binds simulation/models/<Name>/enabled (default true), where Name
    is the name passed to the constructor. Setting that property false skips the
    model's Run(): its last state and its property bindings are preserved, which
    is the property-tree mechanism by which an external system can replace a
    model, for example a host physics engine owning FGGroundReactions. The
    binding is made at construction, so a model that renames itself while
    loading does not move its enable property.

    @author Jon S. Berndt
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGModel : public FGModelFunctions
{
public:

  /** Constructor.
      @param name the name of this model. It populates the member Name and binds
                  simulation/models/<name>/enabled. */
  FGModel(FGFDMExec*, const std::string& name);
  /// Destructor
  ~FGModel() override;

  /** Runs the model; called by the Executive.
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given. The Holding flag is not used in the base
                     FGModel class.
      @see JSBSim.cpp documentation
      @return false if no error */
  virtual bool Run(bool Holding);

  bool InitModel(void) override;
  /// Set the ouput rate for the model in frames
  void SetRate(unsigned int tt) {rate = tt;}
  /// Get the output rate for the model in frames
  unsigned int GetRate(void) const { return rate; }
  FGFDMExec* GetExec(void) const { return FDMExec; }

  void SetPropertyManager(std::shared_ptr<FGPropertyManager> fgpm) { PropertyManager=fgpm;}
  virtual SGPath FindFullPathName(const SGPath& path) const;
  const std::string& GetName(void) const { return Name; }
  virtual bool Load(Element* el) { return true; }

protected:
  unsigned int exe_ctr;
  unsigned int rate;
  bool enabled = true;
  std::string Name;

  /** Uploads this model in memory.
      Uploads the model in memory if its contents are contained in a separate
      file.
      @param el      a pointer to the element
      @param preLoad true if model functions and local properties must be
                     preloaded.
      @return true if model is successfully loaded*/
  bool Upload(Element* el, bool preLoad);

  virtual void Debug(int from);

  FGFDMExec*         FDMExec;
  std::shared_ptr<FGPropertyManager> PropertyManager;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
