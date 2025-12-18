#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_queue()
{
  WrapSTLContainer<std::queue>().apply_combination<stltypes>();
}

}
