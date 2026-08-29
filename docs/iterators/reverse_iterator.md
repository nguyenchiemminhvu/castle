# `castle/iterators/reverse_iterator.h` — Reverse traversal wrapper

**Header:** `castle/iterators/reverse_iterator.h`
**Namespace:** `castle::iterators`

## Purpose

A wrapper that traverses any bidirectional CASTLE iterator **backwards**,
with the same semantics as `std::reverse_iterator`:

```
rbegin() == reverse_iterator(end())
rend()   == reverse_iterator(begin())
```

## Design notes

- Dereferencing yields the element *preceding* the stored base
  iterator, hence the base must be at least bidirectional.
- Random-access operations (`+=`, `-=`, `+`, `-`, `[]`, ordering,
  distance) are **SFINAE-enabled only when** the underlying iterator's
  category is derived from `random_access_iterator_tag`.
- Ordering relations are flipped naturally: `lhs < rhs` in reverse
  space means `rhs.base() < lhs.base()` in forward space.
- Cross const/non-const `reverse_iterator<iterator> ->
  reverse_iterator<const_iterator>` promotion is enabled.

## API

| Member                    | Description                                 |
| ------------------------- | ------------------------------------------- |
| `reverse_iterator(It)`    | Wraps an underlying base iterator           |
| `base()`                  | Returns the stored base iterator            |
| `*`, `->`                 | Dereference: element *before* `base()`      |
| `++`, `--`                | Advance / retreat                           |
| `+`, `-`, `+=`, `-=`, `[]`| Random-access only                          |
| `==`, `!=`, `<`, ...      | Comparisons; reverse of base order          |

## Example

```cpp
#include <castle/buffers/ring_buffer.h>
using rb_t = castle::buffers::ring_buffer<int, 8>;

rb_t q;
for (int i = 0; i < 5; ++i) q.push(i);

for (auto it = q.rbegin(); it != q.rend(); ++it) {
    std::cout << *it << ' ';   // prints 4 3 2 1 0
}
```

## See also

- `iterator.h` — the underlying `basic_iterator`.
- `iterator_traits.h` — category tags used for SFINAE gating.
