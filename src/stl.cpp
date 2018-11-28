#include <vector>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

struct WrapVector
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    using size_type = typename WrappedT::size_type;
    wrapped.method("push_back", static_cast<void (WrappedT::*)(const T&) >(&WrappedT::push_back));
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("getindex", [] (const WrappedT& v, int_t i) { return v[i-1]; });
    wrapped.method("setindex!", [] (WrappedT& v, const T& val, int_t i) { v[i-1] = val; });
    wrapped.method("resize", [] (WrappedT& v, const int_t s) { v.resize(s); });
    wrapped.method("append", [] (WrappedT& v, jlcxx::ArrayRef<T> arr)
    {
      v.reserve(v.size() + arr.size());
      for(const T& x : arr)
      {
        v.push_back(x);
      }
    });
  }
};

}

JLCXX_MODULE define_julia_module(jlcxx::Module& stl)
{
  stl.add_type<Parametric<TypeVar<1>>>("StdVector", julia_type("AbstractVector"))
    .apply<std::vector<int>>(stl::WrapVector());
}

}

