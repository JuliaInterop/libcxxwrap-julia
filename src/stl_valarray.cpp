#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_valarray()
{
  WrapSTLContainer<std::valarray>().apply_combination<stltypes>();
}

}

}