﻿#ifndef JLCXX_ARRAY_HPP
#define JLCXX_ARRAY_HPP

#include "type_conversion.hpp"
#include "tuple.hpp"

namespace jlcxx
{

template<typename PointedT, typename CppT>
struct ValueExtractor
{
  inline CppT operator()(PointedT* p)
  {
    return convert_to_cpp<CppT>(*p);
  }
};


template<typename PointedT>
struct ValueExtractor<PointedT, PointedT>
{
  inline PointedT& operator()(PointedT* p)
  {
    return *p;
  }
};

template<typename PointedT, typename CppT>
class array_iterator_base : public std::iterator<std::random_access_iterator_tag, CppT>
{
private:
  PointedT* m_ptr;
public:
  array_iterator_base() : m_ptr(nullptr)
  {
  }

  explicit array_iterator_base(PointedT* p) : m_ptr(p)
  {
  }

  template <class OtherPointedT, class OtherCppT>
  array_iterator_base(array_iterator_base<OtherPointedT, OtherCppT> const& other) : m_ptr(other.m_ptr) {}

  auto operator*() -> decltype(ValueExtractor<PointedT,CppT>()(m_ptr))
  {
    return ValueExtractor<PointedT,CppT>()(m_ptr);
  }

  array_iterator_base<PointedT, CppT>& operator++()
  {
    ++m_ptr;
    return *this;
  }

  array_iterator_base<PointedT, CppT>& operator--()
  {
    --m_ptr;
    return *this;
  }

  array_iterator_base<PointedT, CppT>& operator+=(std::ptrdiff_t n)
  {
    m_ptr += n;
    return *this;
  }

  array_iterator_base<PointedT, CppT>& operator-=(std::ptrdiff_t n)
  {
    m_ptr -= n;
    return *this;
  }

  PointedT* ptr() const
  {
    return m_ptr;
  }
};

/// Wrap a Julia 1D array in a C++ class. Array is allocated on the C++ side
template<typename ValueT>
class Array
{
public:
  Array(const size_t n = 0)
  {
    jl_value_t* array_type = apply_array_type(static_type_mapping<ValueT>::julia_type(), 1);
    m_array = jl_alloc_array_1d(array_type, n);
  }

  Array(jl_datatype_t* applied_type, const size_t n = 0)
  {
    jl_value_t* array_type = apply_array_type(applied_type, 1);
    m_array = jl_alloc_array_1d(array_type, n);
  }

  /// Append an element to the end of the list
  void push_back(const ValueT& val)
  {
    JL_GC_PUSH1(&m_array);
    const size_t pos = jl_array_len(m_array);
    jl_array_grow_end(m_array, 1);
    jl_arrayset(m_array, box(val), pos);
    JL_GC_POP();
  }

  /// Access to the wrapped array
  jl_array_t* wrapped()
  {
    return m_array;
  }

  // access to the pointer for GC macros
  jl_array_t** gc_pointer()
  {
    return &m_array;
  }

private:
  jl_array_t* m_array;
};

/// Only provide read/write operator[] if the array contains non-boxed values
template<typename PointedT, typename CppT>
struct IndexedArrayRef
{
  IndexedArrayRef(jl_array_t* arr) : m_array(arr)
  {
  }

  CppT operator[](const std::size_t i) const
  {
    return convert_to_cpp<CppT>(jl_arrayref(m_array, i));
  }

  jl_array_t* m_array;
};

template<typename ValueT>
struct IndexedArrayRef<ValueT,ValueT>
{
  IndexedArrayRef(jl_array_t* arr) : m_array(arr)
  {
  }

  ValueT& operator[](const std::size_t i)
  {
    return ((ValueT*)jl_array_data(m_array))[i];
  }

  ValueT operator[](const std::size_t i) const
  {
    return ((ValueT*)jl_array_data(m_array))[i];
  }

  jl_array_t* m_array;
};

/// Reference a Julia array in an STL-compatible wrapper
template<typename ValueT, int Dim = 1>
class ArrayRef : public IndexedArrayRef<mapped_julia_type<ValueT>, ValueT>
{
public:
  ArrayRef(jl_array_t* arr) : IndexedArrayRef<mapped_julia_type<ValueT>, ValueT>(arr)
  {
    assert(wrapped() != nullptr);
  }

  /// Convert from existing C-array (memory owned by C++)
  template<typename... SizesT>
  ArrayRef(ValueT* ptr, const SizesT... sizes);

  /// Convert from existing C-array, explicitly setting Julia ownership
  template<typename... SizesT>
  ArrayRef(const bool julia_owned, ValueT* ptr, const SizesT... sizes);

  typedef mapped_julia_type<ValueT> julia_t;

  typedef array_iterator_base<julia_t, ValueT> iterator;
  typedef array_iterator_base<julia_t const, ValueT const> const_iterator;

  inline jl_array_t* wrapped() const
  {
    return IndexedArrayRef<julia_t, ValueT>::m_array;
  }

  iterator begin()
  {
    return iterator(static_cast<julia_t*>(jl_array_data(wrapped())));
  }

  const_iterator begin() const
  {
    return const_iterator(static_cast<julia_t*>(jl_array_data(wrapped())));
  }

  iterator end()
  {
    return iterator(static_cast<julia_t*>(jl_array_data(wrapped())) + jl_array_len(wrapped()));
  }

  const_iterator end() const
  {
    return const_iterator(static_cast<julia_t*>(jl_array_data(wrapped())) + jl_array_len(wrapped()));
  }

  void push_back(const ValueT& val)
  {
    static_assert(Dim == 1, "push_back is only for 1D ArrayRef");
    jl_array_t* arr_ptr = wrapped();
    JL_GC_PUSH1(&arr_ptr);
    const size_t pos = size();
    jl_array_grow_end(arr_ptr, 1);
    jl_arrayset(arr_ptr, box(val), pos);
    JL_GC_POP();
  }

  const ValueT* data() const
  {
    return (ValueT*)jl_array_data(wrapped());
  }

  ValueT* data()
  {
    return (ValueT*)jl_array_data(wrapped());
  }

  std::size_t size() const
  {
    return jl_array_len(wrapped());
  }
};

template<typename T, int Dim> struct IsValueType<ArrayRef<T,Dim>> : std::true_type {};

// Conversions
template<typename T, int Dim> struct static_type_mapping<ArrayRef<T, Dim>>
{
  typedef jl_array_t* type;
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)apply_array_type(static_type_mapping<T>::julia_type(), Dim); }
};


template<typename ValueT, typename... SizesT>
jl_array_t* wrap_array(const bool julia_owned, ValueT* c_ptr, const SizesT... sizes)
{
  jl_datatype_t* dt = static_type_mapping<ArrayRef<ValueT, sizeof...(SizesT)>>::julia_type();
  jl_value_t *dims = nullptr;
  JL_GC_PUSH1(&dims);
  dims = convert_to_julia(std::make_tuple(static_cast<int_t>(sizes)...));
  jl_array_t* result = jl_ptr_to_array((jl_value_t*)dt, c_ptr, dims, julia_owned);
  JL_GC_POP();
  return result;
}

template<typename ValueT, int Dim>
template<typename... SizesT>
ArrayRef<ValueT, Dim>::ArrayRef(ValueT* c_ptr, const SizesT... sizes) : IndexedArrayRef<julia_t, ValueT>(nullptr)
{
  IndexedArrayRef<julia_t, ValueT>::m_array = wrap_array(false, c_ptr, sizes...);
}

template<typename ValueT, int Dim>
template<typename... SizesT>
ArrayRef<ValueT, Dim>::ArrayRef(const bool julia_owned, ValueT* c_ptr, const SizesT... sizes) : IndexedArrayRef<julia_t, ValueT>(nullptr)
{
  IndexedArrayRef<julia_t, ValueT>::m_array = wrap_array(julia_owned, c_ptr, sizes...);
}

template<typename ValueT, typename... SizesT>
auto make_julia_array(ValueT* c_ptr, const SizesT... sizes) -> ArrayRef<ValueT, sizeof...(SizesT)>
{
  return ArrayRef<ValueT, sizeof...(SizesT)>(true, c_ptr, sizes...);
}

template<typename T, int Dim>
struct ConvertToJulia<ArrayRef<T,Dim>, false, false, false>
{
  template<typename ArrayRefT>
  jl_array_t* operator()(ArrayRefT&& arr) const
  {
    return arr.wrapped();
  }
};

template<typename T, int Dim>
struct ConvertToCpp<ArrayRef<T,Dim>, false, false, false>
{
  ArrayRef<T,Dim> operator()(jl_array_t* arr) const
  {
    return ArrayRef<T,Dim>(arr);
  }
};

// Iterator operator implementation
template<typename L, typename R>
bool operator!=(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return r.ptr() != l.ptr();
}

template<typename L, typename R>
bool operator==(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return r.ptr() == l.ptr();
}

template<typename L, typename R>
bool operator<=(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return l.ptr() <= r.ptr();
}

template<typename L, typename R>
bool operator>=(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return l.ptr() >= r.ptr();
}

template<typename L, typename R>
bool operator>(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return l.ptr() > r.ptr();
}

template<typename L, typename R>
bool operator<(const array_iterator_base<L,L>& l, const array_iterator_base<R,R>& r)
{
  return l.ptr() < r.ptr();
}

template<typename T>
array_iterator_base<T, T> operator+(const array_iterator_base<T,T>& l, const std::ptrdiff_t n)
{
  return array_iterator_base<T, T>(l.ptr() + n);
}

template<typename T>
array_iterator_base<T, T> operator+(const std::ptrdiff_t n, const array_iterator_base<T,T>& r)
{
  return array_iterator_base<T, T>(r.ptr() + n);
}

template<typename T>
array_iterator_base<T, T> operator-(const array_iterator_base<T,T>& l, const std::ptrdiff_t n)
{
  return array_iterator_base<T, T>(l.ptr() - n);
}

template<typename T>
std::ptrdiff_t operator-(const array_iterator_base<T,T>& l, const array_iterator_base<T,T>& r)
{
  return l.ptr() - r.ptr();
}

}

#endif
