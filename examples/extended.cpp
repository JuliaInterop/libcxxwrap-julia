#include <string>

#include "jlcxx/jlcxx.hpp"

namespace extended
{

struct ExtendedWorld
{
  ExtendedWorld(const std::string& message = "default hello") : msg(message){}
  std::string greet() { return msg; }
  std::string msg;
};

} // namespace extended

JLCXX_MODULE define_julia_module(jlcxx::Module& types)
{
  using namespace extended;

  types.add_type<ExtendedWorld>("ExtendedWorld")
    .method("greet", &ExtendedWorld::greet);
}
