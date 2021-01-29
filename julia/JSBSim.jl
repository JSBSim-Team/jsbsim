module JSBSim
  using CxxWrap
  @wrapmodule(joinpath(pwd(), "libJSBSimJL"))
  function __init__()
    @initcxx
  end

  function convert(::Type{String}, path::SGPath)
    s = str(path)
    if Sys.iswindows()
      s = replace(s, "/" => "\\")
    end
    return s
  end
  function SetRootDir(fdm::FGFDMExec, path::String)
    SetRootDir(fdm, SGPath(path))
    SetAircraftPath(fdm, "aircraft")
    SetEnginePath(fdm, "engine")
    SetSystemsPath(fdm, "systems")
  end
  function GetRootDir(fdm::FGFDMExec)::String
    convert(String, _GetRootDir(fdm)[])
  end
  function SetAircraftPath(fdm::FGFDMExec, path::String)
    SetAircraftPath(fdm, SGPath(path))
  end
  function SetEnginePath(fdm::FGFDMExec, path::String)
    SetEnginePath(fdm, SGPath(path))
  end
  function SetSystemsPath(fdm::FGFDMExec, path::String)
    SetSystemsPath(fdm, SGPath(path))
  end
  function LoadModel(fdm::FGFDMExec, model::String, addModelToPath::Bool=true)
    _LoadModel(fdm, model, addModelToPath)
  end
  function Load(ic::FGInitialCondition, path::String, useStoredPath::Bool)
    Load(ic, SGPath(path), useStoredPath)
  end
  function LoadIC(fdm::FGFDMExec, path::String, useStoredPath::Bool)
    ic = GetIC(fdm)
    Load(getindex(ic)[], path, useStoredPath)
  end
end
