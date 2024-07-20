#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_set_iterator(TypeWrapper1& set_iterator)
{
  set_iterator.apply_combination<stl::SetIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_set(TypeWrapper1& set)
{
  set.apply_combination<std::set, stltypes>(stl::WrapSet());
}

}

}