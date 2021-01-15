# Make sure CxxWrap is installed
try
  using CxxWrap
catch e
  Pkg.add("CxxWrap")
end
