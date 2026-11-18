# Mean

## Overview
Stateful running-average accumulator for deterministic embedded code that needs to ingest samples one at a time or from a simple iterator range.

## Header
`#include "castle/math/mean.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| API | Description |
|---|---|
| `template <typename TInput, typename TCalc = TInput> class castle::math::mean` | Maintains a running sum and sample count, then reports the arithmetic mean as `double`. |
| `mean()` / `mean(first, last)` | Construct an empty accumulator or populate it from an iterator range. |
| `add(value)` / `add(first, last)` | Append one sample or a whole range. |
| `operator()(value)` / `operator()(first, last)` | Function-call syntax aliases for the corresponding `add()` overloads. |
| `double get_mean() const` / `operator double() const` | Return the current mean; empty accumulators yield `0.0`. |
| `size_type count() const` | Returns the number of accumulated samples. |
| `void clear()` | Resets the accumulator to the empty state. |

## Usage Example
See `samples/sample_mean.cpp` for a complete example.

```cpp
int values[] = {10, 20, 30};
castle::math::mean<int, long long> average(values, values + 3);
const double result = average.get_mean();
```

## Constraints & Notes
- For integral input types, choose `TCalc` wide enough to hold the running sum without overflow.
- The sample counter is a `uint32_t`.
- `get_mean()` caches the last computed `double` result and recomputes only after mutation.
