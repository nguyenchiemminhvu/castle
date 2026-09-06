#ifndef CASTLE_CONTAINER_ARRAY_VIEW_H
#define CASTLE_CONTAINER_ARRAY_VIEW_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"

namespace castle
{
namespace container
{

// Non-owning view over contiguous elements. The referenced storage must outlive
// the view. A view never allocates and performs no ownership management.
template <typename T>
class array_view
{
public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;

    array_view() CASTLE_NOEXCEPT : data_(nullptr), size_(0U) {}

    array_view(pointer data, size_type size) CASTLE_NOEXCEPT
        : data_(data), size_(size)
    {
    }

    template <typename Container>
    explicit array_view(Container& container) CASTLE_NOEXCEPT
        : data_(container.data()), size_(container.size())
    {
    }

    template <typename Container>
    explicit array_view(CASTLE_CONST Container& container) CASTLE_NOEXCEPT
        : data_(container.data()), size_(container.size())
    {
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }
    reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }
    reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }
    pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? data_ : data_ + size_; }

    array_view subview(size_type offset, size_type count) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (offset > size_)
        {
            return array_view();
        }
        CASTLE_CONST size_type available = size_ - offset;
        if (count > available)
        {
            count = available;
        }
        return array_view(
            count == 0U
            ? data_
            : data_ + offset, count
        );
    }

private:
    pointer data_;
    size_type size_;
};

template <typename T>
array_view<T> make_array_view(T* data, size_type size) CASTLE_NOEXCEPT
{
    return array_view<T>(data, size);
}

template <typename T>
array_view<CASTLE_CONST T> make_array_view(CASTLE_CONST T* data, size_type size) CASTLE_NOEXCEPT
{
    return array_view<CASTLE_CONST T>(data, size);
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_ARRAY_VIEW_H
