#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_vector()
{
  WrapSTLContainer<std::vector>().apply_combination<stltypes>();
}

}
