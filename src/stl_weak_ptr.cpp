#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_weak_ptr()
{
  smartptr::apply_smart_combination<std::weak_ptr, stltypes>();
}

}

}