# Any

## Overview
`castle::any` holds zero or one value of any copyable type, where the type is picked at runtime from an open set. Typical uses: configuration parameters, message payloads, and opaque user context. Unlike `std::any`, it never allocates. The value always lives in a fixed inline buffer, and a type that does not fit fails to compile; there is no heap fallback. Type identity comes from [`castle::type_info`](type_info.md) instead of RTTI. Copy, move, and destroy go through a per-type constant operation table instead of virtual functions, so each `any` carries only one pointer of bookkeeping.

## Header
`#include "castle/utility/any.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [config.md](../core/config.md), [error_handler.md](../core/error_handler.md), [traits.md](../core/traits.md), [types.md](../core/types.md), [alignment.md](../memory/alignment.md), [construct.md](../memory/construct.md), [destroy.md](../memory/destroy.md), [object.md](../memory/object.md), [forward.md](forward.md), [move.md](move.md), [type_info.md](type_info.md)

## Public API
| API | Description |
|---|---|
| `basic_any<Capacity, Alignment>` | Type-erased container with a `Capacity`-byte inline buffer aligned to `Alignment`. |
| `any` | `basic_any<inplace_storage_reserved, inplace_alignment_default>`: 8 machine words, aligned to `max_align_t`. |
| `basic_any::capacity`, `alignment`, `storage_type` | Compile-time buffer geometry. |
| `basic_any::is_supported_type<T>()` | `constexpr` check: `T` is a non-array, destructible, copy-constructible object type, is not a `basic_any`, and fits the buffer. |
| `basic_any()` | Constructs an empty any. `noexcept`. |
| `basic_any(const basic_any&)`, `basic_any(basic_any&&)` | Copy the value, or relocate it. A moved-from any is always empty. |
| `basic_any(T&& value)` | Stores `decay_t<T>`. Never chosen for `basic_any` or `in_place_type_t` arguments. |
| `basic_any(meta::in_place_type_t<T>, args...)` | Constructs a `T` in place. |
| `operator=(const basic_any&)`, `operator=(basic_any&&)`, `operator=(T&&)` | Replace the value. The new value is built before the old one is destroyed. |
| `emplace<T>(args...)` | Destroys the current value and constructs a `T` in place. Returns `T&`. |
| `reset()` | Destroys the value and leaves the any empty. |
| `swap(other)` | Exchanges values by relocation (three relocations). |
| `has_value()` | Whether a value is stored. |
| `type()` | `const type_info&` of the stored value, or `type_id<void>()` when empty. |
| `any_cast<T>(basic_any*)`, `any_cast<T>(const basic_any*)` | Pointer to the value if it is a `T`, else `nullptr`. Also returns `nullptr` for a null pointer or an empty any. |
| `any_cast<T>(basic_any&)`, `any_cast<T>(const basic_any&)`, `any_cast<T>(basic_any&&)` | Returns the value as `T`, which may be a value (`int`) or a reference (`int&`, `const int&`). A type mismatch triggers `CASTLE_ASSERT`. |
| `swap(basic_any&, basic_any&)` | Non-member swap. |
| `make_any<T, Capacity, Alignment>(args...)` | Returns a `basic_any<Capacity, Alignment>` holding a `T` built in place. Capacity and alignment default to those of `any`. |

## Usage Example
See [`samples/utility/any.cpp`](../../samples/utility/any.cpp).

```cpp
#include "castle/utility/any.hpp"

castle::any parameter(42U);

if (uint32_t* value = castle::any_cast<uint32_t>(&parameter))   // checked access
{
    *value += 1U;
}

parameter = 2.5F;                                               // replaces the uint32_t
float gain = castle::any_cast<float>(parameter);                // asserted access

castle::basic_any<16U, 4U> small = castle::make_any<uint16_t, 16U, 4U>(7U);
bool same = small.type() == castle::type_id<uint16_t>();
```

## Constraints & Notes
- **No heap.** The storage is inline. `sizeof(basic_any<C, A>)` is `C` plus one pointer, rounded up to the larger of `A` and `alignof(void*)`.
- **Compile-time rejection.** Storing a type that is too large, over-aligned, not copy-constructible, an array, or another `basic_any` triggers a `static_assert` with a descriptive message. `any_cast<T>` also fails to compile when `T` could never be stored.
- **Copyable payloads only**, as with `std::any`. Copying an any must always succeed, so move-only types are rejected at compile time rather than failing at runtime.
- **Move = relocation.** Moving move-constructs the value into the target and destroys it in the source. The source is then empty, which is deterministic (unlike the unspecified state left by `std::any`).
- **Assignment safety.** All `operator=` overloads build the replacement first, then destroy the old value. Assigning a value that lives inside the current value (`a = any_cast<Holder&>(a).field`) is therefore well-defined. `emplace` constructs in place for zero extra moves, so its arguments must not refer into the current value.
- **Unchecked mismatch.** The reference and value forms of `any_cast` assert on a type mismatch. If checks are disabled (`NDEBUG` or `CASTLE_NO_CHECKS`), a mismatch is undefined behavior. Use the pointer form when the type is not known.
- **Type identity.** `type()` returns the same `type_info` object as `castle::type_id<T>()`, so identity checks cost one pointer compare. It converts implicitly to `castle::type_index` for use as a map key. See [type_info.md](type_info.md) for uniqueness and stability rules.
- **Cost.** `has_value`, `type`, and pointer `any_cast` are one load and one compare. Copy, relocate, and destroy each cost one indirect call through the type's operation table.
- **Thread safety.** None internal. Concurrent access to one any needs external synchronization.
- **No conversion between sizes.** A `basic_any<16>` cannot be assigned to an `any`, and it is not silently stored as a nested value either. The conversion fails to compile.
