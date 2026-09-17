#ifndef CASTLE_CORE_COMPILER_VARIANTS_GCC_H
#define CASTLE_CORE_COMPILER_VARIANTS_GCC_H

#define CASTLE_COMPILER_GCC  1

#define CASTLE_INLINE        inline __attribute__((always_inline))
#define CASTLE_HOT           __attribute__((hot))
#define CASTLE_COLD          __attribute__((cold))
#define CASTLE_NOINLINE      __attribute__((noinline))

#define CASTLE_LIKELY(x)     __builtin_expect(!!(x), 1)
#define CASTLE_UNLIKELY(x)   __builtin_expect(!!(x), 0)

#define CASTLE_UNREACHABLE() __builtin_unreachable()

#define CASTLE_PACKED_ATTR   __attribute__((packed))
#define CASTLE_RESTRICT      __restrict__

#endif // CASTLE_CORE_COMPILER_VARIANTS_GCC_H
