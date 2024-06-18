#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_list(TypeWrapper1& list)
{
  list.apply_combination<std::list, stltypes>(stl::WrapList());
}

}

}