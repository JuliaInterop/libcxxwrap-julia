#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_multiset(TypeWrapper1& multiset)
{
  multiset.apply_combination<std::multiset, stltypes>(stl::WrapMultisetType());
}

}

}