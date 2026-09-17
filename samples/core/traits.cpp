#include "sample_support.hpp"

#include "castle/core/traits.hpp"

#include <stdint.h>

namespace
{

enum class Mode : uint8_t
{
    off = 0,
    on = 1
};

struct Base {};
struct Derived : Base {};
struct Empty {};
union SampleUnion
{
    int integer;
    float real;
};

struct Callable
{
    int operator()(int value) const CASTLE_NOEXCEPT
    {
        return value + 1;
    }
};

struct MemberHolder
{
    int value;
    int get() const CASTLE_NOEXCEPT
    {
        return value;
    }
};

template <typename... Ts>
struct Box {};

template <typename T, castle::enable_if_convertible<T, int> = 0>
CASTLE_CONSTEXPR int to_int(T value) CASTLE_NOEXCEPT
{
    return static_cast<int>(value);
}

template <typename T, castle::enable_if_same<T, int> = 0>
CASTLE_CONSTEXPR int only_int(T value) CASTLE_NOEXCEPT
{
    return value;
}

template <typename T, castle::enable_if_at_least<Base, T> = 0>
CASTLE_CONSTEXPR bool derives_from_base() CASTLE_NOEXCEPT
{
    return true;
}

} // namespace

static_assert(castle::integral_constant<int, 7>::value == 7, "integral_constant");
static_assert(castle::bool_constant<true>::value, "bool_constant");
static_assert(castle::true_type::value, "true_type");
static_assert(!castle::false_type::value, "false_type");
static_assert(castle::is_same<castle::type_identity_t<int>, int>::value, "type_identity_t");
static_assert(!castle::always_false<int>::value, "always_false");
static_assert(!castle::dependent_false<int>::value, "dependent_false");
static_assert(castle::conjunction<castle::true_type, castle::true_type>::value, "conjunction");
static_assert(castle::disjunction<castle::false_type, castle::true_type>::value, "disjunction");
static_assert(castle::negation<castle::false_type>::value, "negation");
static_assert(castle::is_same<castle::remove_reference_t<int&&>, int>::value, "remove_reference_t");
static_assert(castle::meta::is_reference<int&>::value, "is_reference");
static_assert(castle::meta::is_lvalue_reference<int&>::value, "is_lvalue_reference");
static_assert(castle::meta::is_rvalue_reference<int&&>::value, "is_rvalue_reference");
static_assert(castle::is_same<castle::meta::add_lvalue_reference<int>::type, int&>::value, "add_lvalue_reference");
static_assert(castle::is_same<castle::meta::add_rvalue_reference<int>::type, int&&>::value, "add_rvalue_reference");
static_assert(castle::is_same<castle::meta::remove_const<const int>::type, int>::value, "remove_const");
static_assert(castle::is_same<castle::meta::remove_volatile<volatile int>::type, int>::value, "remove_volatile");
static_assert(castle::is_same<castle::remove_cv_t<const volatile int>, int>::value, "remove_cv_t");
static_assert(castle::is_same<castle::add_const_t<int>, const int>::value, "add_const_t");
static_assert(castle::is_same<castle::add_volatile_t<int>, volatile int>::value, "add_volatile_t");
static_assert(castle::is_same<castle::add_cv_t<int>, const volatile int>::value, "add_cv_t");
static_assert(castle::is_const<const int>::value, "is_const");
static_assert(castle::meta::is_volatile<volatile int>::value, "is_volatile");
static_assert(castle::is_void<void>::value, "is_void");
static_assert(castle::is_null_pointer<decltype(nullptr)>::value, "is_null_pointer");
static_assert(castle::is_integral<unsigned long>::value, "is_integral");
static_assert(castle::is_floating_point<double>::value, "is_floating_point");
static_assert(castle::floating_epsilon<float>::value > 0.0f, "floating_epsilon");
static_assert(castle::is_array<int[2]>::value, "is_array");
static_assert(castle::is_pointer<int*>::value, "is_pointer");
static_assert(castle::is_enum<Mode>::value, "is_enum");
static_assert(castle::is_class<Derived>::value, "is_class");
static_assert(castle::is_union<SampleUnion>::value, "is_union");
static_assert(castle::is_function<int(int)>::value, "is_function");
static_assert(castle::is_member_pointer<int MemberHolder::*>::value, "is_member_pointer");
static_assert(castle::is_member_function_pointer<int (MemberHolder::*)() const>::value, "is_member_function_pointer");
static_assert(castle::is_arithmetic<int>::value, "is_arithmetic");
static_assert(castle::is_fundamental<int>::value, "is_fundamental");
static_assert(castle::is_scalar<int*>::value, "is_scalar");
static_assert(castle::is_object<Derived>::value, "is_object");
static_assert(castle::is_compound<Derived>::value, "is_compound");
static_assert(castle::is_signed<int>::value, "is_signed");
static_assert(castle::is_unsigned<unsigned int>::value, "is_unsigned");
static_assert(castle::is_same<castle::make_signed_t<unsigned short>, short>::value, "make_signed_t");
static_assert(castle::is_same<castle::make_unsigned_t<int>, unsigned int>::value, "make_unsigned_t");
static_assert(castle::is_trivially_copyable<int>::value, "is_trivially_copyable");
static_assert(castle::is_trivially_destructible<Derived>::value, "is_trivially_destructible");
static_assert(castle::alignment_of<Derived>::value >= 1U, "alignment_of");
static_assert(sizeof(castle::max_align_t) >= sizeof(long double), "max_align_t");
#if defined(__clang__) || defined(__GNUC__)
static_assert(castle::is_empty<Empty>::value, "is_empty");
static_assert(!castle::is_polymorphic<Derived>::value, "is_polymorphic");
static_assert(!castle::is_abstract<Derived>::value, "is_abstract");
static_assert(!castle::is_final<Derived>::value, "is_final");
static_assert(!castle::has_virtual_destructor<Derived>::value, "has_virtual_destructor");
static_assert(castle::is_standard_layout<MemberHolder>::value, "is_standard_layout");
static_assert(castle::is_trivial<MemberHolder>::value, "is_trivial");
static_assert(castle::is_same<castle::underlying_type_t<Mode>, uint8_t>::value, "underlying_type_t");
#endif
static_assert(castle::is_default_constructible<Derived>::value, "is_default_constructible");
static_assert(castle::is_copy_constructible<Derived>::value, "is_copy_constructible");
static_assert(castle::is_move_constructible<Derived>::value, "is_move_constructible");
static_assert(castle::is_nothrow_default_constructible<Derived>::value, "is_nothrow_default_constructible");
static_assert(castle::is_nothrow_copy_constructible<Derived>::value, "is_nothrow_copy_constructible");
static_assert(castle::is_nothrow_move_constructible<Derived>::value, "is_nothrow_move_constructible");
static_assert(castle::is_destructible<Derived>::value, "is_destructible");
static_assert(castle::is_nothrow_destructible<Derived>::value, "is_nothrow_destructible");
static_assert(castle::is_copy_assignable<MemberHolder>::value, "is_copy_assignable");
static_assert(castle::is_move_assignable<MemberHolder>::value, "is_move_assignable");
static_assert(castle::is_same<int, int>::value, "is_same");
static_assert(castle::largest_type<char, int, long long>::size::value == sizeof(long long), "largest_type");
static_assert(castle::is_base_of<Base, Derived>::value, "is_base_of");
static_assert(castle::is_convertible<int, long>::value, "is_convertible");
static_assert(castle::is_same<castle::remove_pointer_t<int*>, int>::value, "remove_pointer_t");
static_assert(castle::is_same<castle::add_pointer_t<int>, int*>::value, "add_pointer_t");
static_assert(castle::is_same<castle::remove_extent_t<int[2]>, int>::value, "remove_extent_t");
static_assert(castle::is_same<castle::remove_all_extents_t<int[2][3]>, int>::value, "remove_all_extents_t");
static_assert(castle::is_same<castle::decay_t<const int&>, int>::value, "decay_t");
static_assert(castle::is_same<castle::common_type_t<int, double>, double>::value, "common_type_t");
static_assert(castle::is_same<castle::invoke_result_t<Callable, int>, int>::value, "invoke_result_t");
static_assert(castle::is_invocable<Callable, int>::value, "is_invocable");
static_assert(castle::is_invocable_r<int, Callable, int>::value, "is_invocable_r");
static_assert(castle::is_power_of_two<8>::value, "is_power_of_two");
static_assert(castle::is_valid_integer<uint32_t>::value, "is_valid_integer");
static_assert(castle::has_unique_types<int, char, long>::value, "has_unique_types");
static_assert(!castle::has_unique_types<int, int>::value, "has_unique_types duplicates");
static_assert(castle::is_specialization_of<Box<int>, Box>::value, "is_specialization_of");

int main()
{
    Callable callable;
    const castle::meta::in_place_t tag = castle::meta::in_place;
    (void)tag;
    const castle::meta::in_place_type_t<int> typed_tag = castle::meta::in_place_type<int>;
    (void)typed_tag;

    CASTLE_SAMPLE_CHECK(to_int(9U) == 9);
    CASTLE_SAMPLE_CHECK(only_int(5) == 5);
    CASTLE_SAMPLE_CHECK(derives_from_base<Derived>());
    CASTLE_SAMPLE_CHECK(callable(4) == 5);
    CASTLE_SAMPLE_CHECK(castle::meta::whitespace<char>::value[0] == ' ');
    CASTLE_SAMPLE_CHECK(castle::meta::end_line<char>::value[0] == '\n');
    return 0;
}
