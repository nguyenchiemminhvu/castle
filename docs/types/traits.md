# `castle/types/traits.h` — Compile-time predicates

**Header:** `castle/types/traits.h`
**Namespace:** `castle::types`

## Purpose

A small collection of `constexpr` type traits used pervasively by the
rest of CASTLE:

- `has_unique_types<Ts...>` — checks that a variadic type list has no
  duplicates. Backs the uniqueness checks in `observer<>`, `visitor<>`,
  and `signal_event<>`.
- `is_valid_integer<T>` — the SFINAE gate used by every function in
  `castle::bit`, `castle::math`, and by `safe_cast`. Restricts to
  integrals that are **not `bool`** and whose `sizeof` is a power of two
  (i.e. `int8_t`, `uint8_t`, ... `int64_t`, `uint64_t`, plus the
  built-in `int`, `long`, etc. that alias them).
- `whitespace<char>` / `whitespace<wchar_t>` — compile-time whitespace
  character sets used by string parsing helpers.

## API

| Trait                                | Result                                   |
| ------------------------------------ | ---------------------------------------- |
| `has_unique_types_v<Ts...>`          | `bool`                                   |
| `is_valid_integer_v<T>`              | `bool`                                   |
| `whitespace_v<TChar>`                | `const TChar*`                           |

## Example

```cpp
#include <castle/types/traits.h>
using namespace castle::types;

static_assert(has_unique_types_v<int, float, char>);
static_assert(!has_unique_types_v<int, int>);
static_assert(is_valid_integer_v<uint32_t>);
static_assert(!is_valid_integer_v<bool>);
static_assert(!is_valid_integer_v<float>);
```

## See also

- `castle/bit/*` and `castle/math/*` — every generic function is
  SFINAE-gated by `is_valid_integer_v`.
- `castle/design_patterns/observer.h`, `visitor.h` — use
  `has_unique_types_v` to reject duplicate parameters.
