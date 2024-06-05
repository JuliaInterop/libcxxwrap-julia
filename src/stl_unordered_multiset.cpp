#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_unordered_multiset(TypeWrapper1& unordered_multiset)
{
  unordered_multiset.apply_combination<std::unordered_multiset, stltypes>(stl::WrapMultisetType());
}

}

}