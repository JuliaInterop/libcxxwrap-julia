#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_forward_list_iterator(TypeWrapper1& forward_list_iterator)
{
  forward_list_iterator.apply_combination<stl::ForwardListIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_forward_list(TypeWrapper1& forward_list)
{
  forward_list.apply_combination<std::forward_list, stltypes>(stl::WrapForwardList());
}

}

}