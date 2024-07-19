#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_deque_iterator(TypeWrapper1& deque_iterator)
{
  deque_iterator.apply_combination<stl::DequeIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_deque(TypeWrapper1& deque)
{
  deque.apply_combination<std::deque, stltypes>(stl::WrapDeque());
}

}

}