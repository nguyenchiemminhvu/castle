#pragma once

#include "castle/types/traits.h"

#include <array>
#include <cstddef>

using namespace castle::types;

namespace castle
{
namespace design_patterns
{

template <typename... Types>
class observer;

template <typename T>
class observer<T>
{
public:
    virtual ~observer() = default;
    virtual void notify(const T& data) = 0;
};

template <>
class observer<void>
{
public:
    virtual ~observer() = default;
    virtual void notify() = 0;
};

template <typename T, typename... Rest>
class observer<T, Rest...> : public observer<T>, public observer<Rest...>
{
    static_assert(has_unique_types_v<T, Rest...>, "Observer types must be unique.");

public:
    using observer<T>::notify;
    using observer<Rest...>::notify;
};

template <typename TOberver, std::size_t N>
class observable
{
public:
    bool add_observer(TOberver* observer)
    {
        if (observer == nullptr)
        {
            return false;
        }

        if (observer_count_ >= N)
        {
            return false; // Maximum number of observers reached
        }

        std::size_t first_empty_slot = N;
        for (std::size_t i = 0; i < N; ++i)
        {
            if (observers_[i] == nullptr)
            {
                if (first_empty_slot == N)
                {
                    first_empty_slot = i;
                }
            }
            else if (observers_[i] == observer)
            {
                return false; // Observer already exists
            }
        }

        observers_[first_empty_slot] = observer;
        ++observer_count_;
        return true;
    }

    bool remove_observer(TOberver* observer)
    {
        if (observer == nullptr)
        {
            return false;
        }

        for (auto& obs : observers_)
        {
            if (obs == observer)
            {
                obs = nullptr;
                --observer_count_;
                return true;
            }
        }
        return false;
    }

    template <typename TObserverType>
    void notify_observers(const TObserverType& data)
    {
        for (const auto& observer : observers_)
        {
            if (observer != nullptr)
            {
                observer->notify(data);
            }
        }
    }

    void notify_observers()
    {
        for (const auto& observer : observers_)
        {
            if (observer != nullptr)
            {
                observer->notify();
            }
        }
    }

private:
    std::array<TOberver*, N> observers_{};
    std::size_t observer_count_ = 0;
};

} // namespace design_patterns
} // namespace castle