#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_unordered_set(TypeWrapper1& unordered_set)
{
  unordered_set.apply_combination<std::unordered_set, stltypes>(stl::WrapSetType());
}

}

}