# Minimal CxxWrap dummy module for the tests to run
module CxxWrap

# Base type for wrapped C++ types
abstract type CppAny end
abstract type CppBits <: CppAny end
abstract type CppDisplay <: Display end
abstract type CppArray{T,N} <: AbstractArray{T,N} end
abstract type CppAssociative{K,V} <: Associative{K,V} end
abstract type CppEnum end
Base.convert(::Type{Int32}, x::CppEnum) = reinterpret(Int32, x)
import Base: +, |
+{T <: CppEnum}(a::T, b::T) = reinterpret(T, Int32(a) + Int32(b))
|{T <: CppEnum}(a::T, b::T) = reinterpret(T, Int32(a) | Int32(b))
cxxdowncast(x) = error("No downcast for type $(supertype(typeof(x))). Did you specialize SuperType to enable automatic downcasting?")
abstract type SmartPointer{T} <: CppAny end

type CppFunctionInfo
  name::Any
  argument_types::Array{Type,1}
  reference_argument_types::Array{Type,1}
  return_type::Type
  function_pointer::Ptr{Void}
  thunk_pointer::Ptr{Void}
end

type ConstructorFname
  _type::DataType
end

end