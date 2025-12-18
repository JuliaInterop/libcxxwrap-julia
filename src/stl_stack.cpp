#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_stack()
{
  WrapSTLContainer<std::stack>().apply_combination<stltypes>();
}

}
