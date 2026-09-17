# Brace-init-list adapter

## Overview

`castle::container::initializer_list<T>` is the type CASTLE spells whenever an API wants to
accept a braced-init-list, e.g. `vector<int, 4> v = {1, 2, 3};`. Braced-init-lists are compiler
magic: the compiler only ever materializes a temporary backing array for a parameter whose type
is (or resolves to) `std::initializer_list<T>`. CASTLE cannot invent its own hook for that syntax,
so this header instead:

- Uses the compiler-support `<initializer_list>` header when it is available. That header is
  allocation-free and exception-free, so pulling it in does not compromise the "no heap / no
  exceptions" contract.
- Falls back to a hand-written, ABI-matching definition (pointer + count, matching the Itanium
  C++ ABI layout GCC/Clang/ARM Clang expect) for bare-metal toolchains that ship a freestanding
  compiler without that header.
- Adds the free functions and reverse-iterator alias the rest of CASTLE's iterator subsystem
  expects, so an initializer_list can be treated like any other CASTLE range.

Like `std::initializer_list`, it is a lightweight, non-owning view: the elements live in a
compiler-managed temporary array, so a `castle::container::initializer_list<T>` must not be
stored past the full expression (or function call) that produced it.

## Header

```cpp
#include "castle/container/initializer_list.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `initializer_list<T>` | Alias for the compiler-recognized brace-init-list view type. |
| `begin()` / `end()` | Member and free-function access to the backing `const T*` range. |
| `size()` / `empty()` | Element count, as a member and as a free function. |
| `front()` / `back()` | First / last element, as free functions. |
| `rbegin()` / `rend()` | `castle::reverse_iterator<const T*>` view for reverse iteration. |

`initializer_list<T>::iterator` / `const_iterator` are both `const T*`, so it slots directly into
`castle::iterator_traits`, `castle::distance`, `castle::advance`, `castle::next` and
`castle::prev` without any adapter code.

## Example

```cpp
#include "castle/container/initializer_list.hpp"
#include "castle/container/vector.hpp"
#include "castle/container/array_view.hpp"

uint32_t sum(castle::container::initializer_list<uint32_t> samples)
{
    uint32_t total = 0U;
    for (uint32_t value : samples)
    {
        total += value;
    }
    return total;
}

// Any brace-init-list binds to castle::container::initializer_list<T>, exactly
// like it would bind to std::initializer_list<T>.
sum({1U, 2U, 3U, 4U});

// Fixed-capacity containers collaborate with it directly.
castle::container::vector<uint32_t, 4U> gains = {100U, 200U, 300U};
gains = {7U, 8U};

// Bridge to a non-owning view without copying into a container at all.
const auto view = castle::container::make_array_view(
    castle::container::initializer_list<uint32_t>{1U, 2U, 3U, 4U});
```

## Embedded notes

- The backing storage is a compiler-managed temporary; never keep a
  `castle::container::initializer_list<T>` (or an `array_view` built from one) alive past the
  full expression that created it.
- `vector<T, N>`'s initializer_list constructor and assignment operator check the list length
  against `N` with `CASTLE_ASSERT` - an oversized list is a programmer error, not a recoverable
  runtime condition, so it is reported the same way as other CASTLE precondition violations.
- No heap allocation, exceptions or RTTI are involved at any point.
- Template argument deduction for `T` through a *template alias* parameter (e.g.
  `make_array_view(initializer_list<T>)`) from a raw brace literal is not reliably portable
  across compilers; construct a `castle::container::initializer_list<T>` value first (as above)
  when calling such a function template directly.
