#include <algorithm>

#include "jlcxx/jlcxx.hpp"

namespace basic
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

  mod.add_type<std::string>("CppString")
    .constructor<const char*>()
    .method("c_str", [] (const std::string& s) { return s.c_str(); });
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
  // mod.method("str_return_ref", str_return_ref);
  // mod.method("str_return_cptr", str_return_cptr);
  // mod.method("str_return_ptr", str_return_ptr);

}
