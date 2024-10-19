#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_unordered_set()
{
  WrapSTLContainer<std::unordered_set>().apply_combination<stltypes>();
}

}

}