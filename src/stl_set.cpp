#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_set()
{
  WrapSTLContainer<std::set>().apply_combination<stltypes>();
}

}

}