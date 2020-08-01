#include <string>
#include <memory>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

// Dummy base class to test multiple inheritance.
// See https://stackoverflow.com/questions/5445105/conversion-from-void-to-the-pointer-of-the-base-class
struct FirstBase
{
  int allyourbasebelongtous;

  virtual ~FirstBase() {}
};

struct A
{
  virtual std::string message() const = 0;
  virtual ~A() {}
  std::string data = "mydata";
};

struct B : FirstBase, A
{
  virtual std::string message() const
  {
    return "B";
  }
  virtual ~B() {}
};

struct C : B
{
  C() { this->data = "C"; }
  virtual std::string message() const
  {
    return "C";
  }
};

struct D : FirstBase, A
{
  virtual std::string message() const
  {
    return "D";
  }
};

B b;

A& create_abstract()
{
  b = B();
  return b;
}

std::string take_ref(A& a)
{
  return a.message();
}

// Example based on https://discourse.julialang.org/t/simplest-way-to-wrap-virtual-c-class/4977
namespace virtualsolver
{
  typedef double (*history_f) (double);

  class Base
  {
      virtual double history(double) = 0;
    public:
      void solve(){
        for (int i=0;i<3;i++) {
          std::cout<<history((double) i)<<" \n";
        }
      }
      virtual ~Base() {}
  };

  class E: public Base
  {
    double history(double x){return x;}   
  };

  class F: public Base
  {
    public:
      F(history_f h){f=h;}
      double history(double x){return f(x);}

      history_f f;    
  };
}

namespace jlcxx
{
  // Needed for upcasting
  template<> struct SuperType<D> { typedef A type; };
  template<> struct SuperType<C> { typedef B type; };
  template<> struct SuperType<B> { typedef A type; };

  template<> struct SuperType<virtualsolver::E> { typedef virtualsolver::Base type; };
  template<> struct SuperType<virtualsolver::F> { typedef virtualsolver::Base type; };
}

JLCXX_MODULE define_types_module(jlcxx::Module& types)
{
  types.add_type<A>("A").method("message", &A::message);
  types.add_type<B>("B", jlcxx::julia_base_type<A>());
  types.add_type<C>("C", jlcxx::julia_base_type<B>());
  types.add_type<D>("D", jlcxx::julia_base_type<A>());
  types.method("create_abstract", create_abstract);

  types.method("shared_b", []() { return std::make_shared<B>(); });
  types.method("shared_c", []() { return std::make_shared<C>(); });
  types.method("shared_d", []() { return std::make_shared<const D>(); });
  types.method("shared_ptr_message", [](const std::shared_ptr<const A>& x) { return x->message(); });

  types.method("weak_ptr_message_a", [](const std::weak_ptr<A>& x) { return x.lock()->message(); });
  types.method("weak_ptr_message_b", [](const std::weak_ptr<B>& x) { return x.lock()->message(); });

  types.method("dynamic_message_c", [](const A& c) { return dynamic_cast<const C*>(&c)->data; });

  types.method("take_ref", take_ref);
}

JLCXX_MODULE define_vsolver_module(jlcxx::Module& vsolver_mod)
{
  vsolver_mod.add_type<virtualsolver::Base>("BaseV")
    .method("solve", &virtualsolver::Base::solve);

  vsolver_mod.add_type<virtualsolver::E>("E", jlcxx::julia_base_type<virtualsolver::Base>());
  vsolver_mod.add_type<virtualsolver::F>("F", jlcxx::julia_base_type<virtualsolver::Base>())
    .constructor<virtualsolver::history_f>();
}
