#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_valarray()
{
  WrapSTLContainer<std::valarray>().apply_combination<stltypes>();
}

}
