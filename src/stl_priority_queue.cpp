#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_priority_queue()
{
  WrapSTLContainer<std::priority_queue>().apply_combination<stltypes>();
}

}

}