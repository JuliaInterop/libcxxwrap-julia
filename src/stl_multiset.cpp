#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_multiset()
{
  WrapSTLContainer<std::multiset>().apply_combination<stltypes>();
}

}
