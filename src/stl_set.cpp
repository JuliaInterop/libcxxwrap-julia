#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_set()
{
  WrapSTLContainer<std::set>().apply_combination<stltypes>();
}

}
