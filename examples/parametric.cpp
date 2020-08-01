#include <type_traits>

#include "jlcxx/jlcxx.hpp"

namespace parametric
{

struct P1
{
  typedef int val_type;
  static val_type value()
  {
    return 1;
  }
};

struct P2
{
  typedef double val_type;
  static val_type value()
  {
    return 10.;
  }
};

template<typename A, typename B>
struct TemplateType
{
  typedef typename A::val_type first_val_type;
  typedef typename B::val_type second_val_type;

  first_val_type get_first()
  {
    return A::value();
  }

  second_val_type get_second()
  {
    return B::value();
  }
};

// Template containing a non-type parameter
template<typename T, T I>
struct NonTypeParam
{
  typedef T type;
  NonTypeParam(T v = I) : i(v)
  {
  }

  T i = I;
};

template<typename A, typename B=void>
struct TemplateDefaultType
{
};

template<typename T>
struct AbstractTemplate
{
  virtual void foo() const = 0;
  virtual ~AbstractTemplate(){};
};

template<typename T>
struct ConcreteTemplate : public AbstractTemplate<T>
{
  void foo() const {}
};

// Helper to wrap TemplateType instances. May also be a C++14 lambda, see README.md
struct WrapTemplateType
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.method("get_first", &WrappedT::get_first);
    wrapped.method("get_second", &WrappedT::get_second);
  }
};

struct WrapTemplateDefaultType
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&&)
  {
  }
};

// Helper to wrap NonTypeParam instances
struct WrapNonTypeParam
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.template constructor<typename WrappedT::type>();
    // Access the module to add a free function
    wrapped.module().method("get_nontype", [](const WrappedT& w) { return w.i; });
  }
};

struct WrapAbstractTemplate
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&&)
  {
  }
};

struct WrapConcreteTemplate
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& w)
  {
    typedef typename TypeWrapperT::type WrappedT;
    w.module().method("to_base", [] (WrappedT* w) { return static_cast<AbstractTemplate<double>*>(w); });
  }
};

template<typename T1, typename T2, typename T3>
struct Foo3
{
};

struct WrapFoo3
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.module().method("foo3_method", [] (const WrappedT&) {});
  }
};

struct Foo3FreeMethod
{
  Foo3FreeMethod(jlcxx::Module& mod) : m_module(mod)
  {
  }

  template<typename T>
  void operator()()
  {
    m_module.method("foo3_free_method", [] (T) {});
  }

  jlcxx::Module& m_module;
};

template<typename T1, bool B = false>
struct Foo2
{
};

struct ApplyFoo2
{
  template<typename T> using apply = Foo2<T>;
};

struct WrapFoo2
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.module().set_override_module(wrapped.module().julia_module());
    wrapped.module().method("foo2_method", [] (const WrappedT&) {});
    wrapped.module().unset_override_module();
  }
};

template<typename T>
struct CppVector
{
  typedef T value_type;
  CppVector(T* vec, int size) : m_vec(vec), m_size(size) {}

  const T& get(int i) const { return m_vec[i]; } 

  T* m_vec;
  int m_size;
};

struct WrapCppVector
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.template constructor<typename WrappedT::value_type*, int>();
    wrapped.method("get", &WrappedT::get);
  }
};

template<typename T1, typename T2>
struct CppVector2
{
};

struct WrapCppVector2
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&&)
  {
  }
};

} // namespace parametric

namespace jlcxx
{
  template<typename T>
  struct BuildParameterList<parametric::Foo2<T>>
  {
    typedef ParameterList<T> type;
  };

  template<typename T, T Val>
  struct BuildParameterList<parametric::NonTypeParam<T, Val>>
  {
    typedef ParameterList<T, std::integral_constant<T, Val>> type;
  };

  using namespace parametric;

  template<> struct IsMirroredType<P1> : std::false_type { };
  template<> struct IsMirroredType<P2> : std::false_type { };
  template<typename T1, typename T2> struct IsMirroredType<TemplateType<T1,T2>> : std::false_type { };
  template<typename T1, typename T2> struct IsMirroredType<TemplateDefaultType<T1,T2>> : std::false_type { };
  template<typename T, T I> struct IsMirroredType<NonTypeParam<T,I>> : std::false_type { };
  template<typename T1> struct IsMirroredType<AbstractTemplate<T1>> : std::false_type { };
  template<typename T1> struct IsMirroredType<ConcreteTemplate<T1>> : std::false_type { };
  template<typename T1, typename T2, typename T3> struct IsMirroredType<Foo3<T1,T2,T3>> : std::false_type { };
  template<typename T1, bool B> struct IsMirroredType<Foo2<T1,B>> : std::false_type { };
  template<typename T1> struct IsMirroredType<CppVector<T1>> : std::false_type { };
  template<typename T1, typename T2> struct IsMirroredType<CppVector2<T1,T2>> : std::false_type { };

} // namespace jlcxx

JLCXX_MODULE define_julia_module(jlcxx::Module& types)
{
  using namespace jlcxx;
  using namespace parametric;

  types.add_type<P1>("P1");
  types.add_type<P2>("P2");

  types.add_type<Parametric<TypeVar<1>, TypeVar<2>>>("TemplateType")
    .apply<TemplateType<P1,P2>, TemplateType<P2,P1>>(WrapTemplateType());

  types.add_type<Parametric<TypeVar<1>>>("TemplateDefaultType")
    .apply<TemplateDefaultType<P1>, TemplateDefaultType<P2>>(WrapTemplateDefaultType());

  types.add_type<Parametric<jlcxx::TypeVar<1>, jlcxx::TypeVar<2>>>("NonTypeParam")
    .apply<NonTypeParam<int, 1>, NonTypeParam<unsigned int, 2>, NonTypeParam<long, 64>>(WrapNonTypeParam());

  auto abstract_template = types.add_type<Parametric<jlcxx::TypeVar<1>>>("AbstractTemplate");
  abstract_template.apply<AbstractTemplate<double>>(WrapAbstractTemplate());

  types.add_type<Parametric<jlcxx::TypeVar<1>>>("ConcreteTemplate", abstract_template.dt()).apply<ConcreteTemplate<double>>(WrapConcreteTemplate());

  types.add_type<Parametric<TypeVar<1>, TypeVar<2>, TypeVar<3>>, ParameterList<TypeVar<1>>>("Foo3", abstract_template.dt())
    .apply_combination<Foo3, ParameterList<int32_t, double>, ParameterList<P1,P2,bool>, ParameterList<float>>(WrapFoo3());

  /// Add a non-member function that uses Foo3
  typedef jlcxx::combine_types<jlcxx::ApplyType<Foo3>, ParameterList<int32_t, double>, ParameterList<P1,P2,bool>, ParameterList<float>> foo3_types;
  jlcxx::for_each_type<foo3_types>(Foo3FreeMethod(types));

  types.add_type<Parametric<TypeVar<1>>>("Foo2")
    .apply_combination<ApplyFoo2, ParameterList<int32_t, double>>(WrapFoo2());

  types.add_type<Parametric<TypeVar<1>>>("CppVector", jlcxx::julia_type("AbstractVector"))
    .apply<CppVector<double>, CppVector<std::complex<float>>>(WrapCppVector());

  types.add_type<Parametric<TypeVar<1>, TypeVar<2>>, ParameterList<TypeVar<1>>>("CppVector2", jlcxx::julia_type("AbstractVector"))
    .apply<CppVector2<double,float>>(WrapCppVector2());
}
