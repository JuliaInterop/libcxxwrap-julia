#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_deque()
{
  WrapSTLContainer<std::deque>().apply_combination<stltypes>();
}

}

}