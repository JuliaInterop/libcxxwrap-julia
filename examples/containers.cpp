#include <tuple>

#include "jlcxx/array.hpp"
#include "jlcxx/jlcxx.hpp"
#include "jlcxx/tuple.hpp"
#include "jlcxx/const_array.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/stl.hpp"

const double* const_vector()
{
  static double d[] = {1., 2., 3};
  return d;
}

const double* const_matrix()
{
  static double d[2][3] = {{1., 2., 3}, {4., 5., 6.}};
  return &d[0][0];
}

// Tuple array test
std::tuple<jlcxx::Array<double>, jlcxx::Array<double>>
make_array_tuple()
{
  jlcxx::Array<double> x;
  jlcxx::Array<double> y;

  x.push_back(1.0);
  x.push_back(2.0);
  y.push_back(3.0);

  return std::make_tuple(x, y);
}

std::tuple<double,int,bool> copy_tuple(std::tuple<double,int,bool> t)
{
  return t;
}

std::vector<double> read_array_tuple(std::tuple<jlcxx::ArrayRef<double>, jlcxx::ArrayRef<double>> t)
{
  jlcxx::ArrayRef<double> x = std::get<0>(t);
  jlcxx::ArrayRef<double> y = std::get<1>(t);

  std::vector<double> result;

  for(auto el : x)
  {
    result.push_back(el);
  }

  for(auto el : y)
  {
    result.push_back(el);
  }

  return result;
}

std::vector<std::tuple<double,double>> make_tuple_vector()
{
  std::vector<std::tuple<double,double>> result;
  result.push_back(std::make_tuple(1.0, 2.0));
  result.push_back(std::make_tuple(3.0, 4.0));
  return result;
}

JLCXX_MODULE define_julia_module(jlcxx::Module& containers)
{
  using namespace jlcxx;

  containers.method("test_tuple", []() { return std::make_tuple(1, 2., 3.f); });
  containers.method("const_ptr", []() { return const_vector(); });
  containers.method("const_ptr_arg", [](const double* p) { return std::make_tuple(p[0], p[1], p[2]); });
  containers.method("const_vector", []() { return jlcxx::make_const_array(const_vector(), 3); });
  // Note the column-major order for matrices
  containers.method("const_matrix", []() { return jlcxx::make_const_array(const_matrix(), 3, 2); });

  containers.method("mutable_array", []()
  {
    static double a[2][3] = {{1., 2., 3}, {4., 5., 6.}};
    return make_julia_array(&a[0][0], 3, 2);
  });
  containers.method("check_mutable_array", [](jlcxx::ArrayRef<double, 2> arr)
  {
    for(auto el : arr)
    {
      if(el != 1.0)
      {
        return false;
      }
    }
    return true;
  });

  containers.method("do_embedding_test", [] ()
  {
    jlcxx::JuliaFunction func1("func1");
    float arr1_jl[] = {1.0, 2.0, 3.0};
    func1((jl_value_t*)jlcxx::ArrayRef<float, 1>(&arr1_jl[0], 3).wrapped());
  });

  containers.method("array_return", [] () {
    Array<std::string> result;
    result.push_back("hello");
    result.push_back("world");
    return result;
  });

  containers.method("int_array_return", [] () {
    jlcxx::Array<int> data{ };
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);

    return data;
  });

  // Test some automatic type creation
  containers.method("tuple_int_pointer", [] () { return std::make_tuple(static_cast<int*>(nullptr), 1); });
  containers.method("uint8_arrayref", [] (jlcxx::ArrayRef<uint8_t *> a)
  {
    int result = 0;
    for(std::size_t i = 0; i != a.size(); ++i)
    {
      result += *(a[i]);
    }
    return result;
  });
  containers.method("uint8_ptr", [] (uint8_t* x) { return int(*x); });
  containers.method("copy_tuple", &copy_tuple);
  containers.method("make_array_tuple", &make_array_tuple);
  containers.method("read_array_tuple", &read_array_tuple);
  containers.method("make_tuple_vector", &make_tuple_vector);
}
