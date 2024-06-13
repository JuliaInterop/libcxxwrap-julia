#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_vector(TypeWrapper1& vector)
{
  vector.apply_combination<std::vector, stltypes>(stl::WrapVector());
}

}

}