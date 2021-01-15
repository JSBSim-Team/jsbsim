using Test

module JSBSim
  using CxxWrap
  @wrapmodule(joinpath(pwd(), "libJSBSimJL.so"))
end

path = JSBSim.SGPath("Hello world!")
@test JSBSim.str(path) == "Hello world!"
