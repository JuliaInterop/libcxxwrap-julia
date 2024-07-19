#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_list_iterator(TypeWrapper1& list_iterator)
{
  list_iterator.apply_combination<stl::ListIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_list(TypeWrapper1& list)
{
  list.apply_combination<std::list, stltypes>(stl::WrapList());
}

}

}