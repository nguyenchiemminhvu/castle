# Type Traits

## Overview
This header is Castle's STL-free metaprogramming foundation: it supplies compile-time constants, trait predicates, type transformations, callable detection, and a small set of Castle-specific helpers used throughout the library.

## Header
`#include "castle/core/traits.hpp"`

## Dependencies
- [compiler.md](compiler.md)

## Public API
- **Foundation:** `castle::meta::integral_constant`, `bool_constant`, `true_type`, `false_type`, `void_t`, `type_identity`, `always_false`, `dependent_false`.
- **Logic and SFINAE:** `conjunction`, `disjunction`, `negation`, `enable_if`, `conditional`.
- **Reference and cv transforms:** `remove_reference`, `add_lvalue_reference`, `add_rvalue_reference`, `remove_const`, `remove_volatile`, `remove_cv`, `add_const`, `add_volatile`, `add_cv`, `declval`.
- **Primary and composite categories:** `is_void`, `is_null_pointer`, `is_integral`, `is_floating_point`, `floating_epsilon`, `is_array`, `is_pointer`, `is_enum`, `is_class`, `is_union`, `is_function`, `is_member_pointer`, `is_member_function_pointer`, `is_arithmetic`, `is_fundamental`, `is_scalar`, `is_object`, `is_compound`.
- **Signedness and layout:** `is_signed`, `is_unsigned`, `make_signed`, `make_unsigned`, `is_trivially_copyable`, `is_trivially_destructible`, `alignment_of`, `max_align_t`, and builtin-backed class-shape traits where supported.
- **Construction, conversion, and transforms:** `is_constructible`, `is_nothrow_constructible`, `is_default_constructible`, `is_copy_constructible`, `is_move_constructible`, `is_destructible`, `is_copy_assignable`, `is_move_assignable`, `is_same`, `largest_type`, `is_base_of`, `is_convertible`, `remove_pointer`, `add_pointer`, `remove_extent`, `remove_all_extents`, `decay`, `common_type`, `underlying_type`.
- **Callables and Castle-specific helpers:** `invoke_result`, `is_invocable`, `is_invocable_r`, `is_power_of_two`, `is_valid_integer`, `has_unique_types`, `is_specialization_of`, `enable_if_at_least`, `enable_if_convertible`, `enable_if_same`, `whitespace`, `end_line`, `in_place_t`, `in_place_type_t`, and the top-level alias re-exports in namespace `castle`.

## Usage Example
```cpp
#include "castle/core/traits.hpp"

static_assert(castle::meta::is_integral<unsigned int>::value, "integral");
using raw_t = castle::decay_t<const int&>;
```

See also: `samples/sample_traits.cpp`.

## Constraints & Notes
- Designed to avoid any dependency on `<type_traits>` or other STL metaprogramming utilities.
- Many object-model queries rely on compiler builtins when no small portable implementation exists.
- Most APIs are compile-time only and are meant to be used in templates, `static_assert`s, and SFINAE contexts.
