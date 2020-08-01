#include <algorithm>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

namespace basic
{

struct ImmutableBits
{
  double a;
  double b;
};

struct MutableBits
{
  double a;
  double b;
};

struct A{
  float x, y;
};

float f(A a){ return a.x + a.y; }
float g(const A & a){ return a.x + a.y; }
float h(const A * a){ return a ? a->x + a->y : 0.0; }

A* return_a_ptr(A& a) { a.x += 1; a.y += 1; return &a; }
const A* return_a_cptr(A& a) { a.x += 1; a.y += 1; return &a; }
A& return_a_ref(A& a) { a.x += 1; a.y += 1; return a; }
const A& return_a_cref(A& a) { a.x += 1; a.y += 1; return a; }

float twice_val(float x) { return 2.0*x; }
float twice_cref(const float& x) { return 2.0*x; }
float twice_ref(float& x) { return 2.0*x; }
void twice_ref_mut(float& x) { x *= 2.0; }
float twice_cptr(const float* x) { return 2.0*(*x); }
float twice_ptr(float* x) { return 2.0*(*x); }
void twice_ptr_mut(float* x) { (*x) *= 2.0; }

int strlen_cchar(const char* str)
{
  return std::string(str).size();
}

int strlen_strref(std::string& s)
{
  return s.size();
}

int strlen_strptr(std::string* s)
{
  return s->size();
}

struct StringHolder
{
  StringHolder(const char* s) : m_str(s) {}
  std::string m_str;
};

std::string str_return_val(const StringHolder& strholder)
{
  return strholder.m_str;
}

const std::string& str_return_cref(const StringHolder& strholder)
{
  return strholder.m_str;
}

std::string& str_return_ref(StringHolder& strholder)
{
  return strholder.m_str;
}

const std::string* str_return_cptr(const StringHolder& strholder)
{
  return &strholder.m_str;
}

std::string* str_return_ptr(StringHolder& strholder)
{
  return &strholder.m_str;
}

struct TypeFtor
{
  template<typename T>
  void apply() { m_count += sizeof(T); }

  int m_count = 0;
};

struct IntTypeLister
{
  IntTypeLister(std::vector<std::string>& typenames, std::vector<jl_value_t*>& datatypes) : m_typenames(typenames), m_datatypes(datatypes)
  {
  }

  template<typename T>
  void operator()()
  {
    m_typenames.push_back(jlcxx::fundamental_int_type_name<T>());
    m_datatypes.push_back((jl_value_t*)jlcxx::julia_type<T>());
  }

  std::vector<std::string>& m_typenames;
  std::vector<jl_value_t*>& m_datatypes;
};

struct FixedIntTypeLister
{
  FixedIntTypeLister(std::vector<std::string>& typenames, std::vector<jl_value_t*>& datatypes) : m_typenames(typenames), m_datatypes(datatypes)
  {
  }

  template<typename T>
  void operator()()
  {
    m_typenames.push_back(jlcxx::fixed_int_type_name<T>());
    m_datatypes.push_back((jl_value_t*)jlcxx::julia_type<T>());
  }

  std::vector<std::string>& m_typenames;
  std::vector<jl_value_t*>& m_datatypes;
};

}

extern "C"
{
  basic::ImmutableBits make_immutable()
  {
    return {1.0,5.0};
  }

  void print_final(basic::ImmutableBits b)
  {
    std::cout << "finalizing bits (" << b.a << "," << b.b << ")" << std::endl;
  }
}

JLCXX_MODULE define_julia_module(jlcxx::Module& mod)
{
  using namespace basic;

  mod.map_type<ImmutableBits>("ImmutableBits");
  mod.map_type<MutableBits>("MutableBits");
  mod.method("increment_immutable", [] (const ImmutableBits& x)
  {
    return ImmutableBits({x.a+1.0, x.b+1.0});
  });

  mod.map_type<A>("A");
  mod.method("f", f);
  mod.method("g", g);
  mod.method("h", h);
  mod.method("return_a_ptr", return_a_ptr);
  mod.method("return_a_cptr", return_a_cptr);
  mod.method("return_a_ref", return_a_ref);
  mod.method("return_a_cref", return_a_cref);

  mod.method("twice_val", twice_val);
  mod.method("twice_cref", twice_cref);
  mod.method("twice_ref", twice_ref);
  mod.method("twice_ref_mut", twice_ref_mut);
  mod.method("twice_cptr", twice_cptr);
  mod.method("twice_ptr", twice_ptr);
  mod.method("twice_ptr_mut", twice_ptr_mut);

  // String tests
  mod.method("strlen_cchar", strlen_cchar);
  mod.method("strlen_char", [] (char* str) { return std::string(str).size(); });
  mod.method("strlen_str", [] (std::string s) { return s.size(); });
  mod.method("strlen_strcref", [] (const std::string& s) { return s.size(); });
  mod.method("strlen_strref", strlen_strref);
  mod.method("strlen_strptr", strlen_strptr);
  mod.method("strlen_strcptr", [] (const std::string* s) { return s->size(); });
  mod.method("print_str", [] (const std::string& s) { std::cout << s << std::endl; });

  mod.add_type<StringHolder>("StringHolder")
    .constructor<const char*>();
  
  mod.method("str_return_val", str_return_val);
  mod.method("str_return_cref", str_return_cref);
  mod.method("str_return_ref", str_return_ref);
  mod.method("str_return_cptr", str_return_cptr);
  mod.method("str_return_ptr", str_return_ptr);

  mod.method("replace_str_val!", [] (std::string& oldstring, const char* newstring) { oldstring = newstring; });


  mod.method("boxed_mirrored_type", [] (void (*f)(jl_value_t*))
  {
    f(jlcxx::box<ImmutableBits>(ImmutableBits({1,2})));
  });
  mod.method("boxed_mutable_mirrored_type", [] (void (*f)(jl_value_t*))
  {
    f(jlcxx::box<MutableBits>(MutableBits({2,3})));
  });

  mod.method("test_for_each_type", [] ()
  {
    TypeFtor f;
    jlcxx::for_each_parameter_type<jlcxx::ParameterList<float, double>>(f);
    return f.m_count;
  });

  mod.method("strict_method", [](jlcxx::StrictlyTypedNumber<long>)
  {
    return std::string("long");
  });
  mod.method("strict_method", [](jlcxx::StrictlyTypedNumber<char>)
  {
    return std::string("char");
  });
  mod.method("strict_method", [](jlcxx::StrictlyTypedNumber<bool>)
  {
    return std::string("bool");
  });

  mod.method("loose_method", [](bool)
  {
    return std::string("bool");
  });
  mod.method("loose_method", [](int)
  {
    return std::string("int");
  });

  mod.method("julia_integer_mapping", []()
  {
    std::vector<std::string> typenames;
    std::vector<jl_value_t*> datatypes;
    typenames.push_back("char");
    datatypes.push_back((jl_value_t*)jlcxx::julia_type<char>());
    jlcxx::for_each_type<jlcxx::fundamental_int_types>(IntTypeLister(typenames,datatypes));
    jlcxx::for_each_type<jlcxx::fixed_int_types>(FixedIntTypeLister(typenames,datatypes));
    return std::make_tuple(typenames,datatypes);
  });
}
