#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_stack(TypeWrapper1& stack)
{
  stack.apply_combination<std::stack, stltypes>(stl::WrapStack());
}

}

}