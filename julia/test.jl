using Test

module JSBSim
  using CxxWrap
  @wrapmodule(joinpath(pwd(), "libJSBSimJL"))
end

path = JSBSim.SGPath("Hello world!")
@test JSBSim.str(path) == "Hello world!"
