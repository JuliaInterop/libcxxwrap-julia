#include <type_traits>
#include <string>
#include <memory>
#include <iostream>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/stl.hpp"

namespace cpp_types
{

// Custom minimal smart pointer type
template<typename T>
struct MySmartPointer
{
  MySmartPointer(T* ptr) : m_ptr(ptr)
  {
  }

  MySmartPointer(std::shared_ptr<T> ptr) : m_ptr(ptr.get())
  {
  }

  T& operator*() const
  {
    return *m_ptr;
  }

  T* m_ptr;
};

struct DoubleData
{
  double a[4];
};

struct World
{
  World(const std::string& message = "default hello") : msg(message){}
  World(jlcxx::cxxint_t) : msg("NumberedWorld") {}
  void set(const std::string& msg) { this->msg = msg; }
  const std::string& greet() const { return msg; }
  std::string msg;
  ~World() { std::cout << "Destroying World with message " << msg << std::endl; }
};

struct Array { Array() {} };

struct NonCopyable
{
  NonCopyable() {}
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable(const NonCopyable&) = delete;
};

struct AConstRef
{
  AConstRef() {}
  int value() const
  {
    return 42;
  }
};

struct ReturnConstRef
{
  const AConstRef& operator()()
  {
    return m_val;
  }

  AConstRef m_val;
};

struct CallOperator
{
  CallOperator() {}

  int operator()() const
  {
    return 43;
  }
};

struct ConstPtrConstruct
{
  ConstPtrConstruct(const World* w) : m_w(w)
  {
  }

  const std::string& greet() { return m_w->greet(); }

  const World* m_w;
};

// Call a function on a type that is defined in Julia
struct JuliaTestType {
  double a;
  double b;
};
void call_testtype_function()
{
  jlcxx::JuliaFunction("julia_test_func")(jlcxx::box<JuliaTestType>(JuliaTestType({2.0, 3.0}), jlcxx::julia_type("JuliaTestType")));
}

enum MyEnum
{
  EnumValA,
  EnumValB
};

enum class EnumClass { red, green = 20, blue };

struct Foo
{
  Foo(const std::wstring& n, jlcxx::ArrayRef<double,1> d) : name(n), data(d.begin(), d.end())
  {
  }

  std::wstring name;
  std::vector<double> data;
};

struct NullableStruct { NullableStruct() {} };

struct IntDerived
{
    int val;
    IntDerived() : val(42) { }
    bool operator == (IntDerived &other){ return this->val == other.val; }
};

} // namespace cpp_types

namespace jlcxx
{
  template<> struct IsMirroredType<cpp_types::DoubleData> : std::false_type { };
  template<typename T> struct IsSmartPointerType<cpp_types::MySmartPointer<T>> : std::true_type { };
  template<typename T> struct ConstructorPointerType<cpp_types::MySmartPointer<T>> { typedef std::shared_ptr<T> type; };
}

class SingletonType
{
  public:
    static SingletonType& instance() {static SingletonType s; return s; }
    int alive() { return 1; }
  private:
    SingletonType() {}
    ~SingletonType() {}
};

JLCXX_MODULE define_julia_module(jlcxx::Module& types)
{
  using namespace cpp_types;

  types.method("call_testtype_function", call_testtype_function);

  types.add_type<DoubleData>("DoubleData");

  types.add_type<World>("World")
    .constructor<const std::string&>()
    .constructor<jlcxx::cxxint_t>(false) // no finalizer
    .constructor([] (const std::string& a, const std::string& b) { return new World(a + " " + b); })
    .method("set", &World::set)
    .method("greet_cref", &World::greet)
    .method("greet_lambda", [] (const World& w) { return w.greet(); } );

  types.method("test_unbox", [] ()
  {
    std::vector<bool> results;
    results.push_back(jlcxx::unbox<int>(jlcxx::JuliaFunction("return_int")()) == 3);
    results.push_back(*jlcxx::unbox<double*>(jlcxx::JuliaFunction("return_ptr_double")()) == 4.0);
    results.push_back(jlcxx::unbox<World>(jlcxx::JuliaFunction("return_world")()).greet() == "returned_world");
    results.push_back(jlcxx::unbox<World*>(jlcxx::JuliaFunction("return_world")())->greet() == "returned_world");
    results.push_back(jlcxx::unbox<World&>(jlcxx::JuliaFunction("return_world")()).greet() == "returned_world");
    results.push_back(jlcxx::unbox<World*>(jlcxx::JuliaFunction("return_world_ptr")())->greet() == "returned_world_ptr");
    results.push_back(jlcxx::unbox<World&>(jlcxx::JuliaFunction("return_world_ref")()).greet() == "returned_world_ref");
    return results;
  });

  types.add_type<Array>("Array");

  types.method("world_factory", []()
  {
    return new World("factory hello");
  });
  
  types.method("shared_world_factory", []() -> const std::shared_ptr<World>
  {
    return std::shared_ptr<World>(new World("shared factory hello"));
  });
  // Shared ptr overload for greet
  types.method("greet_shared", [](const std::shared_ptr<World>& w)
  {
    return w->greet();
  });

  types.method("greet_shared_const", [](const std::shared_ptr<const World>& w)
  {
    return w->greet();
  });

  types.method("shared_world_ref", []() -> std::shared_ptr<World>&
  {
    static std::shared_ptr<World> refworld(new World("shared factory hello ref"));
    return refworld;
  });

  types.method("reset_shared_world!", [](std::shared_ptr<World>& target, std::string message)
  {
    target.reset(new World(message));
  });

  jlcxx::add_smart_pointer<MySmartPointer>(types, "MySmartPointer");

  types.method("smart_world_factory", []()
  {
    return MySmartPointer<World>(new World("smart factory hello"));
  });
  // smart ptr overload for greet
  types.method("greet_smart", [](const MySmartPointer<World>& w)
  {
    return (*w).greet();
  });

  // weak ptr overload for greet
  types.method("greet_weak", [](const std::weak_ptr<World>& w)
  {
    return w.lock()->greet();
  });

  types.method("unique_world_factory", []()
  {
    return std::unique_ptr<const World>(new World("unique factory hello"));
  });

  types.method("world_by_value", [] () -> World
  {
    return World("world by value hello");
  });

  types.method("boxed_world_factory", []()
  {
    static World w("boxed world");
    return jlcxx::box<World&>(w);
  });

  types.method("boxed_world_pointer_factory", []()
  {
    static World w("boxed world pointer");
    return jlcxx::box<World*>(&w);
  });

  types.method("world_ref_factory", []() -> World&
  {
    static World w("reffed world");
    return w;
  });

  types.add_type<NonCopyable>("NonCopyable");

  types.add_type<AConstRef>("AConstRef").method("value", &AConstRef::value);
  types.add_type<ReturnConstRef>("ReturnConstRef").method("value", &ReturnConstRef::operator());

  types.add_type<CallOperator>("CallOperator").method(&CallOperator::operator())
    .method([] (const CallOperator&, int i)  { return i; } );

  types.add_type<ConstPtrConstruct>("ConstPtrConstruct")
    .constructor<const World*>()
    .method("greet_cref", &ConstPtrConstruct::greet);

  // Enum
  types.add_bits<MyEnum>("MyEnum", jlcxx::julia_type("CppEnum"));
  types.set_const("EnumValA", EnumValA);
  types.set_const("EnumValB", EnumValB);

  #if JULIA_VERSION_MAJOR == 1 && JULIA_VERSION_MINOR < 4
  jl_gc_collect(1);
  #else
  jl_gc_collect(JL_GC_FULL);
  #endif

  types.method("enum_to_int", [] (const MyEnum e) { return static_cast<int>(e); });
  types.method("get_enum_b", [] () { return EnumValB; });

  types.add_bits<EnumClass>("EnumClass", jlcxx::julia_type("CppEnum"));
  types.set_const("EnumClassRed", EnumClass::red);
  types.set_const("EnumClassBlue", EnumClass::blue);
  types.method("check_red", [] (const EnumClass c) { return c == EnumClass::red; });

  types.add_type<Foo>("Foo")
    .constructor<const std::wstring&, jlcxx::ArrayRef<double,1>>()
    .method("name", [](Foo& f) { return f.name; })
    .method("data", [](Foo& f) { return jlcxx::ArrayRef<double,1>(&(f.data[0]), f.data.size()); });

  types.method("print_foo_array", [] (jlcxx::ArrayRef<jl_value_t*> farr)
  {
    for(jl_value_t* v : farr)
    {
      const Foo& f = jlcxx::unbox<Foo&>(v);
      std::wcout << f.name << ":";
      for(const double d : f.data)
      {
        std::wcout << " " << d;
      }
      std::wcout << std::endl;
    }
  });

  types.add_type<NullableStruct>("NullableStruct");
  types.method("return_ptr", [] () { return new NullableStruct; });
  types.method("return_null", [] () { return static_cast<NullableStruct*>(nullptr); });

  types.method("greet_vector", [] (const std::vector<World>& v)
  {
    std::stringstream messages;
    for(const World& w : v)
    {
      messages << w.greet() << " ";
    }
    const std::string result = messages.str();
    return result.substr(0,result.size()-1);
  });

  types.add_type<IntDerived>("IntDerived", jlcxx::julia_type("Integer", "Base"));
  types.set_override_module(jl_base_module);
  types.method("==", [](IntDerived& a, IntDerived& b) { return a == b; });
  types.method("Int", [](IntDerived& a) { return a.val; });
  types.unset_override_module();

  types.add_type<SingletonType>("SingeltonType")
    .method("alive", &SingletonType::alive);
  types.method("singleton_instance", SingletonType::instance);
}

JLCXX_MODULE define_types2_module(jlcxx::Module& types2)
{
  types2.method("vecvec", [] (const std::vector<std::vector<int>>& v) { return v[0][0]; });
  types2.method("vecvec", [] (const std::vector<std::vector<cpp_types::World>>& v) { return v[0][0]; });
}

JLCXX_MODULE define_types3_module(jlcxx::Module& types3)
{
  types3.method("vecvec", [] (const std::vector<std::vector<int>>& v) { return 2*v[0][0]; });
  types3.method("vecvec", [] (const std::vector<std::vector<cpp_types::World>>& v) { return v[0][0]; });
}
