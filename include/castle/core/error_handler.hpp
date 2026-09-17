// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Deterministic Castle error descriptors and assertion-routing macros.
// Include this header when embedded code must report contract failures without
// exceptions, heap allocation, RTTI, or virtual dispatch. The header exposes a
// lightweight failure descriptor, an optional global callback slot, and a set
// of CASTLE_ASSERT* macros whose behavior is selected entirely by build-time
// feature macros.

#ifndef CASTLE_CORE_ERROR_HANDLER_HPP
#define CASTLE_CORE_ERROR_HANDLER_HPP

#include "castle/core/compiler.hpp"

#include <assert.h>
#include <stddef.h>

namespace castle
{

/// @brief Allocation-free descriptor of a failure site.
class exception
{
public:
    /// @brief Constructs a failure descriptor.
    /// @param reason Failure reason string or nullptr.
    /// @param file Source filename string or nullptr.
    /// @param line Source line number.
    CASTLE_CONSTEXPR exception(CASTLE_CONST char* reason, CASTLE_CONST char* file, int line) CASTLE_NOEXCEPT
        : reason_(reason == nullptr ? "" : reason)
        , file_  (file   == nullptr ? "" : file)
        , line_  (line)
    {
    }

    /// @brief Returns the stored reason string.
    /// @return Pointer to the stored reason text.
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST char* what()      CASTLE_CONST CASTLE_NOEXCEPT { return reason_; }
    /// @brief Returns the stored source filename.
    /// @return Pointer to the stored filename text.
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST char* file_name() CASTLE_CONST CASTLE_NOEXCEPT { return file_; }
    /// @brief Returns the stored source line number.
    /// @return Source line number supplied to the constructor.
    CASTLE_NODISCARD CASTLE_CONSTEXPR              int   line()      CASTLE_CONST CASTLE_NOEXCEPT { return line_; }

private:
    CASTLE_CONST char* reason_;
    CASTLE_CONST char* file_;
    int                line_;
};

#if defined(CASTLE_LOG_ERRORS) || defined(CASTLE_USE_ASSERT_FUNCTION)

/// @brief Global callback slot used by callback-based Castle assertion modes.
class error_handler
{
public:
    /// @brief Function pointer type used for error callbacks.
    using callback_t = void (*)(CASTLE_CONST castle::exception&);

    /// @brief Installs or clears the global error callback.
    /// @param cb Callback to install, or nullptr to clear it.
    static void set_callback(callback_t cb) CASTLE_NOEXCEPT
    {
        get_slot() = cb;
    }

    /// @brief Dispatches an exception object to the installed callback.
    /// @param e Failure descriptor to forward.
    static void error(CASTLE_CONST castle::exception& e) CASTLE_NOEXCEPT
    {
        callback_t cb = get_slot();
        if (cb != nullptr)
        {
            cb(e);
        }
    }

    /// @brief Returns the currently installed callback.
    /// @return The registered callback, or nullptr when none is installed.
    CASTLE_NODISCARD static callback_t get_callback() CASTLE_NOEXCEPT
    {
        return get_slot();
    }

private:
    /// @brief Returns storage for the global callback pointer.
    /// @return Reference to the single callback slot.
    static callback_t& get_slot() CASTLE_NOEXCEPT
    {
        static callback_t s_callback = nullptr;
        return s_callback;
    }
};

#endif // CASTLE_LOG_ERRORS || CASTLE_USE_ASSERT_FUNCTION

} // namespace castle

/// @def CASTLE_ERROR
/// @brief Builds a user-defined error object according to the active error-detail mode.
/// @param TYPE Error type that accepts the generated constructor arguments.
#if defined(CASTLE_VERBOSE_ERRORS)
    #define CASTLE_ERROR(TYPE)         (TYPE(__FILE__, __LINE__))
    /// @def CASTLE_ERROR_GENERIC
    /// @brief Builds a castle::exception according to the active error-detail mode.
    /// @param TEXT Failure reason string literal.
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception((TEXT), __FILE__, __LINE__))
#elif defined(CASTLE_MINIMAL_ERRORS)
    #define CASTLE_ERROR(TYPE)         (TYPE("", -1))
    /// @def CASTLE_ERROR_GENERIC
    /// @brief Builds a castle::exception according to the active error-detail mode.
    /// @param TEXT Failure reason string literal.
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception("", "", -1))
#else
    #define CASTLE_ERROR(TYPE)         (TYPE("", -1))
    /// @def CASTLE_ERROR_GENERIC
    /// @brief Builds a castle::exception according to the active error-detail mode.
    /// @param TEXT Failure reason string literal.
    #define CASTLE_ERROR_GENERIC(TEXT) (::castle::exception((TEXT), "", -1))
#endif

/// @def CASTLE_DO_NOTHING
/// @brief Expands to a no-op expression.
#define CASTLE_DO_NOTHING ((void)0)

#if defined(CASTLE_NO_CHECKS)

    /// @def CASTLE_ASSERT
    /// @brief Verifies a condition and otherwise does nothing when checks are disabled.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    #define CASTLE_ASSERT(cond, err)                       static_cast<void>(sizeof(cond))
    /// @def CASTLE_ASSERT_OR_RETURN
    /// @brief Verifies a condition and otherwise does nothing when checks are disabled.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_OR_RETURN(cond, err)             static_cast<void>(sizeof(cond))
    /// @def CASTLE_ASSERT_OR_RETURN_VALUE
    /// @brief Verifies a condition and otherwise does nothing when checks are disabled.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    /// @param v Return value expression.
    #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)    static_cast<void>(sizeof(cond))
    /// @def CASTLE_ASSERT_FAIL
    /// @brief Forces a failure path and otherwise does nothing when checks are disabled.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_FAIL(err)                        CASTLE_DO_NOTHING
    /// @def CASTLE_ASSERT_FAIL_AND_RETURN
    /// @brief Forces a failure path and otherwise does nothing when checks are disabled.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_FAIL_AND_RETURN(err)             CASTLE_DO_NOTHING
    /// @def CASTLE_ASSERT_FAIL_AND_RETURN_VALUE
    /// @brief Forces a failure path and otherwise does nothing when checks are disabled.
    /// @param err Error object expression.
    /// @param v Return value expression.
    #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v)    CASTLE_DO_NOTHING

#elif defined(CASTLE_USE_ASSERT_FUNCTION) || defined(CASTLE_LOG_ERRORS)

    /// @def CASTLE_ASSERT
    /// @brief Routes a failed condition through castle::error_handler.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    #define CASTLE_ASSERT(cond, err)                                                               do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); } } while (false)
    /// @def CASTLE_ASSERT_OR_RETURN
    /// @brief Routes a failed condition through castle::error_handler and returns from the current function.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_OR_RETURN(cond, err)                                                     do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); return; } } while (false)
    /// @def CASTLE_ASSERT_OR_RETURN_VALUE
    /// @brief Routes a failed condition through castle::error_handler and returns a fallback value.
    /// @param cond Condition expression.
    /// @param err Error object expression.
    /// @param v Return value expression.
    #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)                                            do { if (CASTLE_UNLIKELY(!(cond))) { ::castle::error_handler::error((err)); return (v); } } while (false)
    /// @def CASTLE_ASSERT_FAIL
    /// @brief Unconditionally routes an error through castle::error_handler.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_FAIL(err)                                                                do { ::castle::error_handler::error((err)); } while (false)
    /// @def CASTLE_ASSERT_FAIL_AND_RETURN
    /// @brief Unconditionally routes an error through castle::error_handler and returns.
    /// @param err Error object expression.
    #define CASTLE_ASSERT_FAIL_AND_RETURN(err)                                                     do { ::castle::error_handler::error((err)); return; } while (false)
    /// @def CASTLE_ASSERT_FAIL_AND_RETURN_VALUE
    /// @brief Unconditionally routes an error through castle::error_handler and returns a fallback value.
    /// @param err Error object expression.
    /// @param v Return value expression.
    #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v)                                            do { ::castle::error_handler::error((err)); return (v); } while (false)

#else

    #if !defined(NDEBUG)
        /// @def CASTLE_ASSERT
        /// @brief Verifies a condition with C assert() in debug builds.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        #define CASTLE_ASSERT(cond, err)                    assert((cond))
        /// @def CASTLE_ASSERT_OR_RETURN
        /// @brief Verifies a condition with C assert() in debug builds and returns on failure.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_OR_RETURN(cond, err)                      do { if (CASTLE_UNLIKELY(!(cond))) { assert(false); return; } } while (false)
        /// @def CASTLE_ASSERT_OR_RETURN_VALUE
        /// @brief Verifies a condition with C assert() in debug builds and returns a fallback value on failure.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        /// @param v Return value expression.
        #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)             do { if (CASTLE_UNLIKELY(!(cond))) { assert(false); return (v); } } while (false)
        /// @def CASTLE_ASSERT_FAIL
        /// @brief Unconditionally triggers C assert(false) in debug builds.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_FAIL(err)                     assert(false)
        /// @def CASTLE_ASSERT_FAIL_AND_RETURN
        /// @brief Unconditionally triggers C assert(false) in debug builds and returns.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_FAIL_AND_RETURN(err)          do { assert(false); return; } while (false)
        /// @def CASTLE_ASSERT_FAIL_AND_RETURN_VALUE
        /// @brief Unconditionally triggers C assert(false) in debug builds and returns a fallback value.
        /// @param err Error object expression.
        /// @param v Return value expression.
        #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v) do { assert(false); return (v); } while (false)
    #else
        /// @def CASTLE_ASSERT
        /// @brief Verifies a condition and otherwise does nothing in release builds.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        #define CASTLE_ASSERT(cond, err)                    static_cast<void>(sizeof(cond))
        /// @def CASTLE_ASSERT_OR_RETURN
        /// @brief Returns from the current function when the condition fails in release builds.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_OR_RETURN(cond, err)                      do { if (CASTLE_UNLIKELY(!(cond))) { return; } } while (false)
        /// @def CASTLE_ASSERT_OR_RETURN_VALUE
        /// @brief Returns a fallback value when the condition fails in release builds.
        /// @param cond Condition expression.
        /// @param err Error object expression.
        /// @param v Return value expression.
        #define CASTLE_ASSERT_OR_RETURN_VALUE(cond, err, v)             do { if (CASTLE_UNLIKELY(!(cond))) { return (v); } } while (false)
        /// @def CASTLE_ASSERT_FAIL
        /// @brief Expands to a no-op in release builds.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_FAIL(err)                     CASTLE_DO_NOTHING
        /// @def CASTLE_ASSERT_FAIL_AND_RETURN
        /// @brief Returns from the current function in release builds.
        /// @param err Error object expression.
        #define CASTLE_ASSERT_FAIL_AND_RETURN(err)          do { return; } while (false)
        /// @def CASTLE_ASSERT_FAIL_AND_RETURN_VALUE
        /// @brief Returns a fallback value in release builds.
        /// @param err Error object expression.
        /// @param v Return value expression.
        #define CASTLE_ASSERT_FAIL_AND_RETURN_VALUE(err, v) do { return (v); } while (false)
    #endif

#endif

#endif // CASTLE_CORE_ERROR_HANDLER_HPP
