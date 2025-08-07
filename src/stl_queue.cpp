#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_queue()
{
  WrapSTLContainer<std::queue>().apply_combination<stltypes>();
}

}

}