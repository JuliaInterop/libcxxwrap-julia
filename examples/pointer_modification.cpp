#include "jlcxx/jlcxx.hpp"

namespace ptrmodif
{

struct MyData
{
  MyData(int v=0) : value(v)
  {
    ++alive_count;
  }
  int value;

  ~MyData()
  {
    assert(alive_count > 0);
    --alive_count;
  }

  // this allows checking that the destructors all ran
  static int alive_count;
};

int MyData::alive_count = 0;

// Shared ptr here to avoid memory leak
std::shared_ptr<MyData> divrem(MyData* a, MyData* b, MyData*& r)
{
  delete r;
  int rem = a->value % b->value;
  if(rem == 0)
  {
    r = nullptr;
  }
  else
  {
    r = new MyData(rem);
  }
  return std::make_shared<MyData>(a->value / b->value);
}

}

JLCXX_MODULE define_julia_module(jlcxx::Module& mod)
{
  using namespace ptrmodif;
  mod.add_type<MyData>("MyData")
    .constructor<int>()
    .method("value", [] (const MyData& d) { return d.value; } )
    .method("setvalue!", [] (MyData& d, const int v) { d.value = v; });
  mod.method("readpointerptr", [] (MyData** ptrref) { return (*ptrref)->value; });
  mod.method("readpointerref", [] (MyData*& ptrref) { return ptrref->value; });
  mod.method("writepointerref!", [] (MyData*& ptrref) { ptrref = new MyData(30); } );
  mod.method("alive_count", [] () { return MyData::alive_count; });
  mod.method("delete", [] (MyData* ptr) { delete ptr; } );

  mod.method("divrem", divrem);
  mod.method("prettydivrem", [] (MyData* a, MyData* b)
  {
    MyData* r = nullptr;
    auto q = divrem(a, b, r);
    return std::make_tuple(q, jlcxx::julia_owned(r));
  });
}
