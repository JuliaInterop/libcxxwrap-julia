#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_unordered_set()
{
  WrapSTLContainer<std::unordered_set>().apply_combination<stltypes>();
}

}
