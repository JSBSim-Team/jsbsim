#include <jlcxx/jlcxx.hpp>

#include "FGFDMExec.h"
#include "initialization/FGInitialCondition.h"

using namespace JSBSim;

JLCXX_MODULE define_julia_module(jlcxx::Module& jsbsim)
{
  // SGPath
  jsbsim.add_type<SGPath>("SGPath")
    .constructor<const std::string&>()
    .method("str", &SGPath::str);

  // FGPropertyManager
  jsbsim.add_type<FGPropertyManager>("FGPropertyManager");

  // FGFDMExec
  auto FDMExec = jsbsim.add_type<FGFDMExec>("FGFDMExec");
  FDMExec.constructor<FGPropertyManager*>()
    .method("SetRootDir", &FGFDMExec::SetRootDir)
    .method("_GetRootDir", &FGFDMExec::GetRootDir)
    .method("SetAircraftPath", &FGFDMExec::SetAircraftPath)
    .method("SetEnginePath", &FGFDMExec::SetEnginePath)
    .method("SetSystemsPath", &FGFDMExec::SetSystemsPath)
    // Resolves ambiguity between the overloaded methods FGFDMExec::LoadModel
    // by static casting the pointer.
    .method("_LoadModel", static_cast<bool (FGFDMExec::*)(const std::string&, bool)>(&FGFDMExec::LoadModel))
    .method("RunIC", &FGFDMExec::RunIC)
    .method("Run", &FGFDMExec::Run)
    .method("GetPropertyValue", &FGFDMExec::GetPropertyValue);

  // FGInitialCondition
  jsbsim.add_type<FGInitialCondition>("FGInitialCondition")
    .constructor<FGFDMExec*>()
    .method("Load", &FGInitialCondition::Load);

  // FGFDMExec again (methods depending on other classes)
  FDMExec.method("GetIC", &FGFDMExec::GetIC);
}
