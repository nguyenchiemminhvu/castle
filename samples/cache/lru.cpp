/*
 * Coin Change
 *
 * You are given an integer array `coins` representing coins of different denominations and an
 * integer `amount` representing a total amount of money.
 *
 * Return the fewest number of coins that you need to make up that amount. If that amount of money
 * cannot be made up by any combination of the coins, return -1.
 *
 * You may assume that you have an infinite number of each kind of coin.
 *
 * Example 1: coins = [1,2,5], amount = 11 -> 3 (11 = 5 + 5 + 1)
 * Example 2: coins = [2], amount = 3 -> -1
 * Example 3: coins = [1], amount = 0 -> 0
 *
 * Constraints:
 * - 1 <= coins.length <= 12
 * - 1 <= coins[i] <= 2^31 - 1
 * - 0 <= amount <= 10^4
 */
#include "sample_support.hpp"

#include "castle/cache/lru.hpp"

namespace
{
// Result for an amount that no coin combination can make.
constexpr int32_t unreachable = -1;

// Memo of amount -> fewest coins, bounded to N entries; evicted amounts are simply recomputed.
template <castle::size_type N>
using memo_type = castle::cache::lru<uint32_t, int32_t, N>;

// Top-down recursion with memoization, mirroring F(i) in the reference solution.
template <castle::size_type N>
int32_t fewest_coins(const uint32_t* coins, castle::size_type count, uint32_t amount, memo_type<N>& memo)
{
    if (amount == 0U)
    {
        return 0;
    }
    if (const int32_t* cached = memo.get(amount))
    {
        return *cached;
    }

    int32_t best = unreachable;
    for (castle::size_type i = 0U; i < count; ++i)
    {
        if (coins[i] > amount)
        {
            continue;
        }
        const int32_t rest = fewest_coins(coins, count, amount - coins[i], memo);
        if ((rest != unreachable) && ((best == unreachable) || (rest + 1 < best)))
        {
            best = rest + 1;
        }
    }
    memo.put(amount, best);
    return best;
}

// The memo lives only for one call, as in the reference solution.
template <castle::size_type N, castle::size_type K>
int32_t coin_change(const uint32_t (&coins)[K], uint32_t amount)
{
    memo_type<N> memo;
    return fewest_coins(coins, K, amount, memo);
}
} // namespace

int main()
{
    const uint32_t coins1[] = {1U, 2U, 5U};
    const uint32_t coins2[] = {2U};
    const uint32_t coins3[] = {1U};

    // A 16-entry memo holds every amount up to 11, so nothing is evicted.
    CASTLE_SAMPLE_CHECK(coin_change<16U>(coins1, 11U) == 3);
    CASTLE_SAMPLE_CHECK(coin_change<16U>(coins2, 3U) == -1);
    CASTLE_SAMPLE_CHECK(coin_change<16U>(coins3, 0U) == 0);

    // A 4-entry memo evicts most amounts; the answer is the same, only more subproblems are recomputed.
    CASTLE_SAMPLE_CHECK(coin_change<4U>(coins1, 11U) == 3);

    return 0;
}
