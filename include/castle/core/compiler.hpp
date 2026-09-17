#ifndef CASTLE_CORE_COMPILER_HPP
#define CASTLE_CORE_COMPILER_HPP

// Detect compiler and include the specific variant file
#if defined(__clang__)
    #include "compiler_variants/clang.hpp"
#elif defined(__ARMCC_VERSION) || defined(__CC_ARM)
    // Note: Check ARM before GCC because ARM6 uses Clang/GCC extensions
    #include "compiler_variants/arm.hpp"
#elif defined(__GNUC__)
    #include "compiler_variants/gcc.hpp"
#else
    #include "compiler_variants/default.hpp"
#endif

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// Standard C++ attributes (compiler independent)

#define CASTLE_UNUSED            [[maybe_unused]]
#define CASTLE_NODISCARD         [[nodiscard]]
#define CASTLE_FALL_THROUGH      [[fallthrough]]
#define CASTLE_DEPRECATED        [[deprecated]]
#define CASTLE_DEPRECATED_MSG(x) [[deprecated(x)]]

// Castle-specific macros for compiler features

#define CASTLE_VOLATILE          volatile
#define CASTLE_MUTABLE           mutable
#define CASTLE_CONST             const
#define CASTLE_CONSTEXPR         constexpr
#define CASTLE_IF_CONSTEXPR      if constexpr
#define CASTLE_NOEXCEPT          noexcept
#define CASTLE_DEFAULT           = default
#define CASTLE_DELETE            = delete
#define CASTLE_OVERRIDE          override
#define CASTLE_FINAL             final
#define CASTLE_VIRTUAL           virtual
#define CASTLE_MOVE              castle::move
#define CASTLE_FORWARD           castle::forward

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // CASTLE_CORE_COMPILER_HPP
