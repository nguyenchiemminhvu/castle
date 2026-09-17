#ifndef CASTLE_UTILITY_STRING_BUILDER_HPP
#define CASTLE_UTILITY_STRING_BUILDER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/error/status.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"

#include <stdint.h>

namespace castle
{

namespace detail
{

// CharT-specific literal text/digit helpers. Specialized below for the two
// character types basic_string_builder supports; any other CharT leaves this
// primary template undefined, which turns into a clear "incomplete type"
// compile error backed by the class-level static_assert further down.
template <typename CharT>
struct string_builder_literals;

template <>
struct string_builder_literals<char>
{
    static CASTLE_CONSTEXPR CASTLE_CONST char* true_text() CASTLE_NOEXCEPT { return "true"; }
    static CASTLE_CONSTEXPR CASTLE_CONST char* false_text() CASTLE_NOEXCEPT { return "false"; }
    static CASTLE_CONSTEXPR CASTLE_CONST char* nan_text() CASTLE_NOEXCEPT { return "nan"; }
    static CASTLE_CONSTEXPR CASTLE_CONST char* inf_text() CASTLE_NOEXCEPT { return "inf"; }
    static CASTLE_CONSTEXPR CASTLE_CONST char* ellipsis_text() CASTLE_NOEXCEPT { return "..."; }
    static CASTLE_CONSTEXPR char digit(unsigned value) CASTLE_NOEXCEPT { return static_cast<char>('0' + value); }
    static CASTLE_CONSTEXPR char minus() CASTLE_NOEXCEPT { return '-'; }
    static CASTLE_CONSTEXPR char dot() CASTLE_NOEXCEPT { return '.'; }
};

template <>
struct string_builder_literals<wchar_t>
{
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* true_text() CASTLE_NOEXCEPT { return L"true"; }
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* false_text() CASTLE_NOEXCEPT { return L"false"; }
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* nan_text() CASTLE_NOEXCEPT { return L"nan"; }
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* inf_text() CASTLE_NOEXCEPT { return L"inf"; }
    static CASTLE_CONSTEXPR CASTLE_CONST wchar_t* ellipsis_text() CASTLE_NOEXCEPT { return L"..."; }
    static CASTLE_CONSTEXPR wchar_t digit(unsigned value) CASTLE_NOEXCEPT { return static_cast<wchar_t>(L'0' + value); }
    static CASTLE_CONSTEXPR wchar_t minus() CASTLE_NOEXCEPT { return L'-'; }
    static CASTLE_CONSTEXPR wchar_t dot() CASTLE_NOEXCEPT { return L'.'; }
};

// Detects castle container types (e.g. castle::container::basic_string<>)
// that expose a `.view()` accessor whose character type matches CharT, so
// they can be appended without needing a dedicated overload per container
// type. Requiring the CharT match keeps e.g. a char-based container from
// being routed here when appended to a wchar_t builder - that combination
// falls through to the static_assert in append() instead of hard-failing
// deep inside basic_string_view's constructor.
template <typename T, typename CharT, typename = void>
struct has_view_member : meta::false_type {};

template <typename T, typename CharT>
struct has_view_member<T, CharT, meta::void_t<decltype(meta::declval<CASTLE_CONST T&>().view())>>
    : meta::bool_constant<meta::is_same<
          typename decltype(meta::declval<CASTLE_CONST T&>().view())::value_type, CharT>::value>
{
};

} // namespace detail

// ============================================================================
// basic_string_builder
//
// Fixed-capacity, heap-free string builder. Folds a heterogeneous, fully
// type-checked set of values (characters, booleans, integers, floating-point
// numbers, enums, C strings/views, or any castle container exposing view())
// into an internal castle::container::basic_string<CharT, N>.
//
// Overflow handling is deterministic and one-shot: the first value that does
// not fully fit is truncated to whatever space remains, and the builder is
// permanently sealed with a trailing "..." marker. Nothing appended after
// sealing changes the content. When TrimWhitespaceBeforeEllipsis is true
// (the default) trailing whitespace already written is stripped before the
// marker is placed, so e.g. "value   " never becomes "value   ..." - it
// becomes "value...". Set the flag to false to keep the marker glued right
// after whatever was written, whitespace included. The choice is a compile
// time template parameter (not a runtime flag) to keep the behavior static
// and branch-free, matching castle's deterministic embedded conventions.
//
// No heap allocation, no exceptions, no RTTI, no virtual dispatch.
// ============================================================================
template <typename CharT, size_type N, bool TrimWhitespaceBeforeEllipsis = true>
class basic_string_builder
{
    static_assert(N > 0U, "castle::basic_string_builder requires a positive capacity");
    static_assert(meta::is_same<CharT, char>::value || meta::is_same<CharT, wchar_t>::value,
                  "castle::basic_string_builder only supports char or wchar_t");

public:
    using value_type = CharT;
    using size_type = castle::size_type;
    using view_type = container::basic_string_view<CharT>;
    using string_type = container::basic_string<CharT, N>;

    static CASTLE_CONSTEXPR size_type static_capacity = N;
    static CASTLE_CONSTEXPR size_type default_float_precision = 3U;
    static CASTLE_CONSTEXPR size_type max_float_precision = 9U;
    static CASTLE_CONSTEXPR size_type max_integer_digits = 20U; // decimal digits of UINT64_MAX

    basic_string_builder() CASTLE_NOEXCEPT
        : buffer_(), truncated_(false)
    {
    }

    basic_string_builder& clear() CASTLE_NOEXCEPT
    {
        buffer_.clear();
        truncated_ = false;
        return *this;
    }

    // ------------------------------------------------------------------------
    // Type-safe value dispatch. Every candidate type is resolved with
    // `if constexpr`, so an unsupported argument type fails to compile against
    // the static_assert below instead of silently doing nothing or binding to
    // an unintended overload.
    // ------------------------------------------------------------------------
    template <typename T>
    basic_string_builder& append(T value) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_convertible<T, view_type>::value)
        {
            append_truncating(view_type(value));
        }
        else CASTLE_IF_CONSTEXPR (meta::is_same<T, CharT>::value)
        {
            append_char_truncating(value);
        }
        else CASTLE_IF_CONSTEXPR (meta::is_same<T, bool>::value)
        {
            append_truncating(value
                              ? view_type(detail::string_builder_literals<CharT>::true_text())
                              : view_type(detail::string_builder_literals<CharT>::false_text()));
        }
        else CASTLE_IF_CONSTEXPR (meta::is_enum<T>::value)
        {
            append(static_cast<meta::underlying_type_t<T>>(value));
        }
        else CASTLE_IF_CONSTEXPR (meta::is_integral<T>::value)
        {
            append_integer_truncating(value);
        }
        else CASTLE_IF_CONSTEXPR (meta::is_floating_point<T>::value)
        {
            append_floating_truncating(value, default_float_precision);
        }
        else CASTLE_IF_CONSTEXPR (detail::has_view_member<T, CharT>::value)
        {
            append_truncating(view_type(value.view()));
        }
        else
        {
            static_assert(meta::dependent_false<T>::value,
                          "castle::basic_string_builder::append: unsupported argument type. "
                          "Supported: CharT, bool, integral, floating-point, enum, types "
                          "convertible to castle::container::basic_string_view<CharT>, or any "
                          "type exposing a view() accessor (e.g. castle::container::basic_string).");
        }
        return *this;
    }

    // Explicit decimal precision for a single floating-point value (silently
    // clamped to max_float_precision). Use this instead of append(value) when
    // the default precision does not suit a particular field.
    template <typename T>
    meta::enable_if_t<meta::is_floating_point<T>::value, basic_string_builder&>
    append(T value, size_type precision) CASTLE_NOEXCEPT
    {
        append_floating_truncating(value, precision > max_float_precision ? max_float_precision : precision);
        return *this;
    }

    template <typename T>
    basic_string_builder& operator<<(T value) CASTLE_NOEXCEPT
    {
        return append(value);
    }

    // Appends every argument in order; equivalent to calling append() once per
    // argument. Safe (but a no-op) to keep calling after the builder has
    // sealed itself with the overflow marker.
    template <typename... Args>
    basic_string_builder& build(Args... args) CASTLE_NOEXCEPT
    {
        (append(args), ...);
        return *this;
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.size(); }
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.empty(); }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.full(); }

    // True once some appended value did not fully fit: the overflow marker has
    // been applied and the builder no longer accepts further content.
    bool truncated() CASTLE_CONST CASTLE_NOEXCEPT { return truncated_; }

    // castle::status flavoured view of truncated(), for callers that thread
    // status codes through their diagnostics pipeline instead of bool checks.
    status build_status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return truncated_ ? status::data_loss : status::ok;
    }

    CASTLE_CONST CharT* c_str() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.c_str(); }
    view_type view() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.view(); }

private:
    void append_truncating(view_type text) CASTLE_NOEXCEPT
    {
        if (truncated_)
        {
            return;
        }

        CASTLE_CONST size_type space = buffer_.available();
        if (text.size() <= space)
        {
            buffer_.append(text);
            return;
        }

        if (space > 0U)
        {
            buffer_.append(view_type(text.data(), space));
        }
        seal_overflow();
    }

    void append_char_truncating(CharT ch) CASTLE_NOEXCEPT
    {
        if (truncated_)
        {
            return;
        }

        if (buffer_.push_back(ch) != status::ok)
        {
            seal_overflow();
        }
    }

    void seal_overflow() CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (TrimWhitespaceBeforeEllipsis)
        {
            trim_trailing_whitespace();
        }

        CASTLE_CONST view_type marker(detail::string_builder_literals<CharT>::ellipsis_text());
        CASTLE_CONST size_type marker_length = marker.size() > N ? N : marker.size();

        while (buffer_.size() > N - marker_length)
        {
            buffer_.pop_back();
        }

        if (marker_length > 0U)
        {
            buffer_.append(view_type(marker.data(), marker_length));
        }

        truncated_ = true;
    }

    void trim_trailing_whitespace() CASTLE_NOEXCEPT
    {
        CASTLE_CONST view_type ws(meta::whitespace<CharT>::value);
        while (!buffer_.empty() && (ws.find(buffer_.back()) != view_type::npos))
        {
            buffer_.pop_back();
        }
    }

    template <typename T>
    void append_integer_truncating(T value) CASTLE_NOEXCEPT
    {
        bool negative = false;
        uint64_t magnitude = 0ULL;

        // Guarded by if constexpr (rather than a plain runtime check) so the
        // "value < 0" comparison is never instantiated for unsigned T - that
        // keeps -Wtype-limits / -Wextra silent instead of flagging a tautology.
        CASTLE_IF_CONSTEXPR (meta::is_signed<T>::value)
        {
            negative = value < static_cast<T>(0);
            magnitude = negative
                        ? (0ULL - static_cast<uint64_t>(static_cast<int64_t>(value)))
                        : static_cast<uint64_t>(value);
        }
        else
        {
            magnitude = static_cast<uint64_t>(value);
        }

        CharT digits[max_integer_digits];
        size_type count = 0U;
        do
        {
            digits[count] = detail::string_builder_literals<CharT>::digit(static_cast<unsigned>(magnitude % 10U));
            ++count;
            magnitude /= 10U;
        } while ((magnitude != 0U) && (count < max_integer_digits));

        if (negative)
        {
            append_char_truncating(detail::string_builder_literals<CharT>::minus());
        }

        for (size_type i = count; i > 0U; --i)
        {
            append_char_truncating(digits[i - 1U]);
        }
    }

    template <typename T>
    void append_floating_truncating(T value, size_type precision) CASTLE_NOEXCEPT
    {
        if (value != value) // IEEE-754: NaN is the only value not equal to itself.
        {
            append_truncating(view_type(detail::string_builder_literals<CharT>::nan_text()));
            return;
        }

        CASTLE_CONST bool negative = value < static_cast<T>(0);
        T magnitude = negative ? -value : value;

        if (negative)
        {
            append_char_truncating(detail::string_builder_literals<CharT>::minus());
        }

        // Magnitudes the 64-bit accumulator below cannot hold are reported as
        // infinite. Kept dependency-free (no <math.h> isinf/isnan), mirroring
        // castle::math::sqrt_real's builtin-first / libm-free convention.
        CASTLE_CONST T overflow_threshold = static_cast<T>(18446744073709551615ULL);
        if (magnitude >= overflow_threshold)
        {
            append_truncating(view_type(detail::string_builder_literals<CharT>::inf_text()));
            return;
        }

        uint64_t integer_part = static_cast<uint64_t>(magnitude);
        T fractional = magnitude - static_cast<T>(integer_part);

        uint64_t scale = 1ULL;
        for (size_type i = 0U; i < precision; ++i)
        {
            scale *= 10U;
        }

        uint64_t fractional_scaled = static_cast<uint64_t>((fractional * static_cast<T>(scale)) + static_cast<T>(0.5));
        if (fractional_scaled >= scale)
        {
            fractional_scaled -= scale;
            ++integer_part;
        }

        append_integer_truncating(integer_part);

        if (precision > 0U)
        {
            append_char_truncating(detail::string_builder_literals<CharT>::dot());

            CharT digits[max_float_precision];
            for (size_type i = precision; i > 0U; --i)
            {
                digits[i - 1U] = detail::string_builder_literals<CharT>::digit(static_cast<unsigned>(fractional_scaled % 10U));
                fractional_scaled /= 10U;
            }

            for (size_type i = 0U; i < precision; ++i)
            {
                append_char_truncating(digits[i]);
            }
        }
    }

    string_type buffer_;
    bool truncated_;
};

template <size_type N, bool TrimWhitespaceBeforeEllipsis = true>
using string_builder = basic_string_builder<char, N, TrimWhitespaceBeforeEllipsis>;

template <size_type N, bool TrimWhitespaceBeforeEllipsis = true>
using wstring_builder = basic_string_builder<wchar_t, N, TrimWhitespaceBeforeEllipsis>;

} // namespace castle

#endif // CASTLE_UTILITY_STRING_BUILDER_HPP
