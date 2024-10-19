#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_multiset()
{
  WrapSTLContainer<std::multiset>().apply_combination<stltypes>();
}

}

}