#include <jlcxx/jlcxx.hpp>

#include "simgear/misc/sg_path.hxx"

JLCXX_MODULE define_julia_module(jlcxx::Module& jsbsim)
{
  jsbsim.add_type<SGPath>("SGPath")
    .constructor<const std::string&>()
    .method("str", &SGPath::str);
}
