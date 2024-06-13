#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_valarray(TypeWrapper1& valarray)
{
  valarray.apply_combination<std::valarray, stltypes>(stl::WrapValArray());
}

}

}