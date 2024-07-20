#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_multiset_iterator(TypeWrapper1& multiset_iterator)
{
  multiset_iterator.apply_combination<stl::MultisetIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_multiset(TypeWrapper1& multiset)
{
  multiset.apply_combination<std::multiset, stltypes>(stl::WrapMultiset());
}

}

}