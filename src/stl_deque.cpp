#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_deque(TypeWrapper1& deque)
{
  deque.apply_combination<std::deque, stltypes>(stl::WrapDeque());
}

}

}