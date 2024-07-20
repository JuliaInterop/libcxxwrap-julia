#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_unordered_set_iterator(TypeWrapper1& unordered_set_iterator)
{
  unordered_set_iterator.apply_combination<stl::UnorderedSetIteratorWrapper, stltypes>(stl::WrapIterator());
}

void apply_unordered_set(TypeWrapper1& unordered_set)
{
  unordered_set.apply_combination<std::unordered_set, stltypes>(stl::WrapUnorderedSet());
}

}

}