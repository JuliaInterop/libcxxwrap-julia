# Minimal CxxWrap dummy module for the tests to run
module CxxWrap

# Base type for wrapped C++ types
abstract type CppEnum end
Base.convert(::Type{Int32}, x::CppEnum) = reinterpret(Int32, x)
import Base: +, |
+(a::T, b::T) where {T <: CppEnum} = reinterpret(T, Int32(a) + Int32(b))
|(a::T, b::T) where {T <: CppEnum} = reinterpret(T, Int32(a) | Int32(b))
cxxdowncast(x) = error("No downcast for type $(supertype(typeof(x))). Did you specialize SuperType to enable automatic downcasting?")
abstract type SmartPointer{T} end

mutable struct CppFunctionInfo
  name::Any
  argument_types::Array{Type,1}
  reference_argument_types::Array{Type,1}
  return_type::Type
  function_pointer::Int
  thunk_pointer::Int
end

mutable struct ConstructorFname
  _type::DataType
end

end