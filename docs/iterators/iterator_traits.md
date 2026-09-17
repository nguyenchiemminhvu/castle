# `castle/iterators/iterator_traits.h` — Traits & category tags

**Header:** `castle/iterators/iterator_traits.h`
**Namespace:** `castle::iterators`

## Purpose

Provides:

- **Iterator category tags** — aliased to the `std::` tags so a CASTLE
  iterator is simultaneously a valid `std::` iterator (and hence works
  with `std::find`, `std::copy`, `std::accumulate`, ... out of the box).
- **`iterator_traits<It>`** — SFINAE-detected trait that exposes the
  usual five typedefs (`iterator_category`, `value_type`,
  `difference_type`, `pointer`, `reference`).
- Specialisations for **raw pointers** (`T*`, `const T*`) treating them
  as random-access iterators.

## Category hierarchy

```
input           output
   |
forward
   |
bidirectional
   |
random_access
```

The tags are exact aliases of their `std::` counterparts, so
`std::iterator_traits<castle_iterator>` also works.

## API

| Alias / trait                    | Description                              |
| -------------------------------- | ---------------------------------------- |
| `input_iterator_tag`             | `= std::input_iterator_tag`              |
| `output_iterator_tag`            | `= std::output_iterator_tag`             |
| `forward_iterator_tag`           | `= std::forward_iterator_tag`            |
| `bidirectional_iterator_tag`     | `= std::bidirectional_iterator_tag`      |
| `random_access_iterator_tag`     | `= std::random_access_iterator_tag`      |
| `iterator_traits<It>`            | Five typedefs, SFINAE-detected           |
| `iterator_traits<T*>`            | Raw-pointer specialisation               |

## See also

- `iterator.h` — the concrete `basic_iterator` class that consumes
  these tags via SFINAE.
- `reverse_iterator.h` — forwards its typedefs through `iterator_traits`.
