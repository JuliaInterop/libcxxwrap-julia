#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_forward_list()
{
  WrapSTLContainer<std::forward_list>().apply_combination<stltypes>();
}

}
