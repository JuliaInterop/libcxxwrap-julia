#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_stack()
{
  WrapSTLContainer<std::stack>().apply_combination<stltypes>();
}

}

}