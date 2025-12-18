#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_priority_queue()
{
  WrapSTLContainer<std::priority_queue>().apply_combination<stltypes>();
}

}
