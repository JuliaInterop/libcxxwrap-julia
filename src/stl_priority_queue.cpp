#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

void apply_priority_queue(TypeWrapper1& priority_queue)
{
  priority_queue.apply_combination<std::priority_queue, stltypes>(stl::WrapPriorityQueue());
}

}

}