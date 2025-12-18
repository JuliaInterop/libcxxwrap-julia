#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_deque()
{
  WrapSTLContainer<std::deque>().apply_combination<stltypes>();
}

}
