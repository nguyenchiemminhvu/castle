// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file string_view.hpp
 * @brief Non-owning view over a character sequence.
 *
 * Use `basic_string_view<CharT>` when text should be inspected or sliced
 * without copying and without requiring a terminating null character in the
 * viewed range. The view stores only a pointer and a length, performs no
 * allocation, and offers O(1) size queries, slicing, and iterator access.
 *
 * @code
 * castle::container::string_view text("SET:TEMP=25");
 * bool command = text.starts_with(castle::container::string_view("SET:"));
 * @endcode
 */
#ifndef CASTLE_CONTAINER_STRING_VIEW_HPP
#define CASTLE_CONTAINER_STRING_VIEW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include <string.h>

namespace castle
{
namespace container
{

template <typename CharT, size_type N>
class basic_string;

/**
 * @brief Non-owning view over a contiguous character sequence.
 * @tparam CharT Character type.
 * @note The view stores only a pointer and a length. It never allocates and
 * never owns the referenced characters.
 * @note `size()`, `length()`, `data()`, iterator access, and `substr()` are O(1).
 * `find(CharT)` is O(N) worst-case. `find(basic_string_view)` is O(N*M)
 * worst-case, where `M` is the needle size.
 * @warning The referenced character storage must outlive the view.
 */
template <typename CharT>
class basic_string_view
{
public:
    using value_type = CharT;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using pointer = CharT*;
    using const_pointer = CASTLE_CONST CharT*;
    using reference = CharT&;
    using const_reference = CASTLE_CONST CharT&;
    using iterator = CharT*;
    using const_iterator = CASTLE_CONST CharT*;

    /**
     * @brief Constructs an empty string view.
     * @note Complexity: O(1).
     */
    basic_string_view() CASTLE_NOEXCEPT : data_(nullptr), size_(0U) {}

    /**
     * @brief Constructs a view from a pointer and explicit length.
     * @param data Pointer to the first character, or `nullptr` for an empty view.
     * @param size Number of characters in the viewed range.
     * @note Complexity: O(1).
     * @warning No ownership is taken; `data` must remain valid for the view's lifetime.
     */
    basic_string_view(const_pointer data, size_type size) CASTLE_NOEXCEPT
        : data_(data), size_(size)
    {
    }

    /**
     * @brief Constructs a view from a null-terminated character sequence.
     * @param text Pointer to the first character of a null-terminated string, or `nullptr`.
     * @note Complexity: O(N) because the length is measured.
     * @warning For multi-byte character types, the sequence is scanned until `CharT()`.
     */
    basic_string_view(const_pointer text) CASTLE_NOEXCEPT
        : data_(text), size_(length(text))
    {
    }

    /**
     * @brief Returns the number of characters.
     * @return View length.
     * @note Complexity: O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the number of characters.
     * @return View length.
     * @note Complexity: O(1).
     */
    size_type length() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Reports whether the view is empty.
     * @return `true` when `size() == 0`, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Returns the underlying character pointer.
     * @return Pointer to the first character, or `nullptr` for an empty default view.
     * @note Complexity: O(1).
     */
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the character at `index`.
     * @param index Zero-based character index.
     * @return Const reference to the indexed character.
     * @note Complexity: O(1).
     * @warning `index` must be less than `size()`.
     */
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the first character.
     * @return Const reference to the first character.
     * @note Complexity: O(1).
     * @warning The view must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the last character.
     * @return Const reference to the last character.
     * @note Complexity: O(1).
     * @warning The view must not be empty.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }

    /**
     * @brief Returns an iterator to the first character.
     * @return Pointer to the first character.
     * @note Complexity: O(1).
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator one past the last character.
     * @return Pointer one past the last character.
     * @note Complexity: O(1).
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? data_ : data_ + size_; }

    /**
     * @brief Returns a bounded substring view.
     * @param offset Zero-based start offset.
     * @param count Requested character count, or `npos` to extend to the end.
     * @return A view covering the requested range clamped to the available
     * characters, or an empty default-constructed view when `offset > size()`.
     * @note Complexity: O(1).
     * @warning The returned view aliases the same storage as the source view.
     */
    basic_string_view substr(size_type offset, size_type count = static_cast<size_type>(-1)) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (offset > size_)
        {
            return basic_string_view();
        }
        CASTLE_CONST size_type available = size_ - offset;
        if (count > available)
        {
            count = available;
        }
        return basic_string_view(
            count == 0U
            ? data_
            : data_ + offset, count
        );
    }

    /**
     * @brief Finds the first occurrence of a character.
     * @param value Character to search for.
     * @param offset Zero-based starting position.
     * @return Index of the first matching character, or `npos` when absent.
     * @note Complexity: O(size() - offset) worst-case.
     */
    size_type find(CharT value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (offset >= size_)
        {
            return npos;
        }

        CASTLE_IF_CONSTEXPR (sizeof(CharT) == 1U)
        {
            CASTLE_CONST void* found = memchr(data_ + offset,
                                              static_cast<unsigned char>(value),
                                              size_ - offset);
            return (found == nullptr)
                   ? npos
                   : static_cast<size_type>(static_cast<const CharT*>(found) - data_);
        }
        else
        {
            for (size_type i = offset; i < size_; ++i)
            {
                if (data_[i] == value)
                {
                    return i;
                }
            }
            return npos;
        }
    }

    /**
     * @brief Finds the first occurrence of a substring view.
     * @param value Substring to search for.
     * @param offset Zero-based starting position.
     * @return Index of the first match, or `npos` when absent.
     * @note Complexity: O(size() * value.size()) worst-case.
     */
    size_type find(basic_string_view value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (value.size_ == 0U)
        {
            return offset <= size_ ? offset : npos;
        }
        if (value.size_ > size_ || offset > size_ - value.size_) // LCOV_EXCL_BR_LINE
        {
            return npos;
        }

        CASTLE_CONST size_type last_start = size_ - value.size_;

        CASTLE_IF_CONSTEXPR (sizeof(CharT) == 1U)
        {
            size_type i = offset;
            while (i <= last_start)
            {
                CASTLE_CONST void* found = memchr(data_ + i,
                                                  static_cast<unsigned char>(value.data_[0U]),
                                                  last_start - i + 1U);
                if (found == nullptr)
                {
                    return npos;
                }
                i = static_cast<size_type>(static_cast<const CharT*>(found) - data_);
                if (memcmp(data_ + i, value.data_, value.size_) == 0)
                {
                    return i;
                }
                ++i;
            }
            return npos;
        }
        else
        {
            for (size_type i = offset; i <= last_start; ++i)
            {
                size_type j = 0U;
                while ((j < value.size_) && (data_[i + j] == value.data_[j]))
                {
                    ++j;
                }
                if (j == value.size_)
                {
                    return i;
                }
            }
            return npos;
        }
    }

    /**
     * @brief Lexicographically compares two views.
     * @param other View to compare against.
     * @return Negative when `*this < other`, zero when equal, or positive when `*this > other`.
     * @note Complexity: O(min(size(), other.size())).
     */
    int compare(basic_string_view other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type common = size_ < other.size_
                                        ? size_
                                        : other.size_;

        int result = memcmp(data_, other.data_, common);
        if (result != 0)
        {
            return result;
        }
        if (size_ < other.size_)
        {
            return -1;
        }
        if (size_ > other.size_)
        {
            return 1;
        }
        return 0;
    }

    /**
     * @brief Reports whether the view starts with `prefix`.
     * @param prefix Prefix to test.
     * @return `true` when the first `prefix.size()` characters match.
     * @note Complexity: O(prefix.size()).
     */
    bool starts_with(basic_string_view prefix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return prefix.size_ <= size_ && substr(0U, prefix.size_).compare(prefix) == 0; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Reports whether the view ends with `suffix`.
     * @param suffix Suffix to test.
     * @return `true` when the last `suffix.size()` characters match.
     * @note Complexity: O(suffix.size()).
     */
    bool ends_with(basic_string_view suffix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return suffix.size_ <= size_ && substr(size_ - suffix.size_, suffix.size_).compare(suffix) == 0; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Sentinel used by search functions when no match exists.
     */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

private:
    /**
     * @brief Calculates the length of a null-terminated string.
     * @param text Pointer to the null-terminated string.
     * @return Length of the string, or `0` if `text` is `nullptr`.
     */
    static size_type length(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            return 0U;
        }

        CASTLE_IF_CONSTEXPR (sizeof(CharT) == 1U)
        {
            return static_cast<size_type>(strlen(reinterpret_cast<CASTLE_CONST char*>(text)));
        }
        else
        {
            size_type count = 0U;
            while (text[count] != CharT())
            {
                ++count;
            }
            return count;
        }
    }

    const_pointer data_;
    size_type size_;
};

/**
 * @brief `char` specialization alias for `basic_string_view`.
 */
using string_view = basic_string_view<char>;

/**
 * @brief Compares two string views for equality.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when both views contain the same character sequence.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator==(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) == 0;
}

/**
 * @brief Compares two string views for inequality.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when the character sequences differ.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator!=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Performs lexicographical less-than comparison.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when `lhs` is lexicographically smaller than `rhs`.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator<(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) < 0;
}

/**
 * @brief Performs lexicographical greater-than comparison.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when `lhs` is lexicographically greater than `rhs`.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator>(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Performs lexicographical less-than-or-equal comparison.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when `lhs` is not greater than `rhs`.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator<=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Performs lexicographical greater-than-or-equal comparison.
 * @tparam CharT Character type.
 * @param lhs Left-hand view.
 * @param rhs Right-hand view.
 * @return `true` when `lhs` is not less than `rhs`.
 * @note Complexity: O(min(lhs.size(), rhs.size())).
 */
template <typename CharT>
bool operator>=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

}
}

#endif
