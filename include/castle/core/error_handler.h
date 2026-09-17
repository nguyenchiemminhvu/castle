#ifndef CASTLE_CORE_ERROR_HANDLER_H
#define CASTLE_CORE_ERROR_HANDLER_H

// ============================================================================
// castle/core/error_handler.h
// ----------------------------------------------------------------------------
// CASTLE version of a deterministic, heap-free, exception-free error handling
// facility for embedded targets.
//
// Feature switches (define at build time, e.g. via -D or in a config header):
//
//   CASTLE_NO_CHECKS             All CASTLE_ASSERT_* macros become no-ops.
//   CASTLE_USE_ASSERT_FUNCTION   Failures are dispatched to a user-provided
//                                function pointer (installed at run time).
//   CASTLE_LOG_ERRORS            Failures are dispatched to a global callback
//                                installed on castle::error_handler.
//   CASTLE_VERBOSE_ERRORS        CASTLE_ERROR* captures __FILE__/__LINE__.
//   CASTLE_MINIMAL_ERRORS        CASTLE_ERROR* captures nothing (smallest).
//   (default)                    CASTLE_ERROR* captures a short reason string.
//
// If none of NO_CHECKS / USE_ASSERT_FUNCTION / LOG_ERRORS is defined, the
// macros fall back to the C <assert.h> in debug builds and evaporate in
// release builds. Exceptions are never thrown - CASTLE targets platforms
// where exceptions are disabled or forbidden.
// ============================================================================

#include "castle/core/compiler.h"

#include <assert.h>
#include <stddef.h>

namespace castle
{

// ----------------------------------------------------------------------------
// castle::exception
// ----------------------------------------------------------------------------
// A trivially copyable, allocation-free descriptor of a failure site. Holds
// only pointers to string literals so it is safe to pass by value even in
// interrupt or low-priority contexts.
// ----------------------------------------------------------------------------
class exception
{
public:
    CASTLE_CONSTEXPR exception(CASTLE_CONST char* reason, CASTLE_CONST char* file, int line) CASTLE_NOEXCEPT
        : reason_(reason == nullptr ? "" : reason)
        , file_  (file   == nullptr ? "" : file)
        , line_  (line)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST char* what()      CASTLE_CONST CASTLE_NOEXCEPT { return reason_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST char* file_name() CASTLE_CONST CASTLE_NOEXCEPT { return file_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR              int   line()      CASTLE_CONST CASTLE_NOEXCEPT { return line_; }

private:
    CASTLE_CONST char* reason_;
    CASTLE_CONST char* file_;
    int                line_;
};

// ----------------------------------------------------------------------------
// castle::error_handler
// ----------------------------------------------------------------------------
// A single-slot registry for a global error callback. Only enabled when
// CASTLE_LOG_ERRORS or CASTLE_USE_ASSERT_FUNCTION is defined so unused
// targets do not pay for storage. The callback is a plain function pointer
// (no captures, no heap, no virtuals) which keeps behaviour deterministic
// and interrupt-safe.
// ----------------------------------------------------------------------------
#if defined(CASTLE_LOG_ERRORS) || defined(CASTLE_USE_ASSERT_FUNCTION)

class error_handler
{
public:
    using callback_t = void (*)(CASTLE_CONST castle::exception&);

    static void set_callback(callback_t cb) CASTLE_NOEXCEPT
    {
        get_slot() = cb;
    }

    static void error(CASTLE_CONST castle::exception& e) CASTLE_NOEXCEPT
    {
        callback_t cb = get_slot();
        if (cb != nullptr)
        {
            cb(e);
        }
    }

    CASTLE_NODISCARD static callback_t get_callback() CASTLE_NOEXCEPT
    {
        return get_slot();
    }

private:
    static callback_t& get_slot() CASTLE_NOEXCEPT
    {
        static callback_t s_callback = nullptr;
        return s_callback;
    }
};

#endif // CASTLE_LOG_ERRORS || CASTLE_USE_ASSERT_FUNCTION

} // namespace castle

// ============================================================================
// Error object construction helpers.
// ============================================================================
#if defined(CASTLE_VERBOSE_ERRORS)
    #define CASTLE_ERROR(TYPE)         (TYPE(__FILE__, __LINE__))
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception((TEXT), __FILE__, __LINE__))
#elif defined(CASTLE_MINIMAL_ERRORS)
    #define CASTLE_ERROR(TYPE)         (TYPE("", -1))
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception("", "", -1))
#else
    #define CASTLE_ERROR(TYPE)         (TYPE("", -1))
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception((TEXT), "", -1))
#endif

#define CASTLE_DO_NOTHING ((void)0)

// ============================================================================
// Assertion macros.
// ============================================================================
#if defined(CASTLE_NO_CHECKS)

    #define CASTLE_ASSERT(cond, err)                       static_cast<void>(sizeof(cond))
    #define CASTLE_ASSERT_OR_RETURN(cond, err)             static_cast<void>(sizeof(cond))
    #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)    static_cast<void>(sizeof(cond))
    #define CASTLE_ASSERT_FAIL(err)                        CASTLE_DO_NOTHING
    #define CASTLE_ASSERT_FAIL_AND_RETURN(err)             CASTLE_DO_NOTHING
    #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v)    CASTLE_DO_NOTHING

#elif defined(CASTLE_USE_ASSERT_FUNCTION) || defined(CASTLE_LOG_ERRORS)

    #define CASTLE_ASSERT(cond, err)                                                       \
        do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); } } while (false)
    #define CASTLE_ASSERT_OR_RETURN(cond, err)                                             \
        do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); return; } } while (false)
    #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)                                    \
        do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); return (v); } } while (false)
    #define CASTLE_ASSERT_FAIL(err)                                                        \
        do { ::castle::error_handler::error((err)); } while (false)
    #define CASTLE_ASSERT_FAIL_AND_RETURN(err)                                             \
        do { ::castle::error_handler::error((err)); return; } while (false)
    #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v)                                    \
        do { ::castle::error_handler::error((err)); return (v); } while (false)

#else // Default: rely on C assert() in debug, no-op in release.

    #if !defined(NOT_DEBUG)
        #define CASTLE_ASSERT(cond, err)                    assert((cond))
        #define CASTLE_ASSERT_OR_RETURN(cond, err)          \
            do { if (CASTLE_UNLIKELY(!(cond))) { assert(false); return; } } while (false)
        #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v) \
            do { if (CASTLE_UNLIKELY(!(cond))) { assert(false); return (v); } } while (false)
        #define CASTLE_ASSERT_FAIL(err)                     assert(false)
        #define CASTLE_ASSERT_FAIL_AND_RETURN(err)          do { assert(false); return; } while (false)
        #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v) do { assert(false); return (v); } while (false)
    #else
        #define CASTLE_ASSERT(cond, err)                    static_cast<void>(sizeof(cond))
        #define CASTLE_ASSERT_OR_RETURN(cond, err)          \
            do { if (CASTLE_UNLIKELY(!(cond))) { return; } } while (false)
        #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v) \
            do { if (CASTLE_UNLIKELY(!(cond))) { return (v); } } while (false)
        #define CASTLE_ASSERT_FAIL(err)                     CASTLE_DO_NOTHING
        #define CASTLE_ASSERT_FAIL_AND_RETURN(err)          do { return; } while (false)
        #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v) do { return (v); } while (false)
    #endif

#endif

#endif // CASTLE_CORE_ERROR_HANDLER_H