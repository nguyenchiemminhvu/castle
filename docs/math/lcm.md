# `castle/math/lcm.h` — Least common multiple

**Header:** `castle/math/lcm.h`
**Namespace:** `castle::math`
**Sample:** [`samples/sample_math_lcm.cpp`](../../samples/sample_math_lcm.cpp)

## Purpose

`constexpr` LCM built on top of `gcd`, in both compile-time and runtime
flavours.

## Design notes

- Uses the identity `lcm(a, b) = |a / gcd(a, b) * b|` — dividing first
  avoids overflow that the naive `a * b / gcd` would trigger.
- `lcm(0, x)` and `lcm(x, 0)` are defined as `0`.
- Signed inputs are folded to their absolute value.

## API

| Overload                          | Kind         |
| --------------------------------- | ------------ |
| `lcm_v<A, B>::value`              | Compile time |
| `lcm(T a, T b)`                   | Runtime      |

## Example

```cpp
#include <castle/math/lcm.h>
using namespace castle::math;

static_assert(lcm_v<4, 6>::value == 12);
auto x = lcm(4, 6);   // 12
```

## See also

- `castle/math/gcd.h` — factored dependency.
