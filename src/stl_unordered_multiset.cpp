#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_unordered_multiset_iterator(TypeWrapper1& unordered_multiset_iterator)
{
  unordered_multiset_iterator.apply_combination<stl::UnorderedMultisetIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_unordered_multiset(TypeWrapper1& unordered_multiset)
{
  unordered_multiset.apply_combination<std::unordered_multiset, stltypes>(stl::WrapUnorderedMultiset());
}

}

}