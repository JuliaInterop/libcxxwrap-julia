#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_list()
{
  WrapSTLContainer<std::list>().apply_combination<stltypes>();
}

}

}