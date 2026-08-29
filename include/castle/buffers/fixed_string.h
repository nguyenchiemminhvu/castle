#pragma once

#include "castle/types/traits.h"
#include "castle/iterators/iterator.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <cstdarg>
#include <cstdio>

using namespace castle::types;

// ─────────────────────────────────────────────────────────────────────────────
// fixed_string<N> — stack-allocated, bounded, append-only string builder.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A fixed-capacity character buffer used to assemble log messages and records
 * without any heap allocation.
 *
 *   • Capacity N is a compile-time constant — total footprint is N+1 bytes
 *     plus book-keeping (size + truncated flag).  The extra byte reserves
 *     room for a trailing NUL so c_str() is always valid.
 *   • append() silently truncates on overflow and sets truncated() = true.
 *     A trailing "..." sentinel replaces the last 3 bytes so clipping is
 *     visible to the reader.
 *   • view() returns a std::string_view for zero-copy consumption.
 *
 * Not thread-safe on its own; the logger serialises access via its mutex.
 */

namespace castle
{
namespace buffers
{

template <std::size_t N>
class fixed_string
{
    static_assert(N >= 16, "fixed_string capacity must leave room for prefix + ellipsis");

public:
    fixed_string() noexcept { data_[0] = '\0'; }

    using value_type      = char;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = char&;
    using const_reference = const char&;

    using self_type = fixed_string<N>;

    // Logical index == physical index; iteration covers the size() characters,
    // excluding the trailing NUL. Random-access because iterator_at() is O(1).
    using iterator =
        castle::iterators::basic_iterator<
            self_type,
            castle::iterators::iterator_at_traversal,
            castle::iterators::random_access_iterator_tag>;
    using const_iterator =
        castle::iterators::basic_iterator<
            const self_type,
            castle::iterators::iterator_at_traversal,
            castle::iterators::random_access_iterator_tag>;
    using reverse_iterator       = castle::iterators::reverse_iterator<iterator>;
    using const_reverse_iterator = castle::iterators::reverse_iterator<const_iterator>;

    static constexpr size_type capacity() noexcept { return N; }

    size_type   size()      const noexcept { return size_; }
    bool        empty()     const noexcept { return size_ == 0; }
    bool        truncated() const noexcept { return truncated_; }

    const value_type* c_str() const noexcept { return data_.data(); }
    std::string_view view()   const noexcept { return { data_.data(), size_ }; }

    value_type& operator[](size_type i) noexcept { return data_[i]; }

    void clear() noexcept
    {
        size_      = 0;
        truncated_ = false;
        data_[0]   = '\0';
    }

    // ── append overloads ────────────────────────────────────────────────

    void append(std::string_view sv) noexcept { write_raw(sv.data(), sv.size()); }

    void append(const value_type* s) noexcept
    {
        if (s == nullptr)
        {
            append(std::string_view{"(null)"});
            return;
        }
        write_raw(s, std::strlen(s));
    }

    void append(value_type c) noexcept { write_raw(&c, 1); }

    void append(const std::string& s) noexcept { write_raw(s.data(), s.size()); }

    // Integers — formatted into a small stack scratch, then copied.
    void append(long long v)          noexcept { append_fmt("%lld", v); }
    void append(unsigned long long v) noexcept { append_fmt("%llu", v); }
    void append(long v)               noexcept { append_fmt("%ld",  v); }
    void append(unsigned long v)      noexcept { append_fmt("%lu",  v); }
    void append(int v)                noexcept { append_fmt("%d",   v); }
    void append(unsigned int v)       noexcept { append_fmt("%u",   v); }
    void append(short v)              noexcept { append_fmt("%d",   static_cast<int>(v)); }
    void append(unsigned short v)     noexcept { append_fmt("%u",   static_cast<unsigned>(v)); }

    // Floating point — "%g" defaults (6 sig figs).
    void append(double v) noexcept { append_fmt("%g", v); }
    void append(float  v) noexcept { append_fmt("%g", static_cast<double>(v)); }

    // Pointers
    void append(const void* p) noexcept
    {
        if (p == nullptr)
        {
            append(std::string_view{"(null)"});
            return;
        }
        append_fmt("%p", p);
    }

    // Bool
    void append(bool v) noexcept { append(std::string_view{ v ? "true" : "false" }); }

    // --- utilities -------------------------------------------------------------

    void trim_whitespace_left() noexcept
    {
        if (size_ == 0)
        {
            return;
        }

        auto* start = data_.data();
        while (start < data_.data() + size_ && std::strchr(whitespace_v<value_type>, *start) != nullptr)
        {
            ++start;
        }
        if (start != data_.data())
        {
            const size_type new_size = static_cast<size_type>(data_.data() + size_ - start);
            (void)std::memmove(data_.data(), start, new_size);
            size_ = new_size;
            data_[size_] = '\0';
        }
    }

    void trim_whitespace_right() noexcept
    {
        if (size_ == 0)
        {
            return;
        }

        if (truncated_)
        {
            // If the string was truncated, we can't be sure where the last
            // non-whitespace character is, so we don't trim right.
            return;
        }

        auto* end = data_.data() + size_;
        while (end > data_.data() && std::strchr(whitespace_v<value_type>, *(end - 1)) != nullptr)
        {
            --end;
        }
        size_ = static_cast<size_type>(end - data_.data());
        data_[size_] = '\0';
    }

    void trim_whitespaces() noexcept
    {
        trim_whitespace_left();
        trim_whitespace_right();
    }

    void pad_left(value_type c, size_type total_width) noexcept
    {
        if (total_width <= size_ || total_width > N)
        {
            return;
        }

        const size_type pad_count = total_width - size_;
        if (pad_count > 0)
        {
            // Shift existing content to the right.
            (void)std::memmove(data_.data() + pad_count, data_.data(), size_);
            // Fill the left padding with the specified character.
            (void)std::memset(data_.data(), c, pad_count);
            size_ += pad_count;
            data_[size_] = '\0';
        }
    }

    void pad_right(value_type c, size_type total_width) noexcept
    {
        if (total_width <= size_ || total_width > N)
        {
            return;
        }

        for (size_type i = size_; i < total_width; ++i)
        {
            write_raw(&c, 1);
        }
    }

    void to_uppercase() noexcept
    {
        for (size_type i = 0; i < size_; ++i)
        {
            data_[i] = static_cast<value_type>(std::toupper(static_cast<int32_t>(data_[i])));
        }
    }

    void to_lowercase() noexcept
    {
        for (size_type i = 0; i < size_; ++i)
        {
            data_[i] = static_cast<value_type>(std::tolower(static_cast<int32_t>(data_[i])));
        }
    }

    // ── iteration (character order, excludes trailing NUL) ────────────────
    //
    // Any mutating operation invalidates existing iterators.

    iterator       begin()       noexcept { return iterator(this, 0); }
    iterator       end()         noexcept { return iterator(this, static_cast<difference_type>(size_)); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator end()   const noexcept { return const_iterator(this, static_cast<difference_type>(size_)); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end(); }

    reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(cbegin()); }

    // Maps a logical index to its value_typeacter. Used by the iterator traversal
    // policy. Precondition: index < size(); no bounds check so dereferencing a
    // valid iterator stays O(1).
    reference       iterator_at(size_type index)       noexcept { return data_[index]; }
    const_reference iterator_at(size_type index) const noexcept { return data_[index]; }

private:
    void append_fmt(const value_type* fmt, ...) noexcept
    {
        // 32 bytes is enough for any 64-bit integer / %g double / %p.
        value_type scratch[32];
        va_list ap;
        va_start(ap, fmt);
        const int n = std::vsnprintf(scratch, sizeof(scratch), fmt, ap);
        va_end(ap);
        if (n > 0)
        {
            const size_type len =
                (static_cast<size_type>(n) < sizeof(scratch))
                    ? static_cast<size_type>(n)
                    : sizeof(scratch) - 1;
            write_raw(scratch, len);
        }
    }

    void write_raw(const value_type* src, size_type len) noexcept
    {
        if (truncated_) return;                     // Nothing more fits.

        const size_type room = (size_ < N) ? (N - size_) : 0;
        if (len <= room)
        {
            std::memcpy(data_.data() + size_, src, len);
            size_ += len;
        }
        else
        {
            if (room > 0)
            {
                std::memcpy(data_.data() + size_, src, room);
                size_ += room;
            }
            mark_truncated();
        }
        data_[size_] = '\0';
    }

    void mark_truncated() noexcept
    {
        truncated_ = true;
        // Overwrite the last 3 bytes with an ellipsis so clipping is visible.
        constexpr size_type k = 3;
        if (size_ >= k)
        {
            const size_type pos = size_ - k;
            data_[pos + 0] = '.';
            data_[pos + 1] = '.';
            data_[pos + 2] = '.';
        }
        data_[size_] = '\0';
    }

    std::array<value_type, N + 1> data_{};
    size_type                     size_      = 0;
    bool                          truncated_ = false;
};

} // namespace buffers
} // namespace castle