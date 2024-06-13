#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_set(TypeWrapper1& set)
{
  set.apply_combination<std::set, stltypes>(stl::WrapSetType());
}

}

}