#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_unique_ptr()
{
  smartptr::apply_smart_combination<std::unique_ptr, stltypes>();
}

}
