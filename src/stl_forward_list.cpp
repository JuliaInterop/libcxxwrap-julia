#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_forward_list(TypeWrapper1& forward_list)
{
  forward_list.apply_combination<std::forward_list, stltypes>(stl::WrapForwardList());
}

}

}