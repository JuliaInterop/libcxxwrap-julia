#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_unordered_multiset()
{
  WrapSTLContainer<std::unordered_multiset>().apply_combination<stltypes>();
}

}
