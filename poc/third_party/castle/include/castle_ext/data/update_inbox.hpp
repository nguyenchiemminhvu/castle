// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file update_inbox.hpp
 * @brief Thread-safe accumulator of update notifications for one consumer thread.
 *
 * An inbox is the receiving end of data-layer notifications. It is a
 * `castle::patterns::observer` of `update_set<Ts...>`, so it can be attached to a
 * `data_pool` or to any `castle::patterns::observable` that forwards update sets.
 * `notify()` may run in a producer's context (HAL thread, ISR-deferred task); it only merges the
 * set into atomic words and, when the inbox goes from idle to pending, calls an optional wake
 * hook such as a semaphore release. The consumer thread drains everything with `take()`.
 *
 * @code
 * struct imu {};
 * struct gnss {};
 *
 * castle::semaphore<1> wake_up(0U);
 * castle_ext::data::update_inbox<imu, gnss> inbox{[&wake_up]() noexcept
 * {
 *     (void)wake_up.release();
 * }};
 *
 * // Producer context
 * inbox.notify(castle_ext::data::update_set<imu, gnss>::of<imu>());
 *
 * // Consumer thread
 * wake_up.acquire();
 * castle_ext::data::update_set<imu, gnss> changed = inbox.take();   // { imu }
 * @endcode
 */
#ifndef CASTLE_EXT_DATA_UPDATE_INBOX_HPP
#define CASTLE_EXT_DATA_UPDATE_INBOX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/patterns/observer.hpp"
#include "castle/sync/atomic.hpp"
#include "castle_ext/data/update_set.hpp"

namespace castle_ext
{
namespace data
{

/**
 * @brief Lock-free, coalescing mailbox of `update_set<Ts...>` notifications.
 *
 * @tparam Ts Data types of the update set, in the same order as the producing pool.
 *
 * Notifications are coalesced: several notifications of the same type before `take()` report
 * that type once. The consumer then reads the latest value from the data pool, so no update is
 * lost; only intermediate values are skipped.
 *
 * @note `notify()` and `has_pending()` are safe from any number of threads. `take()` is meant
 *       for the single consumer thread that owns the inbox.
 */
template <typename... Ts>
class update_inbox CASTLE_FINAL : public castle::patterns::observer<update_set<Ts...>>
{
public:
    using set_type = update_set<Ts...>;
    using wake_callback = castle::callbacks::function<void()>;

    /** @brief Constructs an idle inbox without wake hook; the consumer polls `has_pending()`. */
    update_inbox() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs an idle inbox with a wake hook.
     * @param wake Called by `notify()` when the inbox turns from idle to pending. It runs in the
     *             notifier's context and must be short and thread-safe (e.g. a semaphore release).
     */
    explicit update_inbox(wake_callback wake) CASTLE_NOEXCEPT
        : wake_(CASTLE_MOVE(wake))
    {
    }

    /**
     * @brief Binds or replaces the wake hook after construction.
     * @note Not synchronized: call it before the inbox is attached to a pool or observable.
     */
    void set_wake(wake_callback wake) CASTLE_NOEXCEPT
    {
        wake_ = CASTLE_MOVE(wake);
    }

    update_inbox(CASTLE_CONST update_inbox&) CASTLE_DELETE;
    update_inbox& operator=(CASTLE_CONST update_inbox&) CASTLE_DELETE;

    /**
     * @brief Adds `updates` to the pending set.
     * @param updates Types that carry new data.
     * @note Lock-free: one atomic OR per non-empty storage word. The wake hook runs when a
     *       storage word turns from empty to non-empty, so a consumer that waits on it never
     *       misses pending updates. With more than 32 types an extra wake-up is possible.
     */
    void notify(CASTLE_CONST set_type& updates) CASTLE_NOEXCEPT CASTLE_OVERRIDE
    {
        CASTLE_CONST word_type* CASTLE_CONST source = updates.bits_.data();
        bool turned_pending = false;

        for (castle::size_type i = 0U; i < word_count; ++i)
        {
            if (source[i] != 0U)
            {
                turned_pending |= (words_[i].fetch_or(source[i], castle::memory_order_acq_rel) == 0U);
            }
        }

        if (turned_pending && wake_)
        {
            wake_();
        }
    }

    /**
     * @brief Removes and returns every pending type.
     * @return Types notified since the previous `take()`; empty when nothing is pending.
     */
    CASTLE_NODISCARD set_type take() CASTLE_NOEXCEPT
    {
        set_type result;
        word_type* CASTLE_CONST destination = result.bits_.data();

        for (castle::size_type i = 0U; i < word_count; ++i)
        {
            destination[i] = words_[i].exchange(0U, castle::memory_order_acq_rel);
        }
        return result;
    }

    /** @brief Returns whether at least one type is pending. */
    CASTLE_NODISCARD bool has_pending() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (castle::size_type i = 0U; i < word_count; ++i)
        {
            if (words_[i].load(castle::memory_order_acquire) != 0U)
            {
                return true;
            }
        }
        return false;
    }

private:
    using bits_type = decltype(set_type{}.bits_);
    using word_type = typename bits_type::word_type;

    static CASTLE_CONSTEXPR castle::size_type word_count = bits_type::number_of_words();

    castle::atomic<word_type> words_[word_count]{};
    wake_callback wake_{};
};

} // namespace data
} // namespace castle_ext

#endif // CASTLE_EXT_DATA_UPDATE_INBOX_HPP
