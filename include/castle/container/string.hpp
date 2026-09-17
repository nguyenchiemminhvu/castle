// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file string.hpp
 * @brief Fixed-capacity owning string that always maintains a trailing null terminator.
 *
 * Use this container when text must be stored in a deterministic, heap-free buffer with a
 * compile-time maximum length. Storage for `N + 1` characters is embedded directly in the
 * object, where `N` is the maximum logical length and the extra slot stores the terminator.
 * Appends, inserts, erases, and resizes operate in O(length) worst case and report failures
 * through `castle::status` instead of exceptions.
 *
 * @note Successful mutations preserve null termination at `data()[size()]`.
 * @warning `N` counts logical characters only; writing beyond `N` returns `status::full`.
 */
#ifndef CASTLE_CONTAINER_STRING_HPP
#define CASTLE_CONTAINER_STRING_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/string_view.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity owning string with embedded storage.
 * @tparam CharT Character type.
 * @tparam N Maximum number of stored characters excluding the terminator.
 * @note The string never allocates and always reserves one extra slot for the null terminator.
 * @warning Bounds are not checked by element accessors such as `operator[]`.
 */
template <typename CharT, size_type N>
class basic_string
{
    static_assert(N > 0U, "basic_string requires a positive capacity");

public:
    using value_type = CharT;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = CharT&;
    using const_reference = CASTLE_CONST CharT&;
    using pointer = CharT*;
    using const_pointer = CASTLE_CONST CharT*;
    using iterator = CharT*;
    using const_iterator = CASTLE_CONST CharT*;
    using view_type = basic_string_view<CharT>;

    /** @brief Compile-time maximum logical length. */
    static CASTLE_CONSTEXPR size_type static_capacity = N;

    /**
     * @brief Constructs an empty string.
     * @note Complexity is O(1).
     */
    basic_string() CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
    }

    /**
     * @brief Constructs a string from a null-terminated character sequence.
     * @param text Source C-style string.
     * @note Complexity is O(length(text)).
     * @warning Construction clears the string when assignment fails.
     */
    basic_string(const_pointer text) CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
        if (assign(text) != status::ok)
        {
            clear();
        }
    }

    /**
     * @brief Constructs a string from a string view.
     * @param view Source character range.
     * @note Complexity is O(view.size()).
     * @warning Construction clears the string when assignment fails.
     */
    basic_string(view_type view) CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
        if (assign(view) != status::ok)
        {
            clear();
        }
    }

    /**
     * @brief Copy-constructs the string.
     * @param other Source string.
     * @note Complexity is O(other.size()).
     */
    basic_string(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
        : size_(other.size_)
    {
        copy_chars(data_, other.data_, size_);
    }

    /**
     * @brief Replaces the string contents with a copy of another string.
     * @param other Source string.
     * @return Reference to `*this`.
     * @note Complexity is O(other.size()).
     */
    basic_string& operator=(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            size_ = other.size_;
            copy_chars(data_, other.data_, size_);
        }
        return *this;
    }

    /**
     * @brief Returns the current logical length.
     * @return Number of stored characters in O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the current logical length.
     * @return Same value as `size()`.
     */
    size_type length() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the fixed logical capacity.
     * @return `N`.
     */
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the remaining number of characters that can be appended.
     * @return `capacity() - size()`.
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Tests whether the string is empty.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the string has reached capacity.
     * @return `true` when `size() == capacity()`.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns an iterator to the first character.
     * @return Pointer to the first character slot.
     */
    iterator begin() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first character.
     * @return Const pointer to the first character slot.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first character.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator one past the last character.
     * @return End iterator.
     */
    iterator end() CASTLE_NOEXCEPT { return data_ + size_; }

    /**
     * @brief Returns a const iterator one past the last character.
     * @return Const end iterator.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + size_; }

    /**
     * @brief Returns a const iterator one past the last character.
     * @return Const end iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + size_; }

    /**
     * @brief Returns a mutable pointer to the underlying character buffer.
     * @return Pointer to the first character.
     * @note `data()[size()]` is always the null terminator after successful operations.
     */
    pointer data() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const pointer to the underlying character buffer.
     * @return Const pointer to the first character.
     */
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a C-style null-terminated view of the buffer.
     * @return Pointer to the internal null-terminated character array.
     */
    const_pointer c_str() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the character at `index` without bounds checking.
     * @param index Zero-based character index.
     * @return Reference to the requested character.
     * @warning `index` must be less than `size()`.
     */
    reference operator[](size_type index) CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the character at `index` without bounds checking.
     * @param index Zero-based character index.
     * @return Const reference to the requested character.
     * @warning `index` must be less than `size()`.
     */
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the first character.
     * @return Reference to `data()[0]`.
     * @warning The string must not be empty.
     */
    reference front() CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the first character.
     * @return Const reference to `data()[0]`.
     * @warning The string must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the last character.
     * @return Reference to `data()[size() - 1]`.
     * @warning The string must not be empty.
     */
    reference back() CASTLE_NOEXCEPT { return data_[size_ - 1U]; }

    /**
     * @brief Returns the last character.
     * @return Const reference to `data()[size() - 1]`.
     * @warning The string must not be empty.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }

    /**
     * @brief Resets the string to empty.
     * @note Complexity is O(1).
     */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        data_[0U] = CharT();
    }

    /**
     * @brief Assigns from a null-terminated character sequence.
     * @param text Source C-style string.
     * @return `status::ok` on success, `status::invalid_argument` when `text == nullptr`, or
     * `status::full` when the source length exceeds `capacity()`.
     * @note Complexity is O(length(text)).
     */
    status assign(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            clear();
            return status::invalid_argument;
        }
        size_type length = 0U;
        while (text[length] != CharT())
        {
            ++length;
            if (length > N)
            {
                return status::full;
            }
        }
        copy_chars(data_, text, length);
        size_ = length;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Assigns from a string view.
     * @param view Source character range.
     * @return `status::ok` on success or `status::full` when `view.size() > capacity()`.
     * @note Complexity is O(view.size()).
     */
    status assign(view_type view) CASTLE_NOEXCEPT
    {
        if (view.size() > N)
        {
            return status::full;
        }
        copy_chars(data_, view.data(), view.size());
        size_ = view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Appends one character.
     * @param value Character to append.
     * @return `status::ok` on success or `status::full` when the string is already full.
     * @note Complexity is O(1).
     */
    status push_back(CharT value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }
        data_[size_++] = value;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Removes the last character.
     * @return `status::ok` on success or `status::empty` when the string is empty.
     * @note Complexity is O(1).
     */
    status pop_back() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        --size_;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Appends a null-terminated character sequence.
     * @param text Source C-style string.
     * @return `status::ok` on success, `status::invalid_argument` when `text == nullptr`, or
     * `status::full` when the appended text would exceed capacity.
     * @note Complexity is O(length(text)).
     */
    status append(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            return status::invalid_argument;
        }
        CASTLE_CONST size_type old_size = size_;
        size_type length = 0U;
        while (text[length] != CharT())
        {
            ++length;
            if (length > N - old_size)
            {
                return status::full;
            }
        }
        copy_chars(data_ + old_size, text, length);
        size_ = old_size + length;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Appends a string view.
     * @param view Source character range.
     * @return `status::ok` on success or `status::full` when the appended view would exceed
     * capacity.
     * @note Complexity is O(view.size()).
     */
    status append(view_type view) CASTLE_NOEXCEPT
    {
        if (view.size() > N - size_)
        {
            return status::full;
        }
        copy_chars(data_ + size_, view.data(), view.size());
        size_ += view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Appends another string.
     * @param other Source string.
     * @return `status::ok` on success or `status::full` when the appended string would exceed
     * capacity.
     * @note Complexity is O(other.size()).
     */
    status append(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
    {
        return append(view_type(other.data_, other.size_));
    }

    /**
     * @brief Inserts a string view at `position`.
     * @param position Insertion index in the current string.
     * @param view Characters to insert.
     * @return `status::ok` on success, `status::out_of_range` when `position > size()`, or
     * `status::full` when the insertion would exceed capacity.
     * @note Complexity is O(size() + view.size()) because trailing characters are shifted.
     */
    status insert(size_type position, view_type view) CASTLE_NOEXCEPT
    {
        if (position > size_)
        {
            return status::out_of_range;
        }
        if (view.size() > N - size_)
        {
            return status::full;
        }
        for (size_type i = size_; i > position; --i)
        {
            data_[i + view.size() - 1U] = data_[i - 1U];
        }
        for (size_type i = 0U; i < view.size(); ++i)
        {
            data_[position + i] = view[i];
        }
        size_ += view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Erases up to `count` characters starting at `position`.
     * @param position Starting erase index.
     * @param count Number of characters to erase, clamped to the available suffix.
     * @return `status::ok` on success or `status::out_of_range` when `position > size()`.
     * @note Complexity is O(size() - position).
     */
    status erase(size_type position, size_type count = static_cast<size_type>(-1)) CASTLE_NOEXCEPT
    {
        if (position > size_)
        {
            return status::out_of_range;
        }
        if (count > size_ - position)
        {
            count = size_ - position;
        }
        for (size_type i = position; i + count < size_; ++i)
        {
            data_[i] = data_[i + count];
        }
        size_ -= count;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Changes the logical size of the string.
     * @param count New string length.
     * @param value Fill character used when growing.
     * @return `status::ok` on success or `status::full` when `count > capacity()`.
     * @note Complexity is O(max(0, count - size())) when growing and O(1) when shrinking.
     */
    status resize(size_type count, CharT value = CharT()) CASTLE_NOEXCEPT
    {
        if (count > N)
        {
            return status::full;
        }
        if (count > size_)
        {
            for (size_type i = size_; i < count; ++i)
            {
                data_[i] = value;
            }
        }
        size_ = count;
        data_[size_] = CharT();
        return status::ok;
    }

    /**
     * @brief Finds the first occurrence of a character.
     * @param value Character to search for.
     * @param offset Starting index.
     * @return Matching index or `view_type::npos` when not found.
     * @note Complexity is O(size() - offset).
     */
    size_type find(CharT value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).find(value, offset);
    }

    /**
     * @brief Finds the first occurrence of a substring.
     * @param value Substring to search for.
     * @param offset Starting index.
     * @return Matching index or `view_type::npos` when not found.
     * @note Complexity is delegated to `basic_string_view::find()`.
     */
    size_type find(view_type value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).find(value, offset);
    }

    /**
     * @brief Lexicographically compares the string with another character range.
     * @param other String view to compare with.
     * @return Negative, zero, or positive according to lexicographic ordering.
     * @note Complexity is O(min(size(), other.size())).
     */
    int compare(view_type other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).compare(other);
    }

    /**
     * @brief Returns a non-owning view of the current contents.
     * @return `basic_string_view<CharT>` spanning `[data(), data() + size())`.
     */
    view_type view() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_);
    }

private:
    /**
     * @brief Copies characters from the source to the destination.
     * @param dst Destination buffer.
     * @param src Source buffer.
     */
    static void copy_chars(pointer dst, const_pointer src, size_type count) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < count; ++i)
        {
            dst[i] = src[i];
        }
    }

    CharT data_[N + 1U];
    size_type size_;
};

/**
 * @brief Narrow-character fixed-capacity string alias.
 * @tparam N Maximum number of stored characters excluding the terminator.
 */
template <size_type N>
using string = basic_string<char, N>;

/**
 * @brief UTF-8 byte-string alias backed by `char`.
 * @tparam N Maximum number of stored bytes excluding the terminator.
 */
template <size_type N>
using u8string = basic_string<char, N>;

/**
 * @brief Tests two strings for equality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when both strings contain the same character sequence.
 */
template <typename CharT, size_type N>
bool operator==(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs.view()) == 0;
}

/**
 * @brief Tests two strings for inequality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when the strings differ.
 */
template <typename CharT, size_type N>
bool operator!=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Tests a string and a string view for equality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string view.
 * @return `true` when both ranges contain the same character sequence.
 */
template <typename CharT, size_type N>
bool operator==(CASTLE_CONST basic_string<CharT, N>& lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) == 0;
}

/**
 * @brief Tests a string view and a string for equality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string view.
 * @param rhs Right-hand string.
 * @return `true` when both ranges contain the same character sequence.
 */
template <typename CharT, size_type N>
bool operator==(basic_string_view<CharT> lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return rhs == lhs;
}

/**
 * @brief Tests a string and a string view for inequality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string view.
 * @return `true` when the ranges differ.
 */
template <typename CharT, size_type N>
bool operator!=(CASTLE_CONST basic_string<CharT, N>& lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Tests a string view and a string for inequality.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string view.
 * @param rhs Right-hand string.
 * @return `true` when the ranges differ.
 */
template <typename CharT, size_type N>
bool operator!=(basic_string_view<CharT> lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Lexicographically orders two strings.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when `lhs` compares less than `rhs`.
 */
template <typename CharT, size_type N>
bool operator<(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs.view()) < 0;
}

/**
 * @brief Lexicographically orders two strings.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when `lhs` compares greater than `rhs`.
 */
template <typename CharT, size_type N>
bool operator>(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Tests whether `lhs` is lexicographically less than or equal to `rhs`.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when `lhs` is not greater than `rhs`.
 */
template <typename CharT, size_type N>
bool operator<=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Tests whether `lhs` is lexicographically greater than or equal to `rhs`.
 * @tparam CharT Character type.
 * @tparam N String capacity.
 * @param lhs Left-hand string.
 * @param rhs Right-hand string.
 * @return `true` when `lhs` is not less than `rhs`.
 */
template <typename CharT, size_type N>
bool operator>=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_STRING_HPP
