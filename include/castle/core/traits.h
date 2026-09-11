#ifndef CASTLE_CORE_TRAITS_H
#define CASTLE_CORE_TRAITS_H

#include "castle/core/compiler.h"

#include <stddef.h>
#include <stdint.h>
#include <float.h>

// ============================================================================
// castle/core/traits.h
// ----------------------------------------------------------------------------
// Minimal, dependency-free C++17 trait foundation for the CASTLE embedded
// template library. Only the traits actually consumed by CASTLE's generic
// machinery are provided; this is intentionally not a clone of <type_traits>.
//
// Traits are organised into cohesive sections:
//
//   1.  Foundation           (integral_constant, bool_constant, void_t, ...)
//   2.  Logical operators    (conjunction, disjunction, negation)
//   3.  Conditional / SFINAE (enable_if, conditional)
//   4.  Reference qualifiers (remove_reference, add_l/rvalue_reference, declval)
//   5.  CV qualifiers        (remove_/add_const, volatile, cv)
//   6.  Primary categories   (is_void, is_integral, is_pointer, ...)
//   7.  Composite categories (is_arithmetic, is_scalar, is_object, ...)
//   8.  Sign traits          (is_signed, is_unsigned, make_signed/unsigned)
//   9.  Class shape traits   (is_empty, is_polymorphic, is_final, ...)
//  10.  Supported operations (is_constructible, is_assignable, ...)
//  11.  Relational traits    (is_base_of, is_convertible, is_same)
//  12.  Transformations      (remove_pointer, decay, common_type, ...)
//  13.  Callable traits      (invoke_result, is_invocable)
//  14.  castle-specific      (has_unique_types, enable_if_at_least, ...)
//  15.  Public aliases       (short names re-exported into namespace castle)
//
// Implementation notes are placed close to the trait that motivates them.
// See castle/memory/alignment.h for aligned_storage and alignment helpers.
// ============================================================================

namespace castle
{
namespace meta
{

// ────────────────────────────────────────────────────────────────────────────
// 1. Foundation
// ────────────────────────────────────────────────────────────────────────────

// integral_constant: canonical compile-time constant wrapper. Every boolean
// trait in this header derives (directly or transitively) from it.
template <typename T, T Value>
struct integral_constant
{
    static CASTLE_CONSTEXPR T value = Value;
    using value_type = T;
    using type = integral_constant<T, Value>;

    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

// bool_constant / true_type / false_type: shorthand for boolean traits.
template <bool Value>
using bool_constant = integral_constant<bool, Value>;
using true_type  = bool_constant<true>;
using false_type = bool_constant<false>;

// voider / void_t: SFINAE workhorse. Turns any well-formed type pack into
// void so it can gate a partial specialization on expression validity.
template <typename...>
struct voider { using type = void; };

template <typename... Ts>
using void_t = typename voider<Ts...>::type;

// type_identity: blocks template argument deduction. Useful when a generic
// API must take a "target" type explicitly rather than deduce it.
template <typename T> struct type_identity { using type = T; };
template <typename T> using type_identity_t = typename type_identity<T>::type;

// always_false / dependent_false: canonical trick for producing dependent
// static_assert failures inside if-CASTLE_CONSTEXPR branches or fallback templates.
template <typename...> struct always_false    : false_type {};
template <typename T>  struct dependent_false : false_type {};

// ────────────────────────────────────────────────────────────────────────────
// 2. Logical operators over trait packs
// ────────────────────────────────────────────────────────────────────────────

// conjunction: short-circuit logical AND over a pack of trait types.
template <typename... Ts>
struct conjunction : true_type {};

template <typename T>
struct conjunction<T> : T {};

// disjunction: short-circuit logical OR over a pack of trait types.
template <typename... Ts>
struct disjunction : false_type {};

template <typename T>
struct disjunction<T> : T {};

// negation: logical NOT of a single trait's ::value.
template <typename T>
struct negation : bool_constant<!bool(T::value)> {};

// ────────────────────────────────────────────────────────────────────────────
// 3. Conditional selection / SFINAE gate
// ────────────────────────────────────────────────────────────────────────────

// enable_if: SFINAE primitive. Only defines ::type when Condition is true.
template <bool Condition, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> { using type = T; };

template <bool Condition, typename T = void>
using enable_if_t = typename enable_if<Condition, T>::type;

// conditional: compile-time ternary on types.
template <bool Condition, typename T, typename F>
struct conditional { using type = F; };

template <typename T, typename F>
struct conditional<true, T, F> { using type = T; };

template <bool Condition, typename T, typename F>
using conditional_t = typename conditional<Condition, T, F>::type;

// Short-circuiting recursive definitions of conjunction/disjunction rely on
// conditional_t, so their inductive cases live here.
template <typename T, typename... Ts>
struct conjunction<T, Ts...>
    : conditional_t<bool(T::value), conjunction<Ts...>, T> {};

template <typename T, typename... Ts>
struct disjunction<T, Ts...>
    : conditional_t<bool(T::value), T, disjunction<Ts...>> {};

// ────────────────────────────────────────────────────────────────────────────
// 4. Reference qualifiers
// ────────────────────────────────────────────────────────────────────────────

// remove_reference: strips T& and T&& down to T.
template <typename T> struct remove_reference       { using type = T; };
template <typename T> struct remove_reference<T&>   { using type = T; };
template <typename T> struct remove_reference<T&&>  { using type = T; };
template <typename T> using  remove_reference_t = typename remove_reference<T>::type;

// add_lvalue_reference / add_rvalue_reference: reference-forming helpers.
template <typename T> struct add_lvalue_reference { using type = T&; };
template <typename T> struct add_rvalue_reference { using type = T&&; };
template <typename T> using  add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
template <typename T> using  add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

// Reference-category tests.
template <typename T> struct is_reference        : false_type {};
template <typename T> struct is_reference<T&>    : true_type  {};
template <typename T> struct is_reference<T&&>   : true_type  {};

template <typename T> struct is_lvalue_reference       : false_type {};
template <typename T> struct is_lvalue_reference<T&>   : true_type  {};

template <typename T> struct is_rvalue_reference       : false_type {};
template <typename T> struct is_rvalue_reference<T&&>  : true_type  {};

// declval: unevaluated rvalue-of-T producer. Never actually called; used
// only inside decltype / CASTLE_NOEXCEPT / sizeof contexts.
template <typename T>
add_rvalue_reference_t<T> declval() CASTLE_NOEXCEPT;

// ────────────────────────────────────────────────────────────────────────────
// 5. CV qualifier removal / addition / detection
// ────────────────────────────────────────────────────────────────────────────

template <typename T> struct remove_const              { using type = T; };
template <typename T> struct remove_const<CASTLE_CONST T>     { using type = T; };

template <typename T> struct remove_volatile           { using type = T; };
template <typename T> struct remove_volatile<CASTLE_VOLATILE T> { using type = T; };

// remove_cv: composition of remove_const and remove_volatile.
template <typename T>
struct remove_cv
{
    using type = typename remove_const<typename remove_volatile<T>::type>::type;
};

template <typename T> using remove_const_t    = typename remove_const<T>::type;
template <typename T> using remove_volatile_t = typename remove_volatile<T>::type;
template <typename T> using remove_cv_t       = typename remove_cv<T>::type;

template <typename T> struct add_const    { using type = CASTLE_CONST T; };
template <typename T> struct add_volatile { using type = CASTLE_VOLATILE T; };
template <typename T> struct add_cv       { using type = CASTLE_CONST CASTLE_VOLATILE T; };
template <typename T> using  add_const_t    = typename add_const<T>::type;
template <typename T> using  add_volatile_t = typename add_volatile<T>::type;
template <typename T> using  add_cv_t       = typename add_cv<T>::type;

template <typename T> struct is_const              : false_type {};
template <typename T> struct is_const<CASTLE_CONST T>     : true_type  {};

template <typename T> struct is_volatile              : false_type {};
template <typename T> struct is_volatile<CASTLE_VOLATILE T>  : true_type  {};

// ────────────────────────────────────────────────────────────────────────────
// 6. Primary type categories
// ────────────────────────────────────────────────────────────────────────────

// is_void: T (with cv stripped) is void.
template <typename T> struct is_void_impl               : false_type {};
template <>           struct is_void_impl<void>         : true_type  {};
template <typename T> struct is_void : is_void_impl<remove_cv_t<T>> {};

// is_null_pointer: matches std::nullptr_t and its cv variants.
template <typename T> struct is_null_pointer_impl                    : false_type {};
template <>           struct is_null_pointer_impl<decltype(nullptr)> : true_type  {};
template <typename T> struct is_null_pointer : is_null_pointer_impl<remove_cv_t<T>> {};

// is_integral: enumerated over the standard integer types. bool and the
// character types are intentionally included, matching <type_traits>.
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

// is_floating_point: enumerated for the three standard floating-point types.
template <typename T> struct is_floating_point              : false_type {};
template <>           struct is_floating_point<float>       : true_type  {};
template <>           struct is_floating_point<double>      : true_type  {};
template <>           struct is_floating_point<long double> : true_type  {};

// floating_epsilon: returns the smallest representable difference between 1.0
// and the next representable value of T. Only defined for floating-point types.
template <typename T> struct floating_epsilon;

template <> struct floating_epsilon<float>
{
    using value_type = float;
    static CASTLE_CONSTEXPR float value = __FLT_EPSILON__;
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

template <> struct floating_epsilon<double>
{
    using value_type = double;
    static CASTLE_CONSTEXPR double value = __DBL_EPSILON__;
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

template <> struct floating_epsilon<long double>
{
    using value_type = long double;
    static CASTLE_CONSTEXPR long double value = __LDBL_EPSILON__;
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
    CASTLE_CONSTEXPR value_type operator()() CASTLE_CONST CASTLE_NOEXCEPT { return value; }
};

// is_array: fires on both known and unknown bound array specializations.
template <typename T>           struct is_array        : false_type {};
template <typename T>           struct is_array<T[]>   : true_type  {};
template <typename T, size_t N> struct is_array<T[N]>  : true_type  {};

// is_pointer: T is object/function pointer (not member pointer).
template <typename T> struct is_pointer      : false_type {};
template <typename T> struct is_pointer<T*>  : true_type  {};

// is_enum / is_class / is_union: rely on compiler builtins because there is
// no portable non-builtin implementation.
template <typename T>
struct is_enum
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_enum(T)>
#else
    : false_type
#endif
{};

template <typename T>
struct is_class
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_class(T)>
#else
    : false_type
#endif
{};

template <typename T>
struct is_union
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_union(T)> {};
#else
    : false_type {};
#endif

// is_function: matches every free-function signature form, including the
// C-variadic, cv-, ref-, and CASTLE_NOEXCEPT-qualified variants.
template <typename T> struct is_function : false_type {};
template <typename R, typename... Args> struct is_function<R(Args...)>                : true_type {};
template <typename R, typename... Args> struct is_function<R(Args......)>             : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_CONST>          : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_VOLATILE>       : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_CONST CASTLE_VOLATILE> : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) &>              : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) &&>             : true_type {};
template <typename R, typename... Args> struct is_function<R(Args...) CASTLE_NOEXCEPT>       : true_type {};

// is_member_pointer: any T C::*, cv-qualified or not.
template <typename T>              struct is_member_pointer_impl        : false_type {};
template <typename T, typename C>  struct is_member_pointer_impl<T C::*> : true_type {};
template <typename T>
struct is_member_pointer : is_member_pointer_impl<typename remove_cv<T>::type> {};

// is_member_function_pointer: member pointer whose pointee is a function.
template <typename T> struct is_member_function_pointer_impl : false_type {};
template <typename T, typename C>
struct is_member_function_pointer_impl<T C::*> : bool_constant<is_function<T>::value> {};
template <typename T>
struct is_member_function_pointer
    : is_member_function_pointer_impl<typename remove_cv<T>::type> {};

// ────────────────────────────────────────────────────────────────────────────
// 7. Composite type categories
// ────────────────────────────────────────────────────────────────────────────

// is_arithmetic: integral or floating-point.
template <typename T>
struct is_arithmetic
    : bool_constant<is_integral<T>::value || is_floating_point<T>::value> {};

// is_fundamental: arithmetic, void, or nullptr_t.
template <typename T>
struct is_fundamental
    : bool_constant<is_arithmetic<T>::value || is_void<T>::value
                    || is_null_pointer<T>::value> {};

// is_scalar: arithmetic, enum, pointer, member pointer, or nullptr_t.
template <typename T>
struct is_scalar
    : bool_constant<is_arithmetic<T>::value || is_enum<T>::value
                    || is_pointer<T>::value || is_member_pointer<T>::value
                    || is_null_pointer<T>::value> {};

// is_object: anything that is neither a function, reference, nor void.
template <typename T>
struct is_object
    : bool_constant<!is_function<T>::value && !is_reference<T>::value
                    && !is_void<T>::value> {};

// is_compound: everything not covered by is_fundamental.
template <typename T>
struct is_compound : bool_constant<!is_fundamental<T>::value> {};

// ────────────────────────────────────────────────────────────────────────────
// 8. Signedness / signed ↔ unsigned conversion
// ────────────────────────────────────────────────────────────────────────────

// is_signed / is_unsigned: only meaningful on arithmetic types; both report
// false for anything else, matching <type_traits>.
template <typename T, bool = is_arithmetic<remove_cv_t<T>>::value>
struct is_signed_impl : false_type {};
template <typename T>
struct is_signed_impl<T, true> : bool_constant<(T(-1) < T(0))> {};
template <typename T>
struct is_signed : is_signed_impl<remove_cv_t<T>> {};

template <typename T, bool = is_arithmetic<remove_cv_t<T>>::value>
struct is_unsigned_impl : false_type {};
template <typename T>
struct is_unsigned_impl<T, true> : bool_constant<(T(0) < T(-1))> {};
template <typename T>
struct is_unsigned : is_unsigned_impl<remove_cv_t<T>> {};

// make_signed / make_unsigned: cover the standard integer set used across
// generic APIs. Kept intentionally small — no cv/enum propagation.
template <typename T> struct make_signed                     { using type = T; };
template <> struct make_signed<unsigned char>                { using type = signed char; };
template <> struct make_signed<unsigned short>               { using type = short; };
template <> struct make_signed<unsigned int>                 { using type = int; };
template <> struct make_signed<unsigned long>                { using type = long; };
template <> struct make_signed<unsigned long long>           { using type = long long; };
template <typename T> using make_signed_t = typename make_signed<T>::type;

template <typename T> struct make_unsigned                   { using type = T; };
template <> struct make_unsigned<signed char>                { using type = unsigned char; };
template <> struct make_unsigned<char>                       { using type = unsigned char; };
template <> struct make_unsigned<short>                      { using type = unsigned short; };
template <> struct make_unsigned<int>                        { using type = unsigned int; };
template <> struct make_unsigned<long>                       { using type = unsigned long; };
template <> struct make_unsigned<long long>                  { using type = unsigned long long; };
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

// ────────────────────────────────────────────────────────────────────────────
// 9. Class-shape / layout / triviality traits
// ────────────────────────────────────────────────────────────────────────────

// Builtins are preferred for compiler/ABI-sensitive properties. GCC, Clang,
// and ARM GCC expose these facilities; a single wrapper header makes a
// future compiler-specific fallback possible without touching containers.
template <typename T>
struct is_trivially_copyable
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_trivially_copyable(T)>
#else
    : false_type
#endif
{};

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

// alignment_of: wraps the alignof() operator in the familiar trait shape so
// generic code can query alignment uniformly.
template <typename T>
struct alignment_of : integral_constant<size_t, alignof(T)> {};

struct max_align_t
{
    alignas(long double) unsigned char data[sizeof(long double)];
};

// ────────────────────────────────────────────────────────────────────────────
// 10. Supported operations (construct / destruct / assign)
// ────────────────────────────────────────────────────────────────────────────

// __is_constructible / __is_nothrow_constructible are Clang builtins and
// only landed in GCC 8. Detect them at preprocess time and fall back to a
// SFINAE implementation based on T(declval<Args>()...) otherwise. The SFINAE
// path is portable to any compliant C++11 compiler.
#if defined(__clang__) && defined(__has_builtin)
    #if __has_builtin(__is_constructible)
        #define CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE 1
    #endif
#elif defined(__GNUC__) && (__GNUC__ >= 8)
    #define CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE 1
#endif

#if defined(CASTLE_HAS_BUILTIN_IS_CONSTRUCTIBLE)

template <typename T, typename... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};

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

// Common construction-flavour aliases.
template <typename T> struct is_default_constructible        : is_constructible<T> {};
template <typename T> struct is_copy_constructible           : is_constructible<T, add_lvalue_reference_t<CASTLE_CONST T>> {};
template <typename T> struct is_move_constructible           : is_constructible<T, add_rvalue_reference_t<T>> {};
template <typename T> struct is_nothrow_default_constructible : is_nothrow_constructible<T> {};
template <typename T> struct is_nothrow_copy_constructible   : is_nothrow_constructible<T, add_lvalue_reference_t<CASTLE_CONST T>> {};
template <typename T> struct is_nothrow_move_constructible   : is_nothrow_constructible<T, add_rvalue_reference_t<T>> {};

// is_destructible: for trivially destructible types the answer is trivially
// yes; otherwise we check ~T() is a valid expression.
template <typename T, bool = is_trivially_destructible<T>::value>
struct is_destructible_impl;
template <typename T>
struct is_destructible_impl<T, true>  : true_type {};
template <typename T>
struct is_destructible_impl<T, false>
    : bool_constant<decltype(declval<T&>().~T(), true_type{})::value> {};
template <typename T>
struct is_destructible : is_destructible_impl<T> {};

template <typename T, bool = is_trivially_destructible<T>::value>
struct is_nothrow_destructible_impl;
template <typename T>
struct is_nothrow_destructible_impl<T, true>  : true_type {};
template <typename T>
struct is_nothrow_destructible_impl<T, false>
    : bool_constant<CASTLE_NOEXCEPT(declval<T&>().~T())> {};
template <typename T>
struct is_nothrow_destructible : is_nothrow_destructible_impl<T> {};

// __is_assignable is a Clang builtin and only landed in GCC 9. Use SFINAE
// on declval<T&>() = declval<U>() as the portable fallback.
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

// ────────────────────────────────────────────────────────────────────────────
// 11. Relational traits
// ────────────────────────────────────────────────────────────────────────────

// is_same: exact type equality (cv- and ref-sensitive).
template <typename T, typename U> struct is_same       : false_type {};
template <typename T>             struct is_same<T, T> : true_type  {};

// largest: returns the largest type in a pack of types. Useful for
// determining the largest storage requirement for a heterogeneous container.
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

// is_base_of: Base is a (public or accessible) base class of Derived. Relies
// on the compiler builtin because there is no cheap non-builtin implementation.
template <typename Base, typename Derived>
struct is_base_of
#if defined(__clang__) || defined(__GNUC__)
    : bool_constant<__is_base_of(Base, Derived)> {};
#else
    : false_type {};
#endif

// is_convertible: implicit-conversion probe using the classic
// static_cast<void(*)(To)>(nullptr)(declval<From>()) trick.
template <typename From, typename To, typename = void>
struct is_convertible_impl : false_type {};

template <typename From, typename To>
struct is_convertible_impl<
    From, To,
    void_t<decltype(static_cast<To(*)()>(nullptr),
                    static_cast<void(*)(To)>(nullptr)(declval<From>()))>>
    : true_type {};

// void → void is convertible; all other void combinations are not.
template <typename From, typename To>
struct is_convertible
    : conditional_t<is_void<From>::value && is_void<To>::value,
                    true_type,
                    is_convertible_impl<From, To>> {};

// ────────────────────────────────────────────────────────────────────────────
// 12. Type transformations
// ────────────────────────────────────────────────────────────────────────────

// Pointer transformations.
template <typename T> struct remove_pointer                    { using type = T; };
template <typename T> struct remove_pointer<T*>                { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_CONST>          { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_VOLATILE>       { using type = T; };
template <typename T> struct remove_pointer<T* CASTLE_CONST CASTLE_VOLATILE> { using type = T; };
template <typename T> using  remove_pointer_t = typename remove_pointer<T>::type;

// add_pointer: strips references first (matches <type_traits>). For function
// types that cannot form a pointer we fall back to T unchanged via SFINAE.
template <typename T, typename = void>
struct add_pointer_impl { using type = T; };
template <typename T>
struct add_pointer_impl<T, void_t<remove_reference_t<T>*>>
{ using type = remove_reference_t<T>*; };
template <typename T> struct add_pointer : add_pointer_impl<T> {};
template <typename T> using  add_pointer_t = typename add_pointer<T>::type;

// Array-extent removal.
template <typename T>           struct remove_extent          { using type = T; };
template <typename T>           struct remove_extent<T[]>     { using type = T; };
template <typename T, size_t N> struct remove_extent<T[N]>    { using type = T; };
template <typename T> using remove_extent_t = typename remove_extent<T>::type;

template <typename T>           struct remove_all_extents        { using type = T; };
template <typename T>           struct remove_all_extents<T[]>   { using type = typename remove_all_extents<T>::type; };
template <typename T, size_t N> struct remove_all_extents<T[N]>  { using type = typename remove_all_extents<T>::type; };
template <typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

// decay: array-to-pointer, function-to-pointer, cv/ref stripping. Staple for
// storing forwarded arguments in generic containers/callbacks.
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

// common_type: minimal two-argument form (with variadic fold) sufficient for
// arithmetic promotion in generic algorithms.
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

// underlying_type: exposes the fixed underlying integer of a scoped/unscoped
// enum. Requires a compiler builtin.
#if defined(__clang__) || defined(__GNUC__)
template <typename T> struct underlying_type { using type = __underlying_type(T); };
template <typename T> using  underlying_type_t = typename underlying_type<T>::type;
#endif

// ────────────────────────────────────────────────────────────────────────────
// 13. Callable traits
// ────────────────────────────────────────────────────────────────────────────
// Restricted to plain callables (function pointers and objects with
// operator()). Member-pointer INVOKE semantics are intentionally not
// supported to keep this small; wrap in a lambda at the call site.

template <typename F, typename... Args>
struct invoke_result
{
    using type = decltype(declval<F>()(declval<Args>()...));
};
template <typename F, typename... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

// is_invocable: SFINAE probe on F(Args...).
template <typename Void, typename F, typename... Args>
struct is_invocable_impl : false_type {};

template <typename F, typename... Args>
struct is_invocable_impl<void_t<decltype(declval<F>()(declval<Args>()...))>,
                         F, Args...> : true_type {};

template <typename F, typename... Args>
struct is_invocable : is_invocable_impl<void, F, Args...> {};

template <typename R, typename F, typename... Args>
struct is_invocable_r
    : bool_constant<is_invocable<F, Args...>::value
   && is_convertible<invoke_result_t<F, Args...>, R>::value> {};

// ────────────────────────────────────────────────────────────────────────────
// 14. castle-specific traits — carried over and extended from castle 1.0
// ────────────────────────────────────────────────────────────────────────────

template <size_t N>
struct is_power_of_two
{
    static CASTLE_CONSTEXPR bool value = ((N > 0) && (N & (N - 1)) == 0);
};

// is_valid_integer<T>: T is an integer (not bool) whose size is a power of 2.
// Handy for compile-time gating of register/word-sized generic containers.
template <typename T>
struct is_valid_integer
{
    static CASTLE_CONSTEXPR bool is_int = is_integral<T>::value && !is_same<T, bool>::value;
    static CASTLE_CONSTEXPR bool is_size_power_of_2 = is_power_of_two<sizeof(T)>::value;
    static CASTLE_CONSTEXPR bool value = is_int && is_size_power_of_2;
};

// pack_contains<T, Us...>: true when T appears at least once in Us...
template <typename T, typename... Us>
struct pack_contains : false_type {};

template <typename T, typename U, typename... Us>
struct pack_contains<T, U, Us...>
    : conditional_t<is_same<T, U>::value, true_type, pack_contains<T, Us...>> {};

// has_unique_types<Ts...>: static_assert-friendly duplicate-type detector
// used at registration time by variant-/tuple-like machinery.
template <typename... Ts> struct has_unique_types;
template <>               struct has_unique_types<>  : true_type {};
template <typename T>     struct has_unique_types<T> : true_type {};
template <typename Head, typename... Tail>
struct has_unique_types<Head, Tail...>
    : bool_constant<!pack_contains<Head, Tail...>::value
                    && has_unique_types<Tail...>::value> {};

// is_specialization_of<T, Template>: detects whether T is Template<Args...>.
// Useful for tagging tuple-, variant-, span-like types in generic code.
template <typename T, template <typename...> class Template>
struct is_specialization_of : false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : true_type {};

// SFINAE convenience aliases — the castle 1.0 idiom of enabling overloads
// gated on a relationship between two types.
template <typename Required, typename Actual>
using enable_if_at_least =
    typename enable_if<is_base_of<Required, Actual>::value, int>::type;

template <typename From, typename To>
using enable_if_convertible =
    typename enable_if<is_convertible<From, To>::value, int>::type;

template <typename T, typename U>
using enable_if_same =
    typename enable_if<is_same<T, U>::value, int>::type;

// whitespace<TChar>: compile-time whitespace character set. Consumed by
// string trim/tokenise algorithms so they stay character-type-agnostic.
template <typename TChar>
struct whitespace;

template <>
struct whitespace<char>
{
    static CASTLE_CONSTEXPR CASTLE_CONST char* value = " \t\n\r\f\v";
};

template <>
struct whitespace<wchar_t>
{
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* value = L" \t\n\r\f\v";
};

// in_place_t / in_place_type_t: tag types for in-place construction of
// objects in generic containers. Matches the C++17 std::in_place_t and
// std::in_place_type_t names, but is defined in the castle::meta namespace to
// avoid collisions with std::in_place_t in C++17 and later.

struct in_place_t
{
    explicit CASTLE_CONSTEXPR in_place_t() {}
};

inline CASTLE_CONSTEXPR in_place_t in_place{};

template <typename T>
struct in_place_type_t
{
    explicit CASTLE_CONSTEXPR in_place_type_t() {}
};

template <typename T>
inline CASTLE_CONSTEXPR in_place_type_t<T> in_place_type{};

} // namespace meta

// ============================================================================
// 15. Public aliases
// ----------------------------------------------------------------------------
// User-facing code gets short names in the top-level castle namespace while
// the implementations stay namespaced under castle::meta. Ordering here
// mirrors the sections above.
// ============================================================================

// --- Foundation -------------------------------------------------------------
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

// --- Logical operators ------------------------------------------------------
template <typename... Ts> using conjunction = meta::conjunction<Ts...>;
template <typename... Ts> using disjunction = meta::disjunction<Ts...>;
template <typename T>     using negation    = meta::negation<T>;

// --- Reference qualifiers ---------------------------------------------------
template <typename T> using remove_reference        = meta::remove_reference<T>;
template <typename T> using remove_reference_t      = meta::remove_reference_t<T>;
template <typename T> using is_reference            = meta::is_reference<T>;
template <typename T> using is_lvalue_reference     = meta::is_reference<T&>;
template <typename T> using is_rvalue_reference     = meta::is_rvalue_reference<T>;

// --- CV qualifiers ----------------------------------------------------------
template <typename T> using remove_cv_t   = meta::remove_cv_t<T>;
template <typename T> using add_const     = meta::add_const<T>;
template <typename T> using add_const_t   = meta::add_const_t<T>;
template <typename T> using add_volatile  = meta::add_volatile<T>;
template <typename T> using add_volatile_t = meta::add_volatile_t<T>;
template <typename T> using add_cv        = meta::add_cv<T>;
template <typename T> using add_cv_t      = meta::add_cv_t<T>;
template <typename T> using is_const      = meta::is_const<T>;

// --- Primary categories -----------------------------------------------------
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

// --- Composite categories ---------------------------------------------------
template <typename T> using is_arithmetic  = meta::is_arithmetic<T>;
template <typename T> using is_fundamental = meta::is_fundamental<T>;
template <typename T> using is_scalar      = meta::is_scalar<T>;
template <typename T> using is_object      = meta::is_object<T>;
template <typename T> using is_compound    = meta::is_compound<T>;

// --- Signedness -------------------------------------------------------------
template <typename T> using is_signed       = meta::is_signed<T>;
template <typename T> using is_unsigned     = meta::is_unsigned<T>;
template <typename T> using make_signed     = meta::make_signed<T>;
template <typename T> using make_signed_t   = meta::make_signed_t<T>;
template <typename T> using make_unsigned   = meta::make_unsigned<T>;
template <typename T> using make_unsigned_t = meta::make_unsigned_t<T>;

// --- Class-shape / triviality ----------------------------------------------
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

// --- Supported operations ---------------------------------------------------
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

// --- Relational -------------------------------------------------------------
template <typename T, typename U> using is_same           = meta::is_same<T, U>;
template <typename... Ts>         using largest_type      = meta::largest_type<Ts...>;
template <typename B, typename D> using is_base_of        = meta::is_base_of<B, D>;
template <typename F, typename T> using is_convertible    = meta::is_convertible<F, T>;

// --- Transformations --------------------------------------------------------
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

// --- Callables --------------------------------------------------------------
template <typename F, typename... Args> using invoke_result   = meta::invoke_result<F, Args...>;
template <typename F, typename... Args> using invoke_result_t = meta::invoke_result_t<F, Args...>;
template <typename F, typename... Args> using is_invocable    = meta::is_invocable<F, Args...>;
template <typename R, typename F, typename... Args>
using is_invocable_r = meta::is_invocable_r<R, F, Args...>;

// --- castle-specific helpers ------------------------------------------------
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

using in_place_t = meta::in_place_t;
template <typename T> using in_place_type_t = meta::in_place_type_t<T>;

} // namespace castle

#endif // CASTLE_CORE_TRAITS_H
