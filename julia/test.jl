using Test

module JSBSim
  using CxxWrap
  @wrapmodule(joinpath(pwd(), "libJSBSimJL"))
  function SetRootDir(fdm::FGFDMExec, path::String)
    _SetRootDir(fdm, SGPath(path))
    SetAircraftPath(fdm, "aircraft")
    SetEnginePath(fdm, "engine")
    SetSystemsPath(fdm, "systems")
  end
  function GetRootDir(fdm::FGFDMExec)
    str(_GetRootDir(fdm))
  end
  function SetAircraftPath(fdm::FGFDMExec, path::String)
    _SetAircraftPath(fdm, SGPath(path))
  end
  function SetEnginePath(fdm::FGFDMExec, path::String)
    _SetEnginePath(fdm, SGPath(path))
  end
  function SetSystemsPath(fdm::FGFDMExec, path::String)
    _SetSystemsPath(fdm, SGPath(path))
  end
  function LoadModel(fdm::FGFDMExec, model::String, addModelToPath::Bool=true)
    _LoadModel(fdm, model, addModelToPath)
  end
  function Load(ic::FGInitialCondition, path::String, useStoredPath::Bool)
    _Load(ic, SGPath(path), useStoredPath)
  end
  function LoadIC(fdm::FGFDMExec, path::String, useStoredPath::Bool)
    ic= GetIC(fdm)
    Load(getindex(ic)[], path, useStoredPath)
  end
end

path = JSBSim.SGPath("Hello world!")
@test JSBSim.str(path) == "Hello world!"

root_dir = joinpath(@__DIR__, "..")

fdm = JSBSim.FGFDMExec()
JSBSim.SetRootDir(fdm, root_dir)

@test JSBSim.GetRootDir(fdm) == root_dir

@test JSBSim.LoadModel(fdm, "737") == true
@test JSBSim.LoadIC(fdm, "cruise_init.xml", true) == true
@test JSBSim.RunIC(fdm) == true

while JSBSim.GetPropertyValue(fdm, "simulation/sim-time-sec") < 5.0
  JSBSim.Run(fdm)
end
