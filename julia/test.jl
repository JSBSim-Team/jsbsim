using Test

module JSBSim
  using CxxWrap
  if Sys.islinux()
    @wrapmodule(joinpath(pwd(), "libJSBSimJL.so"))
  else
    @wrapmodule(joinpath(pwd(), "libJSBSimJL"))
  end
end

path = JSBSim.SGPath("Hello world!")
@test JSBSim.str(path) == "Hello world!"
