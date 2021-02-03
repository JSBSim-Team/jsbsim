# Make sure CxxWrap is installed
import Pkg

try
  using CxxWrap
catch e
  Pkg.add("CxxWrap")
end
