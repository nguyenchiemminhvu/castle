// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file callback_dispatcher.hpp
 * @brief Runs a stored callback in place while letting that callback replace or clear itself safely.
 *
 * Invoking a `castle::callbacks::function` in place keeps the state of a mutable callable between calls,
 * but destroying or overwriting the callable while it runs would free its captures mid-call. The dispatcher
 * records which callback is running and defers any `replace()` or `clear()` aimed at it until it returns.
 * An owner needs one dispatcher however many callbacks it stores, because only one runs at a time.
 */
#ifndef CASTLE_EXT_COMM_NET_DETAIL_CALLBACK_DISPATCHER_HPP
#define CASTLE_EXT_COMM_NET_DETAIL_CALLBACK_DISPATCHER_HPP

#include "castle/core/compiler.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace net
{
namespace detail
{

/** @tparam Callback A `castle::callbacks::function` specialization. */
template <typename Callback>
class callback_dispatcher
{
public:
    callback_dispatcher() CASTLE_DEFAULT;
    callback_dispatcher(CASTLE_CONST callback_dispatcher&) CASTLE_DELETE;
    callback_dispatcher& operator=(CASTLE_CONST callback_dispatcher&) CASTLE_DELETE;

    /** @brief True while any callback is executing; owners reject re-entrant processing with it. */
    CASTLE_NODISCARD bool is_dispatching() CASTLE_CONST CASTLE_NOEXCEPT { return running_ != nullptr; }

    /** @brief True while `target` is the callback executing. */
    CASTLE_NODISCARD bool is_running(Callback CASTLE_CONST& target) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return running_ == &target;
    }

    /** @brief Stores `next` in `target`, or after the call returns when `target` is executing. */
    void replace(Callback& target, Callback&& next)
    {
        if (is_running(target))
        {
            pending_ = castle::move(next);
            action_ = deferred::replace;
        }
        else
        {
            target = castle::move(next);
        }
    }

    /** @brief Empties `target`, or after the call returns when `target` is executing. */
    void clear(Callback& target) CASTLE_NOEXCEPT
    {
        if (is_running(target))
        {
            pending_ = nullptr;
            action_ = deferred::clear;
        }
        else
        {
            target = nullptr;
        }
    }

    /** @brief Calls `target` in place, then applies the replace or clear it requested for itself. */
    template <typename... Args>
    void invoke(Callback& target, Args&&... args)
    {
        running_ = &target;
        action_ = deferred::none;
        target(CASTLE_FORWARD<Args>(args)...);
        running_ = nullptr;

        if (action_ == deferred::replace)
        {
            target = castle::move(pending_);
        }
        else if (action_ == deferred::clear)
        {
            target = nullptr;
        }
        action_ = deferred::none;
    }

private:
    enum class deferred : uint8_t
    {
        none,
        replace,
        clear
    };

    Callback CASTLE_CONST* running_ = nullptr;
    Callback pending_;
    deferred action_ = deferred::none;
};

} // namespace detail
} // namespace net
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_NET_DETAIL_CALLBACK_DISPATCHER_HPP
