#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_vector()
{
  WrapSTLContainer<std::vector>().apply_combination<stltypes>();
}

}

}