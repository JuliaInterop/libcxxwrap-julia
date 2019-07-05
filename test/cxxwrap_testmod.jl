# Minimal CxxWrap dummy module for the tests to run
module CxxWrap

export initialize_cxx_lib

# Base type for wrapped C++ types
abstract type CppEnum end
Base.convert(::Type{Int32}, x::CppEnum) = reinterpret(Int32, x)
import Base: +, |
+(a::T, b::T) where {T <: CppEnum} = reinterpret(T, Int32(a) + Int32(b))
|(a::T, b::T) where {T <: CppEnum} = reinterpret(T, Int32(a) | Int32(b))
cxxupcast(x) = error("No upcast for type $(supertype(typeof(x))). Did you specialize SuperType to enable automatic upcasting?")
abstract type SmartPointer{T} end

const _gc_protected = Dict{UInt64,Tuple{Any, Int}}()

function protect_from_gc(x)
  id = objectid(x)
  (_,n) = get(_gc_protected, id, (x,0))
  _gc_protected[id] = (x,n+1)
  return
end

function unprotect_from_gc(x)
  id = objectid(x)
  (_,n) = get(_gc_protected, id, (x,0))
  if n == 0
    println("warning: attempt to unprotect non-protected object $x")
  end
  if n == 1
    delete!(_gc_protected, id)
  else
    _gc_protected[id] = (x,n-1)
  end
  return
end

function initialize_cxx_lib(initialize_ptr)
  _c_protect_from_gc = @cfunction protect_from_gc Nothing (Any,)
  _c_unprotect_from_gc = @cfunction unprotect_from_gc Nothing (Any,)
  ccall(initialize_ptr, Cvoid, (Any, Any, Ptr{Cvoid}, Ptr{Cvoid}), @__MODULE__, CppFunctionInfo, _c_protect_from_gc, _c_unprotect_from_gc)
end

mutable struct CppFunctionInfo
  name::Any
  argument_types::Array{Type,1}
  return_type::Type
  function_pointer::Int
  thunk_pointer::Int
end

mutable struct ConstructorFname
  _type::DataType
end

end