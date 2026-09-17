#ifndef CASTLE_CONTAINER_STRING_VIEW_H
#define CASTLE_CONTAINER_STRING_VIEW_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"

namespace castle
{
namespace container
{

template <typename CharT, size_type N>
class basic_string;

// Non-owning string range. No null terminator is required.
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

    basic_string_view() CASTLE_NOEXCEPT : data_(nullptr), size_(0U) {}

    basic_string_view(const_pointer data, size_type size) CASTLE_NOEXCEPT
        : data_(data), size_(size)
    {
    }

    basic_string_view(const_pointer text) CASTLE_NOEXCEPT
        : data_(text), size_(length(text))
    {
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type length() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? data_ : data_ + size_; }

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

    size_type find(CharT value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
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

    size_type find(basic_string_view value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (value.size_ == 0U)
        {
            return offset <= size_ ? offset : npos;
        }
        if (value.size_ > size_ || offset > size_ - value.size_)
        {
            return npos;
        }
        for (size_type i = offset; i <= size_ - value.size_; ++i)
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

    int compare(basic_string_view other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type common = size_ < other.size_
                                        ? size_
                                        : other.size_;
        for (size_type i = 0U; i < common; ++i)
        {
            if (data_[i] < other.data_[i])
            {
                return -1;
            }
            if (other.data_[i] < data_[i])
            {
                return 1;
            }
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

    bool starts_with(basic_string_view prefix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return prefix.size_ <= size_ && substr(0U, prefix.size_).compare(prefix) == 0;
    }

    bool ends_with(basic_string_view suffix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return suffix.size_ <= size_ && substr(size_ - suffix.size_, suffix.size_).compare(suffix) == 0;
    }

    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

private:
    static size_type length(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            return 0U;
        }
        size_type count = 0U;
        while (text[count] != CharT())
        {
            ++count;
        }
        return count;
    }

    const_pointer data_;
    size_type size_;
};

using string_view = basic_string_view<char>;

template <typename CharT>
bool operator==(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) == 0;
}

template <typename CharT>
bool operator!=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename CharT>
bool operator<(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) < 0;
}

template <typename CharT>
bool operator>(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

template <typename CharT>
bool operator<=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

template <typename CharT>
bool operator>=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_STRING_VIEW_H
