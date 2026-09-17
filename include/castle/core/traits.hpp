// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// STL-free compile-time traits and type transformations for Castle.
// Include this header when generic embedded code needs constexpr predicates,
// SFINAE helpers, type transformations, callable detection, or Castle-specific
// tag utilities without depending on <type_traits>. The implementation prefers
// compiler builtins where object-model queries would otherwise be non-portable,
// and all facilities remain allocation-free and compile-time oriented.

#ifndef CASTLE_CORE_TRAITS_HPP
#define CASTLE_CORE_TRAITS_HPP

#include "castle/core/compiler.hpp"

#include <stddef.h>
#include <stdint.h>
#include <float.h>

namespace castle
{
/// @brief Namespace containing Castle's primary trait and metaprogramming utilities.
namespace meta
{

/// @brief Wraps a compile-time constant value as a type.
/// @tparam T Stored value type.
/// @tparam Value Stored constant value.
template <typename T, T Value>
struct integral_constant
{
    static CASTLE_CONSTEXPR T value = Value;
    using value_type = T;
    using type = integral_constant<T, Value>;

    /// @brief Implicitly converts to the stored constant value.
    /// @return The wrapped constant value.
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    /// @brief Returns the stored constant value.
    /// @return The wrapped constant value.
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

template <bool Value>
using bool_constant = integral_constant<bool, Value>;
using true_type  = bool_constant<true>;
using false_type = bool_constant<false>;

/// @brief Helper that maps any well-formed type pack to void.
/// @tparam Ts Ignored type pack.
template <typename...>
struct voider { using type = void; };

template <typename... Ts>
using void_t = typename voider<Ts...>::type;

/// @brief Exposes a type unchanged while blocking deduction in some contexts.
/// @tparam T Type to preserve.
template <typename T> struct type_identity { using type = T; };
template <typename T> using type_identity_t = typename type_identity<T>::type;

/// @brief Compile-time false value that remains dependent on template arguments.
/// @tparam Ts Ignored type pack.
template <typename...> struct always_false    : false_type {};
/// @brief Single-type variant of always_false for dependent static assertions.
/// @tparam T Ignored type parameter.
template <typename T>  struct dependent_false : false_type {};

/// @brief Logical AND over a pack of trait types.
/// @tparam Ts Trait types whose ::value members are combined.
template <typename... Ts>
struct conjunction : true_type {};

template <typename T>
struct conjunction<T> : T {};

/// @brief Logical OR over a pack of trait types.
/// @tparam Ts Trait types whose ::value members are combined.
template <typename... Ts>
struct disjunction : false_type {};

template <typename T>
struct disjunction<T> : T {};

/// @brief Logical NOT of a trait type.
/// @tparam T Trait type whose ::value is negated.
template <typename T>
struct negation : bool_constant<!bool(T::value)> {};

/// @brief Defines a nested type only when Condition is true.
/// @tparam Condition Compile-time boolean condition.
/// @tparam T Exposed type when Condition is true.
template <bool Condition, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> { using type = T; };

template <bool Condition, typename T = void>
using enable_if_t = typename enable_if<Condition, T>::type;

/// @brief Selects one of two types based on a compile-time condition.
/// @tparam Condition Compile-time boolean condition.
/// @tparam T Type selected when Condition is true.
/// @tparam F Type selected when Condition is false.
template <bool Condition, typename T, typename F>
struct conditional { using type = F; };

template <typename T, typename F>
struct conditional<true, T, F> { using type = T; };

template <bool Condition, typename T, typename F>
using conditional_t = typename conditional<Condition, T, F>::type;

template <typename T, typename... Ts>
struct conjunction<T, Ts...>
    : conditional_t<bool(T::value), conjunction<Ts...>, T> {};

template <typename T, typename... Ts>
struct disjunction<T, Ts...>
    : conditional_t<bool(T::value), T, disjunction<Ts...>> {};

/// @brief Removes lvalue and rvalue reference qualifiers from a type.
/// @tparam T Type to transform.
template <typename T> struct remove_reference       { using type = T; };
template <typename T> struct remove_reference<T&>   { using type = T; };
template <typename T> struct remove_reference<T&&>  { using type = T; };
template <typename T> using  remove_reference_t = typename remove_reference<T>::type;

/// @brief Adds an lvalue-reference qualifier to a type.
/// @tparam T Type to transform.
template <typename T> struct add_lvalue_reference { using type = T&; };
/// @brief Adds an rvalue-reference qualifier to a type.
/// @tparam T Type to transform.
template <typename T> struct add_rvalue_reference { using type = T&&; };
template <typename T> using  add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
template <typename T> using  add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

/// @brief Reports whether a type is an lvalue or rvalue reference.
/// @tparam T Type to test.
template <typename T> struct is_reference        : false_type {};
template <typename T> struct is_reference<T&>    : true_type  {};
template <typename T> struct is_reference<T&&>   : true_type  {};

/// @brief Reports whether a type is an lvalue reference.
/// @tparam T Type to test.
template <typename T> struct is_lvalue_reference       : false_type {};
template <typename T> struct is_lvalue_reference<T&>   : true_type  {};

/// @brief Reports whether a type is an rvalue reference.
/// @tparam T Type to test.
template <typename T> struct is_rvalue_reference       : false_type {};
template <typename T> struct is_rvalue_reference<T&&>  : true_type  {};

/// @brief Produces an unevaluated rvalue reference to T for use in expressions.
/// @tparam T Type to model.
/// @return An unevaluated rvalue reference to T.
template <typename T>
add_rvalue_reference_t<T> declval() CASTLE_NOEXCEPT;

/// @brief Removes a const qualifier from a type.
/// @tparam T Type to transform.
template <typename T> struct remove_const              { using type = T; };
template <typename T> struct remove_const<CASTLE_CONST T>     { using type = T; };

/// @brief Removes a volatile qualifier from a type.
/// @tparam T Type to transform.
template <typename T> struct remove_volatile           { using type = T; };
template <typename T> struct remove_volatile<CASTLE_VOLATILE T> { using type = T; };

/// @brief Removes both const and volatile qualifiers from a type.
/// @tparam T Type to transform.
template <typename T>
struct remove_cv
{
    using type = typename remove_const<typename remove_volatile<T>::type>::type;
};

template <typename T> using remove_const_t    = typename remove_const<T>::type;
template <typename T> using remove_volatile_t = typename remove_volatile<T>::type;
template <typename T> using remove_cv_t       = typename remove_cv<T>::type;

/// @brief Adds a const qualifier to a type.
/// @tparam T Type to transform.
template <typename T> struct add_const    { using type = CASTLE_CONST T; };
/// @brief Adds a volatile qualifier to a type.
/// @tparam T Type to transform.
template <typename T> struct add_volatile { using type = CASTLE_VOLATILE T; };
/// @brief Adds both const and volatile qualifiers to a type.
/// @tparam T Type to transform.
template <typename T> struct add_cv       { using type = CASTLE_CONST CASTLE_VOLATILE T; };
template <typename T> using  add_const_t    = typename add_const<T>::type;
template <typename T> using  add_volatile_t = typename add_volatile<T>::type;
template <typename T> using  add_cv_t       = typename add_cv<T>::type;

/// @brief Reports whether a type is const-qualified.
/// @tparam T Type to test.
template <typename T> struct is_const              : false_type {};
template <typename T> struct is_const<CASTLE_CONST T>     : true_type  {};

/// @brief Reports whether a type is volatile-qualified.
/// @tparam T Type to test.
template <typename T> struct is_volatile              : false_type {};
template <typename T> struct is_volatile<CASTLE_VOLATILE T>  : true_type  {};

/// @brief Internal helper for void detection after cv removal.
/// @tparam T Type to test.
template <typename T> struct is_void_impl               : false_type {};
template <>           struct is_void_impl<void>         : true_type  {};
/// @brief Reports whether a type is void after removing cv qualifiers.
/// @tparam T Type to test.
template <typename T> struct is_void : is_void_impl<remove_cv_t<T>> {};

/// @brief Internal helper for nullptr detection after cv removal.
/// @tparam T Type to test.
template <typename T> struct is_null_pointer_impl                    : false_type {};
template <>           struct is_null_pointer_impl<decltype(nullptr)> : true_type  {};
/// @brief Reports whether a type is std::nullptr_t after removing cv qualifiers.
/// @tparam T Type to test.
template <typename T> struct is_null_pointer : is_null_pointer_impl<remove_cv_t<T>> {};

/// @brief Reports whether a type is one of the standard integral types.
/// @tparam T Type to test.
template <typename T>
struct is_integral : false_type {};
#define CASTLE_DEFINE_INTEGRAL(T) template <> struct is_integral<T> : true_type {}
CASTLE_DEFINE_INTEGRAL(bool);
CASTLE_DEFINE_INTEGRAL(char);
CASTLE_DEFINE_INTEGRAL(signed char);
CASTLE_DEFINE_INTEGRAL(unsigned char);
CASTLE_DEFINE_INTEGRAL(wchar_t);
CASTLE_DEFINE_INTEGRAL(char16_t);
CASTLE_DEFINE_INTEGRAL(char32_t);
CASTLE_DEFINE_INTEGRAL(short);
CASTLE_DEFINE_INTEGRAL(unsigned short);
CASTLE_DEFINE_INTEGRAL(int);
CASTLE_DEFINE_INTEGRAL(unsigned int);
CASTLE_DEFINE_INTEGRAL(long);
CASTLE_DEFINE_INTEGRAL(unsigned long);
CASTLE_DEFINE_INTEGRAL(long long);
CASTLE_DEFINE_INTEGRAL(unsigned long long);
#undef CASTLE_DEFINE_INTEGRAL

/// @brief Reports whether a type is float, double, or long double.
/// @tparam T Type to test.
template <typename T> struct is_floating_point              : false_type {};
template <>           struct is_floating_point<float>       : true_type  {};
template <>           struct is_floating_point<double>      : true_type  {};
template <>           struct is_floating_point<long double> : true_type  {};

/// @brief Exposes the machine epsilon for a floating-point type.
/// @tparam T Floating-point type to query.
template <typename T> struct floating_epsilon;

template <> struct floating_epsilon<float>
{
    using value_type = float;
    static CASTLE_CONSTEXPR float value = __FLT_EPSILON__;
    /// @brief Implicitly converts to the stored epsilon value.
    /// @return The epsilon value for float.
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    /// @brief Returns the stored epsilon value.
    /// @return The epsilon value for float.
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

template <> struct floating_epsilon<double>
{
    using value_type = double;
    static CASTLE_CONSTEXPR double value = __DBL_EPSILON__;
    /// @brief Implicitly converts to the stored epsilon value.
    /// @return The epsilon value for double.
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    /// @brief Returns the stored epsilon value.
    /// @return The epsilon value for double.
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

template <> struct floating_epsilon<long double>
{
    using value_type = long double;
    static CASTLE_CONSTEXPR long double value = __LDBL_EPSILON__;
    /// @brief Implicitly converts to the stored epsilon value.
    /// @return The epsilon value for long double.
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    /// @brief Returns the stored epsilon value.
    /// @return The epsilon value for long double.
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

/// @brief Reports whether a type is an array of known or unknown bound.
/// @tparam T Type to test.
template <typename T>           struct is_array        : false_type {};
template <typename T>           struct is_array<T[]>   : true_type  {};
template <typename T, size_t N> struct is_array<T[N]>  : true_type  {};

/// @brief Reports whether a type is a non-member pointer.
/// @tparam T Type to test.
template <typename T> struct is_pointer      : false_type {};
template <typename T> struct is_pointer<T*>  : true_type  {};

/// @brief Reports whether a type is an enumeration.
/// @tparam T Type to test.
template <typename T>
struct is_enum
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_enum(T)>
#else
    : false_type
#endif
{};

/// @brief Reports whether a type is a class type.
/// @tparam T Type to test.
template <typename T>
struct is_class
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_class(T)>
#else
    : false_type
#endif
{};

/// @brief Reports whether a type is a union type.
/// @tparam T Type to test.
template <typename T>
struct is_union
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_union(T)> {};
#else
    : false_type {};
#endif

/// @brief Reports whether a type is a free-function type.
/// @tparam T Type to test.
template <typename T> struct is_function : false_type {};
template <typename R, typename... Args> struct is_function<R(Args...)>                : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_CONST>          : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_VOLATILE>       : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_CONST CASTLE_VOLATILE> : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) &>              : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) &&>             : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_NOEXCEPT>       : true_type {};

template <typename T>              struct is_member_pointer_impl        : false_type {};
template <typename T, typename C>  struct is_member_pointer_impl<T C::*> : true_type {};
/// @brief Reports whether a type is a pointer to a non-static data or function member.
/// @tparam T Type to test.
template <typename T>
struct is_member_pointer : is_member_pointer_impl<typename remove_cv<T>::type> {};

template <typename T> struct is_member_function_pointer_impl : false_type {};
template <typename T, typename C>
struct is_member_function_pointer_impl<T C::*> : bool_constant<is_function<T>::value> {};
/// @brief Reports whether a type is a pointer to a non-static member function.
/// @tparam T Type to test.
template <typename T>
struct is_member_function_pointer
    : is_member_function_pointer_impl<typename remove_cv<T>::type> {};

/// @brief Reports whether a type is integral or floating-point.
/// @tparam T Type to test.
template <typename T>
struct is_arithmetic
    : bool_constant<is_integral<T>::value || is_floating_point<T>::value> {};

/// @brief Reports whether a type is arithmetic, void, or nullptr_t.
/// @tparam T Type to test.
template <typename T>
struct is_fundamental
    : bool_constant<is_arithmetic<T>::value || is_void<T>::value
                    || is_null_pointer<T>::value> {};

/// @brief Reports whether a type is a scalar type.
/// @tparam T Type to test.
template <typename T>
struct is_scalar
    : bool_constant<is_arithmetic<T>::value || is_enum<T>::value
                    || is_pointer<T>::value || is_member_pointer<T>::value
                    || is_null_pointer<T>::value> {};

/// @brief Reports whether a type is an object type.
/// @tparam T Type to test.
template <typename T>
struct is_object
    : bool_constant<!is_function<T>::value && !is_reference<T>::value
                    && !is_void<T>::value> {};

/// @brief Reports whether a type is not a fundamental type.
/// @tparam T Type to test.
template <typename T>
struct is_compound : bool_constant<!is_fundamental<T>::value> {};

template <typename T, bool = is_arithmetic<remove_cv_t<T>>::value>
struct is_signed_impl : false_type {};
template <typename T>
struct is_signed_impl<T, true> : bool_constant<(T(-1) < T(0))> {};
/// @brief Reports whether an arithmetic type is signed.
/// @tparam T Type to test.
template <typename T>
struct is_signed : is_signed_impl<remove_cv_t<T>> {};

template <typename T, bool = is_arithmetic<remove_cv_t<T>>::value>
struct is_unsigned_impl : false_type {};
template <typename T>
struct is_unsigned_impl<T, true> : bool_constant<(T(0) < T(-1))> {};
/// @brief Reports whether an arithmetic type is unsigned.
/// @tparam T Type to test.
template <typename T>
struct is_unsigned : is_unsigned_impl<remove_cv_t<T>> {};

/// @brief Maps an unsigned integer type to its signed counterpart when supported.
/// @tparam T Type to transform.
template <typename T> struct make_signed                     { using type = T; };
template <> struct make_signed<unsigned char>                { using type = signed char; };
template <> struct make_signed<unsigned short>               { using type = short; };
template <> struct make_signed<unsigned int>                 { using type = int; };
template <> struct make_signed<unsigned long>                { using type = long; };
template <> struct make_signed<unsigned long long>           { using type = long long; };
template <typename T> using make_signed_t = typename make_signed<T>::type;

/// @brief Maps a signed integer type to its unsigned counterpart when supported.
/// @tparam T Type to transform.
template <typename T> struct make_unsigned                   { using type = T; };
template <> struct make_unsigned<signed char>                { using type = unsigned char; };
template <> struct make_unsigned<char>                       { using type = unsigned char; };
template <> struct make_unsigned<short>                      { using type = unsigned short; };
template <> struct make_unsigned<int>                        { using type = unsigned int; };
template <> struct make_unsigned<long>                       { using type = unsigned long; };
template <> struct make_unsigned<long long>                  { using type = unsigned long long; };
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

/// @brief Reports whether a type is trivially copyable.
/// @tparam T Type to test.
template <typename T>
struct is_trivially_copyable
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_trivially_copyable(T)>
#else
    : false_type
#endif
{};

/// @brief Reports whether a type has a trivial destructor.
/// @tparam T Type to test.
template <typename T>
struct is_trivially_destructible
#if defined(__clang__)
    : bool_constant<__is_trivially_destructible(T)>
#elif defined(__GNUC__)
    : bool_constant<__has_trivial_destructor(T)>
#else
    : false_type
#endif
{};

#if defined(__clang__) || defined(__GNUC__)
template <typename T> struct is_empty              : bool_constant<__is_empty(T)> {};
template <typename T> struct is_polymorphic        : bool_constant<__is_polymorphic(T)> {};
template <typename T> struct is_abstract           : bool_constant<__is_abstract(T)> {};
template <typename T> struct is_final              : bool_constant<__is_final(T)> {};
template <typename T> struct has_virtual_destructor
    : bool_constant<__has_virtual_destructor(T)> {};
template <typename T> struct is_standard_layout    : bool_constant<__is_standard_layout(T)> {};
template <typename T> struct is_trivial            : bool_constant<__is_trivial(T)> {};
#endif

/// @brief Exposes the alignment requirement of a type.
/// @tparam T Type to query.
template <typename T>
struct alignment_of : integral_constant<size_t, alignof(T)> {};

/// @brief Castle-aligned storage block matching long double alignment.
struct max_align_t
{
    alignas(long double) unsigned char data[sizeof(long double)];
};

#if defined(__clang__) && defined(__has_builtin)
    #if __has_builtin(__is_constructible)
        #define CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE 1
    #endif
#elif defined(__GNUC__) && (__GNUC__ >= 8)
    #define CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE 1
#endif

#if defined(CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE)

/// @brief Reports whether T can be constructed from Args....
/// @tparam T Type to construct.
/// @tparam Args Constructor argument types.
template <typename T, typename... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};

/// @brief Reports whether T can be constructed from Args... without throwing.
/// @tparam T Type to construct.
/// @tparam Args Constructor argument types.
template <typename T, typename... Args>
struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(T, Args...)> {};

#else

template <typename T, typename... Args>
struct is_constructible_impl
{
private:
    template <typename U, typename... A,
              typename = decltype(U(declval<A>()...))>
    static true_type  test(int);
    template <typename, typename...>
    static false_type test(...);

public:
    using type = decltype(test<T, Args...>(0));
};

template <typename T, typename... Args>
struct is_constructible : is_constructible_impl<T, Args...>::type {};

template <typename T, typename... Args>
struct is_nothrow_constructible_impl
{
private:
    template <typename U, typename... A>
    static bool_constant<CASTLE_NOEXCEPT(U(declval<A>()...))> test(int);
    template <typename, typename...>
    static false_type                                  test(...);

public:
    using type = decltype(test<T, Args...>(0));
};

template <typename T, typename... Args>
struct is_nothrow_constructible
    : conditional_t<is_constructible<T, Args...>::value,
                    typename is_nothrow_constructible_impl<T, Args...>::type,
                    false_type> {};

#endif

/// @brief Reports whether a type is default-constructible.
/// @tparam T Type to test.
template <typename T> struct is_default_constructible        : is_constructible<T> {};
template <typename T> struct is_copy_constructible           : is_constructible<T, add_lvalue_reference_t<CASTLE_CONST T>> {};
template <typename T> struct is_move_constructible           : is_constructible<T, add_rvalue_reference_t<T>> {};
template <typename T> struct is_nothrow_default_constructible : is_nothrow_constructible<T> {};
template <typename T> struct is_nothrow_copy_constructible   : is_nothrow_constructible<T, add_lvalue_reference_t<CASTLE_CONST T>> {};
template <typename T> struct is_nothrow_move_constructible   : is_nothrow_constructible<T, add_rvalue_reference_t<T>> {};

template <typename T, bool = is_trivially_destructible<T>::value>
struct is_destructible_impl;
template <typename T>
struct is_destructible_impl<T, true>  : true_type {};
template <typename T>
struct is_destructible_impl<T, false>
    : bool_constant<decltype(declval<T&>().~T(), true_type{})::value> {};
/// @brief Reports whether a type is destructible.
/// @tparam T Type to test.
template <typename T>
struct is_destructible : is_destructible_impl<T> {};

template <typename T, bool = is_trivially_destructible<T>::value>
struct is_nothrow_destructible_impl;
template <typename T>
struct is_nothrow_destructible_impl<T, true>  : true_type {};
template <typename T>
struct is_nothrow_destructible_impl<T, false>
    : bool_constant<CASTLE_NOEXCEPT(declval<T&>().~T())> {};
/// @brief Reports whether a type is destructible without throwing.
/// @tparam T Type to test.
template <typename T>
struct is_nothrow_destructible : is_nothrow_destructible_impl<T> {};

#if defined(__clang__) && defined(__has_builtin)
    #if __has_builtin(__is_assignable)
        #define CASTLE_HAS_BUILTIN_IS_ASSIGNABLE 1
    #endif
#elif defined(__GNUC__) && (__GNUC__ >= 9)
    #define CASTLE_HAS_BUILTIN_IS_ASSIGNABLE 1
#endif

#if defined(CASTLE_HAS_BUILTIN_IS_ASSIGNABLE)

template <typename T> struct is_copy_assignable : bool_constant<__is_assignable(T&, CASTLE_CONST T&)> {};
template <typename T> struct is_move_assignable : bool_constant<__is_assignable(T&, T&&)> {};

#else

template <typename T, typename U>
struct is_assignable_impl
{
private:
    template <typename L, typename R,
              typename = decltype(declval<L>() = declval<R>())>
    static true_type  test(int);
    template <typename, typename>
    static false_type test(...);

public:
    using type = decltype(test<T, U>(0));
};

template <typename T> struct is_copy_assignable : is_assignable_impl<T&, CASTLE_CONST T&>::type {};
template <typename T> struct is_move_assignable : is_assignable_impl<T&, T&&>::type      {};

#endif

/// @brief Reports whether two types are exactly the same.
/// @tparam T First type.
/// @tparam U Second type.
template <typename T, typename U> struct is_same       : false_type {};
template <typename T>             struct is_same<T, T> : true_type  {};

/// @brief Selects the largest type in a parameter pack by sizeof.
/// @tparam T1 First type in the pack.
/// @tparam TRest Remaining types.
template <typename T1, typename... TRest>
struct largest_type
{
    using type = conditional_t<sizeof(T1) >= sizeof(typename largest_type<TRest...>::type),
                               T1,
                               typename largest_type<TRest...>::type>;
    using size = integral_constant<size_t, sizeof(type)>;
};

template <typename T>
struct largest_type<T>
{
    using type = T;
    using size = integral_constant<size_t, sizeof(T)>;
};

/// @brief Reports whether Base is a base class of Derived.
/// @tparam Base Candidate base type.
/// @tparam Derived Candidate derived type.
template <typename Base, typename Derived>
struct is_base_of
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_base_of(Base, Derived)> {};
#else
    : false_type {};
#endif

template <typename From, typename To, typename = void>
struct is_convertible_impl : false_type {};

template <typename From, typename To>
struct is_convertible_impl<
    From, To,
    void_t<decltype(static_cast<void>(static_cast<To(*)()>(nullptr)),
                    static_cast<void(*)(To)>(nullptr)(declval<From>()))>>
    : true_type {};

/// @brief Reports whether From is implicitly convertible to To.
/// @tparam From Source type.
/// @tparam To Destination type.
template <typename From, typename To>
struct is_convertible
    : conditional_t<is_void<From>::value && is_void<To>::value,
                    true_type,
                    is_convertible_impl<From, To>> {};

/// @brief Removes one pointer indirection from a type.
/// @tparam T Type to transform.
template <typename T> struct remove_pointer                    { using type = T; };
template <typename T> struct remove_pointer<T*>                { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_CONST>          { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_VOLATILE>       { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_CONST CASTLE_VOLATILE> { using type = T; };
template <typename T> using  remove_pointer_t = typename remove_pointer<T>::type;

/// @brief Internal helper that adds a pointer to a type when possible.
/// @tparam T Type to transform.
template <typename T, typename = void>
struct add_pointer_impl { using type = T; };
template <typename T>
struct add_pointer_impl<T, void_t<remove_reference_t<T>*>>
{ using type = remove_reference_t<T>*; };
/// @brief Adds a pointer to a type after removing references when possible.
/// @tparam T Type to transform.
template <typename T> struct add_pointer : add_pointer_impl<T> {};
template <typename T> using  add_pointer_t = typename add_pointer<T>::type;

/// @brief Removes one array extent from a type.
/// @tparam T Type to transform.
template <typename T>           struct remove_extent          { using type = T; };
template <typename T>           struct remove_extent<T[]>     { using type = T; };
template <typename T, size_t N> struct remove_extent<T[N]>    { using type = T; };
template <typename T> using remove_extent_t = typename remove_extent<T>::type;

/// @brief Removes all array extents from a type.
/// @tparam T Type to transform.
template <typename T>           struct remove_all_extents        { using type = T; };
template <typename T>           struct remove_all_extents<T[]>   { using type = typename remove_all_extents<T>::type; };
template <typename T, size_t N> struct remove_all_extents<T[N]>  { using type = typename remove_all_extents<T>::type; };
template <typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

/// @brief Applies array/function decay and removes cv-ref qualifiers.
/// @tparam T Type to decay.
template <typename T>
struct decay
{
private:
    using U = remove_reference_t<T>;
public:
    using type = conditional_t<
        is_array<U>::value,
        remove_extent_t<U>*,
        conditional_t<
            is_function<U>::value,
            add_pointer_t<U>,
            remove_cv_t<U>>>;
};
template <typename T> using decay_t = typename decay<T>::type;

/// @brief Computes a common decayed type for a set of types.
/// @tparam Ts Types to combine.
template <typename... Ts> struct common_type {};

template <typename T>
struct common_type<T> { using type = decay_t<T>; };

template <typename T, typename U>
struct common_type<T, U>
{
    using type = decay_t<decltype(true ? declval<T>() : declval<U>())>;
};

template <typename T, typename U, typename... Rest>
struct common_type<T, U, Rest...>
{
    using type = typename common_type<typename common_type<T, U>::type, Rest...>::type;
};
template <typename... Ts> using common_type_t = typename common_type<Ts...>::type;

#if defined(__clang__) || defined(__GNUC__)
template <typename T> struct underlying_type { using type = __underlying_type(T); };
template <typename T> using  underlying_type_t = typename underlying_type<T>::type;
#endif

/// @brief Exposes the result type of invoking a callable with Args....
/// @tparam F Callable type.
/// @tparam Args Argument types.
template <typename F, typename... Args>
struct invoke_result
{
    using type = decltype(declval<F>()(declval<Args>()...));
};
template <typename F, typename... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

template <typename Void, typename F, typename... Args>
struct is_invocable_impl : false_type {};

template <typename F, typename... Args>
struct is_invocable_impl<void_t<decltype(declval<F>()(declval<Args>()...))>,
                         F, Args...> : true_type {};

/// @brief Reports whether a callable can be invoked with Args....
/// @tparam F Callable type.
/// @tparam Args Argument types.
template <typename F, typename... Args>
struct is_invocable : is_invocable_impl<void, F, Args...> {};

/// @brief Reports whether a callable can be invoked with Args... and converted to R.
/// @tparam R Desired result type.
/// @tparam F Callable type.
/// @tparam Args Argument types.
template <typename R, typename F, typename... Args>
struct is_invocable_r
    : bool_constant<is_invocable<F, Args...>::value
   && is_convertible<invoke_result_t<F, Args...>, R>::value> {};

/// @brief Reports whether the template size value is a power of two.
/// @tparam N Value to test.
template <size_t N>
struct is_power_of_two
{
    static CASTLE_CONSTEXPR bool value = ((N > 0) && (N & (N - 1)) == 0);
};

/// @brief Reports whether a type is a non-bool integral type whose size is a power of two.
/// @tparam T Type to test.
template <typename T>
struct is_valid_integer
{
    static CASTLE_CONSTEXPR bool is_int = is_integral<T>::value && !is_same<T, bool>::value;
    static CASTLE_CONSTEXPR bool is_size_power_of_2 = is_power_of_two<sizeof(T)>::value;
    static CASTLE_CONSTEXPR bool value = is_int && is_size_power_of_2;
};

/// @brief Reports whether a type appears in a parameter pack.
/// @tparam T Type to search for.
/// @tparam Us Candidate types.
template <typename T, typename... Us>
struct pack_contains : false_type {};

template <typename T, typename U, typename... Us>
struct pack_contains<T, U, Us...>
    : conditional_t<is_same<T, U>::value, true_type, pack_contains<T, Us...>> {};

/// @brief Reports whether every type in a parameter pack is unique.
/// @tparam Ts Types to test.
template <typename... Ts> struct has_unique_types;
template <>               struct has_unique_types<>  : true_type {};
template <typename T>     struct has_unique_types<T> : true_type {};
template <typename Head, typename... Tail>
struct has_unique_types<Head, Tail...>
    : bool_constant<!pack_contains<Head, Tail...>::value
                    && has_unique_types<Tail...>::value> {};

/// @brief Reports whether a type is a specialization of the given class template.
/// @tparam T Type to test.
/// @tparam Template Template to match against.
template <typename T, template <typename...> class Template>
struct is_specialization_of : false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : true_type {};

template <typename Required, typename Actual>
using enable_if_at_least =
    typename enable_if<is_base_of<Required, Actual>::value, int>::type;

template <typename From, typename To>
using enable_if_convertible =
    typename enable_if<is_convertible<From, To>::value, int>::type;

template <typename T, typename U>
using enable_if_same =
    typename enable_if<is_same<T, U>::value, int>::type;

/// @brief Compile-time whitespace character set for a character type.
/// @tparam TChar Character type to query.
template <typename TChar>
struct whitespace;

template <>
struct whitespace<char>
{
    static CASTLE_CONSTEXPR CASTLE_CONST char* value = " \t\f\v";
};

template <>
struct whitespace<wchar_t>
{
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* value = L" \t\f\v";
};

/// @brief Compile-time end-of-line character set for a character type.
/// @tparam TChar Character type to query.
template <typename TChar>
struct end_line;

template <>
struct end_line<char>
{
    static CASTLE_CONSTEXPR CASTLE_CONST char* value = "\n\r";
};

template <>
struct end_line<wchar_t>
{
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* value = L"\n\r";
};

/// @brief Tag type for in-place construction.
struct in_place_t
{
    /// @brief Constructs the in-place tag.
    explicit CASTLE_CONSTEXPR in_place_t() {}
};

inline CASTLE_CONSTEXPR in_place_t in_place{};

/// @brief Tag type for in-place construction of a specific type.
/// @tparam T Target type.
template <typename T>
struct in_place_type_t
{
    /// @brief Constructs the typed in-place tag.
    explicit CASTLE_CONSTEXPR in_place_type_t() {}
};

template <typename T>
inline CASTLE_CONSTEXPR in_place_type_t<T> in_place_type{};

}

template <typename T, T Value>
using integral_constant = meta::integral_constant<T, Value>;
template <bool Value>
using bool_constant     = meta::bool_constant<Value>;
using true_type  = meta::true_type;
using false_type = meta::false_type;
template <typename T>     using type_identity   = meta::type_identity<T>;
template <typename T>     using type_identity_t = meta::type_identity_t<T>;
template <typename... Ts> using always_false    = meta::always_false<Ts...>;
template <typename T>     using dependent_false = meta::dependent_false<T>;

template <typename... Ts> using conjunction = meta::conjunction<Ts...>;
template <typename... Ts> using disjunction = meta::disjunction<Ts...>;
template <typename T>     using negation    = meta::negation<T>;

template <typename T> using remove_reference        = meta::remove_reference<T>;
template <typename T> using remove_reference_t      = meta::remove_reference_t<T>;
template <typename T> using is_reference            = meta::is_reference<T>;
template <typename T> using is_lvalue_reference     = meta::is_reference<T&>;
template <typename T> using is_rvalue_reference     = meta::is_rvalue_reference<T>;

template <typename T> using remove_cv_t   = meta::remove_cv_t<T>;
template <typename T> using add_const     = meta::add_const<T>;
template <typename T> using add_const_t   = meta::add_const_t<T>;
template <typename T> using add_volatile  = meta::add_volatile<T>;
template <typename T> using add_volatile_t = meta::add_volatile_t<T>;
template <typename T> using add_cv        = meta::add_cv<T>;
template <typename T> using add_cv_t      = meta::add_cv_t<T>;
template <typename T> using is_const      = meta::is_const<T>;

template <typename T> using is_void                    = meta::is_void<T>;
template <typename T> using is_null_pointer            = meta::is_null_pointer<T>;
template <typename T> using is_integral                = meta::is_integral<T>;
template <typename T> using is_floating_point          = meta::is_floating_point<T>;
template <typename T> using floating_epsilon           = meta::floating_epsilon<T>;
template <typename T> using is_array                   = meta::is_array<T>;
template <typename T> using is_pointer                 = meta::is_pointer<T>;
template <typename T> using is_enum                    = meta::is_enum<T>;
template <typename T> using is_class                   = meta::is_class<T>;
template <typename T> using is_union                   = meta::is_union<T>;
template <typename T> using is_function                = meta::is_function<T>;
template <typename T> using is_member_pointer          = meta::is_member_pointer<T>;
template <typename T> using is_member_function_pointer = meta::is_member_function_pointer<T>;

template <typename T> using is_arithmetic  = meta::is_arithmetic<T>;
template <typename T> using is_fundamental = meta::is_fundamental<T>;
template <typename T> using is_scalar      = meta::is_scalar<T>;
template <typename T> using is_object      = meta::is_object<T>;
template <typename T> using is_compound    = meta::is_compound<T>;

template <typename T> using is_signed       = meta::is_signed<T>;
template <typename T> using is_unsigned     = meta::is_unsigned<T>;
template <typename T> using make_signed     = meta::make_signed<T>;
template <typename T> using make_signed_t   = meta::make_signed_t<T>;
template <typename T> using make_unsigned   = meta::make_unsigned<T>;
template <typename T> using make_unsigned_t = meta::make_unsigned_t<T>;

template <typename T> using is_trivially_copyable     = meta::is_trivially_copyable<T>;
template <typename T> using is_trivially_destructible = meta::is_trivially_destructible<T>;
template <typename T> using alignment_of              = meta::alignment_of<T>;
using max_align_t = meta::max_align_t;
#if defined(__clang__) || defined(__GNUC__)
template <typename T> using is_empty               = meta::is_empty<T>;
template <typename T> using is_polymorphic         = meta::is_polymorphic<T>;
template <typename T> using is_abstract            = meta::is_abstract<T>;
template <typename T> using is_final               = meta::is_final<T>;
template <typename T> using has_virtual_destructor = meta::has_virtual_destructor<T>;
template <typename T> using is_standard_layout     = meta::is_standard_layout<T>;
template <typename T> using is_trivial             = meta::is_trivial<T>;
#endif

template <typename T, typename... Args>
using is_constructible = meta::is_constructible<T, Args...>;
template <typename T, typename... Args>
using is_nothrow_constructible = meta::is_nothrow_constructible<T, Args...>;
template <typename T> using is_default_constructible      = meta::is_default_constructible<T>;
template <typename T> using is_copy_constructible         = meta::is_copy_constructible<T>;
template <typename T> using is_move_constructible         = meta::is_move_constructible<T>;
template <typename T> using is_nothrow_default_constructible = meta::is_nothrow_default_constructible<T>;
template <typename T> using is_nothrow_copy_constructible = meta::is_nothrow_copy_constructible<T>;
template <typename T> using is_nothrow_move_constructible = meta::is_nothrow_move_constructible<T>;
template <typename T> using is_destructible               = meta::is_destructible<T>;
template <typename T> using is_nothrow_destructible       = meta::is_nothrow_destructible<T>;
template <typename T> using is_copy_assignable            = meta::is_copy_assignable<T>;
template <typename T> using is_move_assignable            = meta::is_move_assignable<T>;

template <typename T, typename U> using is_same           = meta::is_same<T, U>;
template <typename... Ts>         using largest_type      = meta::largest_type<Ts...>;
template <typename B, typename D> using is_base_of        = meta::is_base_of<B, D>;
template <typename F, typename T> using is_convertible    = meta::is_convertible<F, T>;

template <typename T> using remove_pointer       = meta::remove_pointer<T>;
template <typename T> using remove_pointer_t     = meta::remove_pointer_t<T>;
template <typename T> using add_pointer          = meta::add_pointer<T>;
template <typename T> using add_pointer_t        = meta::add_pointer_t<T>;
template <typename T> using remove_extent        = meta::remove_extent<T>;
template <typename T> using remove_extent_t      = meta::remove_extent_t<T>;
template <typename T> using remove_all_extents   = meta::remove_all_extents<T>;
template <typename T> using remove_all_extents_t = meta::remove_all_extents_t<T>;
template <typename T> using decay                = meta::decay<T>;
template <typename T> using decay_t              = meta::decay_t<T>;
template <typename... Ts> using common_type      = meta::common_type<Ts...>;
template <typename... Ts> using common_type_t    = meta::common_type_t<Ts...>;
#if defined(__clang__) || defined(__GNUC__)
template <typename T> using underlying_type   = meta::underlying_type<T>;
template <typename T> using underlying_type_t = meta::underlying_type_t<T>;
#endif

template <typename F, typename... Args> using invoke_result   = meta::invoke_result<F, Args...>;
template <typename F, typename... Args> using invoke_result_t = meta::invoke_result_t<F, Args...>;
template <typename F, typename... Args> using is_invocable    = meta::is_invocable<F, Args...>;
template <typename R, typename F, typename... Args>
using is_invocable_r = meta::is_invocable_r<R, F, Args...>;

template <size_t N>       using is_power_of_two  = meta::is_power_of_two<N>;
template <typename T>     using is_valid_integer = meta::is_valid_integer<T>;
template <typename... Ts> using has_unique_types = meta::has_unique_types<Ts...>;
template <typename T, template <typename...> class Template>
using is_specialization_of = meta::is_specialization_of<T, Template>;

template <typename Required, typename Actual>
using enable_if_at_least = meta::enable_if_at_least<Required, Actual>;
template <typename From, typename To>
using enable_if_convertible = meta::enable_if_convertible<From, To>;
template <typename T, typename U>
using enable_if_same = meta::enable_if_same<T, U>;

template <typename TChar>
using whitespace = meta::whitespace<TChar>;

template <typename TChar>
using end_line = meta::end_line<TChar>;

using in_place_t = meta::in_place_t;
template <typename T> using in_place_type_t = meta::in_place_type_t<T>;

}

#endif
