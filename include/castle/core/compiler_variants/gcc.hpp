#ifndef CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CASTLE_COMPILER_GCC  1

#define CASTLE_INLINE        __attribute__((always_inline)) inline
#define CASTLE_HOT           __attribute__((hot))
#define CASTLE_COLD          __attribute__((cold))
#define CASTLE_NOINLINE      __attribute__((noinline))

#define CASTLE_LIKELY(x)     __builtin_expect(!!(x), 1)
#define CASTLE_UNLIKELY(x)   __builtin_expect(!!(x), 0)

#define CASTLE_UNREACHABLE() __builtin_unreachable()

#define CASTLE_PACKED_ATTR   __attribute__((packed))
#define CASTLE_RESTRICT      __restrict__

// Busy-wait spin hint: reduces power/bus contention on a spinlock retry loop
// without blocking, yielding to an OS, or otherwise changing program state.
#if defined(__i386__) || defined(__x86_64__)
    #define CASTLE_CPU_RELAX() __builtin_ia32_pause()
#elif defined(__arm__) || defined(__aarch64__) || defined(__thumb__)
    #define CASTLE_CPU_RELAX() __asm__ __volatile__("yield" ::: "memory")
#else
    #define CASTLE_CPU_RELAX() __asm__ __volatile__("" ::: "memory")
#endif

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP
