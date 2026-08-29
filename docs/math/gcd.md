# `castle/math/gcd.h` — Greatest common divisor

**Header:** `castle/math/gcd.h`
**Namespace:** `castle::math`
**Sample:** [`samples/sample_math_gcd.cpp`](../../samples/sample_math_gcd.cpp)

## Purpose

`constexpr` Euclidean-algorithm GCD, available in three shapes:

- `gcd_v<A, B>::value` — pure compile-time constant.
- `gcd(a, b)` — runtime for any `is_valid_integer_v<T>`.
- `gcd(a, b, c, ...)` — variadic runtime overload with an
  early-out on `gcd == 1`.

## Design notes

- Handles negative signed inputs by taking the absolute value first.
- Loop-based iteration (not recursion) for the runtime overload, so it
  never blows the stack even on 8-bit MCUs.
- Variadic overload short-circuits as soon as an intermediate GCD
  reaches `1`, keeping worst-case time proportional to a single
  reduction chain.

## API

| Overload                          | Kind         |
| --------------------------------- | ------------ |
| `gcd_v<A, B>::value`              | Compile time |
| `gcd(T a, T b)`                   | Runtime      |
| `gcd(T a, T b, Args... rest)`     | Variadic     |

## Example

```cpp
#include <castle/math/gcd.h>
using namespace castle::math;

static_assert(gcd_v<24, 36>::value == 12);

int g  = gcd(24, 36);          // 12
int g2 = gcd(24, 36, 48, 60);  // 12
```

## See also

- `castle/math/lcm.h` — reuses `gcd` internally.
