#include "jlcxx/stl.hpp"

namespace jlcxx::stl
{

void apply_shared_ptr()
{
  smartptr::apply_smart_combination<std::shared_ptr, stltypes>();
}

}
