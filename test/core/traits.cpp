#include <gtest/gtest.h>

#include <cfloat>

#include "castle/core/traits.h"

namespace
{

namespace cm = castle::meta;

// ---------------------------------------------------------------------------
// Helper types shared across the trait tests.
// ---------------------------------------------------------------------------
enum PlainEnum { PE_A, PE_B };
enum class ScopedEnum : unsigned short { SE_X, SE_Y };

struct EmptyType {};
struct NonEmptyType { int x; };
struct PolyType { virtual void f() {} virtual ~PolyType() {} };
struct AbstractType { virtual void f() = 0; virtual ~AbstractType() {} };
struct FinalType final {};
union UnionType { int i; float f; };
struct NonTrivialType { NonTrivialType() {} ~NonTrivialType() {} int x; };

struct MoveOnly
{
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
};

struct BaseType {};
struct DerivedType : BaseType {};

struct WithMembers { int data; int method(int); };

template <typename...> struct MyTemplate {};

struct Doubler { int operator()(int x) const { return x * 2; } };

// ===========================================================================
// 1. Foundation
// ===========================================================================
TEST(CoreTraitsTest, IntegralConstant)
{
    using Five = cm::integral_constant<int, 5>;
    static_assert(Five::value == 5, "value");
    static_assert(cm::is_same<Five::value_type, int>::value, "value_type");
    static_assert(cm::is_same<Five::type, Five>::value, "type");

    Five five;
    EXPECT_EQ(five(), 5);
    EXPECT_EQ(static_cast<int>(five), 5);
}

TEST(CoreTraitsTest, BoolConstantsAndVoidT)
{
    static_assert(cm::true_type::value, "true_type");
    static_assert(!cm::false_type::value, "false_type");
    static_assert(cm::bool_constant<true>::value, "bool_constant true");
    static_assert(!cm::bool_constant<false>::value, "bool_constant false");

    static_assert(cm::is_same<cm::void_t<int, double, char>, void>::value, "void_t");
    static_assert(cm::is_same<cm::type_identity_t<int>, int>::value, "type_identity");
    static_assert(!cm::always_false<int>::value, "always_false");
    static_assert(!cm::dependent_false<int>::value, "dependent_false");
}

// ===========================================================================
// 2. Logical operators
// ===========================================================================
TEST(CoreTraitsTest, LogicalOperators)
{
    static_assert(cm::conjunction<>::value, "empty conjunction is true");
    static_assert(cm::conjunction<cm::true_type, cm::true_type>::value, "and true");
    static_assert(!cm::conjunction<cm::true_type, cm::false_type>::value, "and false");

    static_assert(!cm::disjunction<>::value, "empty disjunction is false");
    static_assert(cm::disjunction<cm::false_type, cm::true_type>::value, "or true");
    static_assert(!cm::disjunction<cm::false_type, cm::false_type>::value, "or false");

    static_assert(!cm::negation<cm::true_type>::value, "not true");
    static_assert(cm::negation<cm::false_type>::value, "not false");
}

// ===========================================================================
// 3. Conditional / SFINAE
// ===========================================================================
TEST(CoreTraitsTest, ConditionalAndEnableIf)
{
    static_assert(cm::is_same<cm::conditional_t<true, int, double>, int>::value, "cond true");
    static_assert(cm::is_same<cm::conditional_t<false, int, double>, double>::value, "cond false");

    static_assert(cm::is_same<cm::enable_if_t<true, char>, char>::value, "enable_if true");
    // enable_if<false> has no ::type; verified indirectly via SFINAE helpers below.
    SUCCEED();
}

// ===========================================================================
// 4. Reference qualifiers
// ===========================================================================
TEST(CoreTraitsTest, ReferenceTraits)
{
    static_assert(cm::is_same<cm::remove_reference_t<int&>, int>::value, "rm lref");
    static_assert(cm::is_same<cm::remove_reference_t<int&&>, int>::value, "rm rref");
    static_assert(cm::is_same<cm::remove_reference_t<int>, int>::value, "rm none");

    static_assert(cm::is_same<cm::add_lvalue_reference_t<int>, int&>::value, "add lref");
    static_assert(cm::is_same<cm::add_rvalue_reference_t<int>, int&&>::value, "add rref");

    static_assert(cm::is_reference<int&>::value, "is_reference lref");
    static_assert(cm::is_reference<int&&>::value, "is_reference rref");
    static_assert(!cm::is_reference<int>::value, "is_reference none");

    static_assert(cm::is_lvalue_reference<int&>::value, "is_lvalue lref");
    static_assert(!cm::is_lvalue_reference<int&&>::value, "is_lvalue rref");
    static_assert(!cm::is_lvalue_reference<int>::value, "is_lvalue none");

    static_assert(cm::is_rvalue_reference<int&&>::value, "is_rvalue rref");
    static_assert(!cm::is_rvalue_reference<int&>::value, "is_rvalue lref");
}

// ===========================================================================
// 5. CV qualifiers
// ===========================================================================
TEST(CoreTraitsTest, CvTraits)
{
    static_assert(cm::is_same<cm::remove_const_t<const int>, int>::value, "rm const");
    static_assert(cm::is_same<cm::remove_volatile_t<volatile int>, int>::value, "rm volatile");
    static_assert(cm::is_same<cm::remove_cv_t<const volatile int>, int>::value, "rm cv");

    static_assert(cm::is_same<cm::add_const_t<int>, const int>::value, "add const");
    static_assert(cm::is_same<cm::add_volatile_t<int>, volatile int>::value, "add volatile");
    static_assert(cm::is_same<cm::add_cv_t<int>, const volatile int>::value, "add cv");

    static_assert(cm::is_const<const int>::value, "is_const true");
    static_assert(!cm::is_const<int>::value, "is_const false");
    static_assert(cm::is_volatile<volatile int>::value, "is_volatile true");
    static_assert(!cm::is_volatile<int>::value, "is_volatile false");
}

// ===========================================================================
// 6. Primary categories
// ===========================================================================
TEST(CoreTraitsTest, VoidAndNullPointer)
{
    static_assert(cm::is_void<void>::value, "void");
    static_assert(cm::is_void<const void>::value, "const void");
    static_assert(!cm::is_void<int>::value, "not void");

    static_assert(cm::is_null_pointer<decltype(nullptr)>::value, "nullptr_t");
    static_assert(!cm::is_null_pointer<int*>::value, "not nullptr_t");
}

TEST(CoreTraitsTest, IntegralAndFloatingPoint)
{
    static_assert(cm::is_integral<bool>::value, "bool");
    static_assert(cm::is_integral<char>::value, "char");
    static_assert(cm::is_integral<int>::value, "int");
    static_assert(cm::is_integral<unsigned long long>::value, "ull");
    static_assert(!cm::is_integral<float>::value, "float not int");
    static_assert(!cm::is_integral<int*>::value, "ptr not int");

    static_assert(cm::is_floating_point<float>::value, "float");
    static_assert(cm::is_floating_point<double>::value, "double");
    static_assert(cm::is_floating_point<long double>::value, "long double");
    static_assert(!cm::is_floating_point<int>::value, "int not float");
}

TEST(CoreTraitsTest, FloatingEpsilon)
{
    static_assert(cm::floating_epsilon<float>::value == FLT_EPSILON, "float eps");
    static_assert(cm::floating_epsilon<double>::value == DBL_EPSILON, "double eps");
    EXPECT_FLOAT_EQ(cm::floating_epsilon<float>::value, FLT_EPSILON);
    EXPECT_DOUBLE_EQ(cm::floating_epsilon<double>::value, DBL_EPSILON);

    cm::floating_epsilon<float> e;
    EXPECT_FLOAT_EQ(e(), FLT_EPSILON);
    EXPECT_FLOAT_EQ(static_cast<float>(e), FLT_EPSILON);
}

TEST(CoreTraitsTest, ArrayPointerEnumClassUnion)
{
    static_assert(cm::is_array<int[]>::value, "unbounded array");
    static_assert(cm::is_array<int[5]>::value, "bounded array");
    static_assert(!cm::is_array<int>::value, "not array");

    static_assert(cm::is_pointer<int*>::value, "pointer");
    static_assert(!cm::is_pointer<int>::value, "not pointer");
    static_assert(!cm::is_pointer<int WithMembers::*>::value, "member ptr is not pointer");

    static_assert(cm::is_enum<PlainEnum>::value, "plain enum");
    static_assert(cm::is_enum<ScopedEnum>::value, "scoped enum");
    static_assert(!cm::is_enum<int>::value, "int not enum");

    static_assert(cm::is_class<NonEmptyType>::value, "class");
    static_assert(!cm::is_class<int>::value, "int not class");
    static_assert(!cm::is_class<UnionType>::value, "union not class");

    static_assert(cm::is_union<UnionType>::value, "union");
    static_assert(!cm::is_union<NonEmptyType>::value, "class not union");
}

TEST(CoreTraitsTest, FunctionAndMemberPointerTraits)
{
    static_assert(cm::is_function<int(int)>::value, "function");
    static_assert(cm::is_function<void()>::value, "void function");
    static_assert(!cm::is_function<int (*)(int)>::value, "function pointer is not function");

    static_assert(cm::is_member_pointer<int WithMembers::*>::value, "data member ptr");
    static_assert(cm::is_member_pointer<int (WithMembers::*)(int)>::value, "member fn ptr");
    static_assert(!cm::is_member_pointer<int*>::value, "plain ptr not member ptr");

    static_assert(cm::is_member_function_pointer<int (WithMembers::*)(int)>::value, "mem fn ptr");
    static_assert(!cm::is_member_function_pointer<int WithMembers::*>::value, "data mem ptr");
}

// ===========================================================================
// 7. Composite categories
// ===========================================================================
TEST(CoreTraitsTest, CompositeCategories)
{
    static_assert(cm::is_arithmetic<int>::value, "int arithmetic");
    static_assert(cm::is_arithmetic<double>::value, "double arithmetic");
    static_assert(!cm::is_arithmetic<int*>::value, "ptr not arithmetic");

    static_assert(cm::is_fundamental<int>::value, "int fundamental");
    static_assert(cm::is_fundamental<void>::value, "void fundamental");
    static_assert(cm::is_fundamental<decltype(nullptr)>::value, "nullptr fundamental");
    static_assert(!cm::is_fundamental<int*>::value, "ptr not fundamental");

    static_assert(cm::is_scalar<int>::value, "int scalar");
    static_assert(cm::is_scalar<PlainEnum>::value, "enum scalar");
    static_assert(cm::is_scalar<int*>::value, "ptr scalar");
    static_assert(cm::is_scalar<int WithMembers::*>::value, "member ptr scalar");
    static_assert(!cm::is_scalar<NonEmptyType>::value, "class not scalar");

    static_assert(cm::is_object<int>::value, "int object");
    static_assert(cm::is_object<int*>::value, "ptr object");
    static_assert(!cm::is_object<int(int)>::value, "function not object");
    static_assert(!cm::is_object<int&>::value, "reference not object");
    static_assert(!cm::is_object<void>::value, "void not object");

    static_assert(cm::is_compound<int*>::value, "ptr compound");
    static_assert(cm::is_compound<NonEmptyType>::value, "class compound");
    static_assert(!cm::is_compound<int>::value, "int not compound");
}

// ===========================================================================
// 8. Signedness
// ===========================================================================
TEST(CoreTraitsTest, Signedness)
{
    static_assert(cm::is_signed<int>::value, "int signed");
    static_assert(cm::is_signed<float>::value, "float signed");
    static_assert(!cm::is_signed<unsigned int>::value, "unsigned not signed");
    static_assert(!cm::is_signed<NonEmptyType>::value, "class not signed");

    static_assert(cm::is_unsigned<unsigned int>::value, "unsigned");
    static_assert(!cm::is_unsigned<int>::value, "int not unsigned");
    static_assert(!cm::is_unsigned<float>::value, "float not unsigned");

    static_assert(cm::is_same<cm::make_signed_t<unsigned int>, int>::value, "make_signed");
    static_assert(cm::is_same<cm::make_signed_t<unsigned long>, long>::value, "make_signed long");
    static_assert(cm::is_same<cm::make_unsigned_t<int>, unsigned int>::value, "make_unsigned");
    static_assert(cm::is_same<cm::make_unsigned_t<char>, unsigned char>::value, "make_unsigned char");
}

// ===========================================================================
// 9. Class-shape / triviality
// ===========================================================================
TEST(CoreTraitsTest, ClassShapeTraits)
{
    static_assert(cm::alignment_of<double>::value == alignof(double), "alignment_of");
    static_assert(cm::alignment_of<char>::value == alignof(char), "alignment_of char");

    static_assert(cm::is_empty<EmptyType>::value, "empty");
    static_assert(!cm::is_empty<NonEmptyType>::value, "non-empty");

    static_assert(cm::is_polymorphic<PolyType>::value, "polymorphic");
    static_assert(!cm::is_polymorphic<NonEmptyType>::value, "non-polymorphic");

    static_assert(cm::is_abstract<AbstractType>::value, "abstract");
    static_assert(!cm::is_abstract<PolyType>::value, "concrete polymorphic");

    static_assert(cm::is_final<FinalType>::value, "final");
    static_assert(!cm::is_final<NonEmptyType>::value, "non-final");

    static_assert(cm::has_virtual_destructor<PolyType>::value, "virtual dtor");
    static_assert(!cm::has_virtual_destructor<NonEmptyType>::value, "no virtual dtor");

    static_assert(cm::is_standard_layout<int>::value, "int standard layout");
    static_assert(!cm::is_standard_layout<PolyType>::value, "poly not standard layout");

    static_assert(cm::is_trivial<int>::value, "int trivial");
    static_assert(!cm::is_trivial<NonTrivialType>::value, "not trivial");

    static_assert(cm::is_trivially_copyable<int>::value, "int trivially copyable");
    static_assert(!cm::is_trivially_copyable<NonTrivialType>::value, "not trivially copyable");

    static_assert(cm::is_trivially_destructible<int>::value, "int trivially destructible");
    static_assert(!cm::is_trivially_destructible<NonTrivialType>::value, "not trivially destructible");

    static_assert(alignof(cm::max_align_t) >= alignof(double), "max_align_t alignment");
}

// ===========================================================================
// 10. Supported operations
// ===========================================================================
TEST(CoreTraitsTest, ConstructionTraits)
{
    static_assert(cm::is_constructible<int, int>::value, "int from int");
    static_assert(cm::is_constructible<int>::value, "int default");
    static_assert(!cm::is_constructible<int, int*>::value, "int from ptr");

    static_assert(cm::is_default_constructible<int>::value, "default");
    static_assert(cm::is_default_constructible<MoveOnly>::value, "move-only default");

    static_assert(cm::is_copy_constructible<int>::value, "int copy");
    static_assert(!cm::is_copy_constructible<MoveOnly>::value, "move-only no copy");

    static_assert(cm::is_move_constructible<int>::value, "int move");
    static_assert(cm::is_move_constructible<MoveOnly>::value, "move-only move");

    static_assert(cm::is_nothrow_default_constructible<int>::value, "nothrow default");
    static_assert(cm::is_nothrow_move_constructible<int>::value, "nothrow move");
}

TEST(CoreTraitsTest, DestructionAndAssignmentTraits)
{
    static_assert(cm::is_destructible<int>::value, "int destructible");
    static_assert(cm::is_destructible<NonTrivialType>::value, "non-trivial destructible");
    static_assert(cm::is_nothrow_destructible<int>::value, "int nothrow destructible");

    static_assert(cm::is_copy_assignable<int>::value, "int copy assign");
    static_assert(!cm::is_copy_assignable<MoveOnly>::value, "move-only no copy assign");

    static_assert(cm::is_move_assignable<int>::value, "int move assign");
    static_assert(cm::is_move_assignable<MoveOnly>::value, "move-only move assign");
}

// ===========================================================================
// 11. Relational
// ===========================================================================
TEST(CoreTraitsTest, RelationalTraits)
{
    static_assert(cm::is_same<int, int>::value, "same");
    static_assert(!cm::is_same<int, const int>::value, "cv differs");
    static_assert(!cm::is_same<int, long>::value, "different");

    static_assert(cm::is_same<cm::largest_type<char, int, double>::type, double>::value, "largest type");
    static_assert(cm::largest_type<char, int, double>::size::value == sizeof(double), "largest size");
    static_assert(cm::is_same<cm::largest_type<char>::type, char>::value, "single largest");

    static_assert(cm::is_base_of<BaseType, DerivedType>::value, "base of");
    static_assert(!cm::is_base_of<DerivedType, BaseType>::value, "not base of");
    static_assert(cm::is_base_of<BaseType, BaseType>::value, "self base of");

    static_assert(cm::is_convertible<int, double>::value, "int to double");
    static_assert(cm::is_convertible<int*, void*>::value, "ptr to void*");
    static_assert(cm::is_convertible<DerivedType*, BaseType*>::value, "derived* to base*");
    static_assert(!cm::is_convertible<BaseType*, DerivedType*>::value, "base* to derived*");
    static_assert(!cm::is_convertible<int, int*>::value, "int to ptr");
    static_assert(cm::is_convertible<void, void>::value, "void to void");
    static_assert(!cm::is_convertible<NonEmptyType, int>::value, "class to int");
}

// ===========================================================================
// 12. Transformations
// ===========================================================================
TEST(CoreTraitsTest, PointerAndExtentTransforms)
{
    static_assert(cm::is_same<cm::remove_pointer_t<int*>, int>::value, "rm ptr");
    static_assert(cm::is_same<cm::remove_pointer_t<int* const>, int>::value, "rm const ptr");
    static_assert(cm::is_same<cm::remove_pointer_t<int>, int>::value, "rm ptr none");

    static_assert(cm::is_same<cm::add_pointer_t<int>, int*>::value, "add ptr");
    static_assert(cm::is_same<cm::add_pointer_t<int&>, int*>::value, "add ptr strips ref");

    static_assert(cm::is_same<cm::remove_extent_t<int[5]>, int>::value, "rm extent");
    static_assert(cm::is_same<cm::remove_extent_t<int[2][3]>, int[3]>::value, "rm one extent");
    static_assert(cm::is_same<cm::remove_all_extents_t<int[2][3]>, int>::value, "rm all extents");
}

TEST(CoreTraitsTest, DecayCommonTypeUnderlying)
{
    static_assert(cm::is_same<cm::decay_t<int&>, int>::value, "decay ref");
    static_assert(cm::is_same<cm::decay_t<const int&>, int>::value, "decay const ref");
    static_assert(cm::is_same<cm::decay_t<int[5]>, int*>::value, "decay array");
    static_assert(cm::is_same<cm::decay_t<int(int)>, int (*)(int)>::value, "decay function");

    static_assert(cm::is_same<cm::common_type_t<int, double>, double>::value, "common int/double");
    static_assert(cm::is_same<cm::common_type_t<int, int>, int>::value, "common int/int");
    static_assert(cm::is_same<cm::common_type_t<char, short, int>, int>::value, "common promotion");

    static_assert(cm::is_same<cm::underlying_type_t<ScopedEnum>, unsigned short>::value,
                  "scoped underlying");
    static_assert(cm::is_integral<cm::underlying_type_t<PlainEnum>>::value,
                  "plain underlying is integral");
}

// ===========================================================================
// 13. Callable traits
// ===========================================================================
TEST(CoreTraitsTest, CallableTraits)
{
    static_assert(cm::is_same<cm::invoke_result_t<Doubler, int>, int>::value, "functor result");
    static_assert(cm::is_same<cm::invoke_result_t<int (*)(int), int>, int>::value, "fn ptr result");

    static_assert(cm::is_invocable<Doubler, int>::value, "invocable");
    static_assert(!cm::is_invocable<Doubler, int, int>::value, "wrong arity");
    static_assert(!cm::is_invocable<int, int>::value, "int not invocable");
    static_assert(cm::is_invocable<int (*)(int), int>::value, "fn ptr invocable");

    static_assert(cm::is_invocable_r<long, Doubler, int>::value, "result convertible");
    static_assert(!cm::is_invocable_r<int*, Doubler, int>::value, "result not convertible");

    Doubler d;
    EXPECT_EQ(d(21), 42);
}

// ===========================================================================
// 14. castle-specific traits
// ===========================================================================
TEST(CoreTraitsTest, PowerOfTwoAndValidInteger)
{
    static_assert(cm::is_power_of_two<1>::value, "1");
    static_assert(cm::is_power_of_two<2>::value, "2");
    static_assert(cm::is_power_of_two<8>::value, "8");
    static_assert(!cm::is_power_of_two<0>::value, "0");
    static_assert(!cm::is_power_of_two<6>::value, "6");

    static_assert(cm::is_valid_integer<int>::value, "int valid");
    static_assert(cm::is_valid_integer<long long>::value, "ll valid");
    static_assert(!cm::is_valid_integer<bool>::value, "bool invalid");
    static_assert(!cm::is_valid_integer<double>::value, "double invalid");
    static_assert(cm::is_valid_integer<int>::is_int, "int is_int flag");
    static_assert(cm::is_valid_integer<int>::is_size_power_of_2, "int size pow2 flag");
}

TEST(CoreTraitsTest, PackContainsUniqueTypesSpecialization)
{
    static_assert(cm::pack_contains<int, char, int, double>::value, "contains");
    static_assert(!cm::pack_contains<float, char, int, double>::value, "not contains");
    static_assert(!cm::pack_contains<int>::value, "empty pack");

    static_assert(cm::has_unique_types<>::value, "empty unique");
    static_assert(cm::has_unique_types<int>::value, "single unique");
    static_assert(cm::has_unique_types<int, char, double>::value, "all unique");
    static_assert(!cm::has_unique_types<int, int>::value, "duplicate pair");
    static_assert(!cm::has_unique_types<int, char, int>::value, "duplicate spread");

    static_assert(cm::is_specialization_of<MyTemplate<int, char>, MyTemplate>::value, "spec");
    static_assert(!cm::is_specialization_of<int, MyTemplate>::value, "int not spec");
    static_assert(!cm::is_specialization_of<NonEmptyType, MyTemplate>::value, "class not spec");
}

TEST(CoreTraitsTest, EnableIfConvenienceAliases)
{
    static_assert(cm::is_same<cm::enable_if_same<int, int>, int>::value, "enable_if_same");
    static_assert(cm::is_same<cm::enable_if_convertible<int, double>, int>::value,
                  "enable_if_convertible");
    static_assert(cm::is_same<cm::enable_if_at_least<BaseType, DerivedType>, int>::value,
                  "enable_if_at_least");
    SUCCEED();
}

TEST(CoreTraitsTest, WhitespaceAndInPlaceTags)
{
    EXPECT_STREQ(cm::whitespace<char>::value, " \t\n\r\f\v");

    static_assert(cm::is_same<decltype(cm::in_place), const cm::in_place_t>::value, "in_place tag");
    static_assert(cm::is_same<decltype(cm::in_place_type<int>),
                              const cm::in_place_type_t<int>>::value,
                  "in_place_type tag");

    cm::in_place_t tag = cm::in_place;
    (void)tag;
    SUCCEED();
}

// ===========================================================================
// 15. Public aliases re-exported into namespace castle
// ===========================================================================
TEST(CoreTraitsTest, PublicAliasesForwardToMeta)
{
    static_assert(castle::is_integral<int>::value, "public is_integral");
    static_assert(!castle::is_integral<double>::value, "public is_integral false");
    static_assert(castle::is_floating_point<double>::value, "public is_floating_point");
    static_assert(castle::is_same<int, int>::value, "public is_same");
    static_assert(castle::is_void<void>::value, "public is_void");
    static_assert(castle::true_type::value, "public true_type");
    static_assert(!castle::false_type::value, "public false_type");

    static_assert(castle::is_same<castle::decay_t<const int&>, int>::value, "public decay_t");
    static_assert(castle::is_same<castle::remove_reference_t<int&>, int>::value, "public remove_reference_t");
    static_assert(castle::is_same<castle::common_type_t<int, double>, double>::value, "public common_type_t");

    static_assert(castle::is_base_of<BaseType, DerivedType>::value, "public is_base_of");
    static_assert(castle::is_convertible<int, double>::value, "public is_convertible");
    static_assert(castle::is_power_of_two<16>::value, "public is_power_of_two");
    static_assert(castle::is_rvalue_reference<int&&>::value, "public is_rvalue_reference");
    static_assert(!castle::is_rvalue_reference<int&>::value, "public is_rvalue_reference false");

    static_assert(castle::is_same<castle::in_place_t, cm::in_place_t>::value, "public in_place_t");
    SUCCEED();
}

} // namespace
