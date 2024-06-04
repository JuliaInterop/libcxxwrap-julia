#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_queue(TypeWrapper1& queue)
{
  queue.apply_combination<std::queue, stltypes>(stl::WrapQueue());
}

}

}