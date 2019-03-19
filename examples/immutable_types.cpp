#include <algorithm>

#include "jlcxx/jlcxx.hpp"

namespace immutable
{

struct ImmutableBits
{
  double a;
  double b;
};

struct A{
  float x, y;
};

float f(A a){ return a.x + a.y; }
float g(const A & a){ return a.x + a.y; }
//float h(const A * a){ return a ? a->x + a->y : 0.0; }

float twice_val(float x) { return 2.0*x; }
float twice_cref(const float& x) { return 2.0*x; }
float twice_ref(float& x) { return 2.0*x; }
void twice_ref_mut(float& x) { x *= 2.0; }
float twice_cptr(const float* x) { return 2.0*(*x); }
float twice_ptr(float* x) { return 2.0*(*x); }

}

extern "C"
{
  immutable::ImmutableBits make_immutable()
  {
    return {1.0,5.0};
  }

  void print_final(immutable::ImmutableBits b)
  {
    std::cout << "finalizing bits (" << b.a << "," << b.b << ")" << std::endl;
  }
}

namespace jlcxx
{
  template<> struct IsImmutable<immutable::ImmutableBits> : std::true_type {};
  template<> struct IsBits<immutable::ImmutableBits> : std::true_type {};

  template<> struct IsImmutable<immutable::A> : std::true_type {};
  template<> struct IsBits<immutable::A> : std::true_type {};
}

JLCXX_MODULE define_julia_module(jlcxx::Module& mod)
{
  using namespace immutable;
  //jlcxx::static_type_mapping<ImmutableBits>::set_julia_type((jl_datatype_t*)jlcxx::julia_type("ImmutableBits"));
  // mod.method("increment_immutable", [] (const ImmutableBits& x) { return ImmutableBits({x.a+1.0, x.b+1.0}); });

  //jlcxx::static_type_mapping<A>::set_julia_type((jl_datatype_t*)jlcxx::julia_type("A"));
  // mod.method("f", f);
  // mod.method("g", [] (const A& a) { return g(a); } );
  // //mod.method("h", h);

  // //julia_type<dereference_for_mapping<A&>>();
  // //jlcxx::mapped_julia_type<const A&> x = 10;

  mod.method("twice_val", twice_val);
  mod.method("twice_cref", twice_cref);
  mod.method("twice_ref", twice_ref);
  mod.method("twice_ref_mut", twice_ref_mut);
  // mod.method("twice_cptr", twice_cptr);
  // mod.method("twice_ptr", twice_ptr);

  // mod.method("arrtest", [] ()
  // {
  //   jlcxx::ArrayRef<double, 2> y(jlcxx::make_julia_array(new double[6], 3, 2));
  //   // write to y. for example:
  //   for(auto &z : y)
  //     z = 1.0;

  //   std::transform(y.begin(), y.end(), y.begin(), [] (double x) { return 2*x; });

  //   return y;
  // });
}
