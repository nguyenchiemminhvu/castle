#include "sample_support.hpp"

#include "castle/core/status.hpp"
#include "castle/dsa/t_digest.hpp"

int main()
{
    using castle::status;

    // 1) GNSS output jitter: |arrival - expected| of every 100 ms message, in milliseconds.
    //    One hour of messages (36000) summarized in about 1 KiB, without storing a single one.
    castle::dsa::t_digest<float, 64U> jitter;
    for (uint32_t message = 0U; message < 36000U; ++message)
    {
        float amplitude_ms = 0.1F * static_cast<float>((message * 7U) % 20U); // 0.0 .. 1.9 ms, evenly spread
        if ((message % 33U) == 0U)
        {
            amplitude_ms = 10.0F; // about 3 % late messages: the receiver was busy
        }
        CASTLE_SAMPLE_CHECK(jitter.add(amplitude_ms) == status::ok);
    }

    const float cep50 = jitter.quantile(0.50F);
    const float cep68 = jitter.quantile(0.68F);
    const float cep95 = jitter.quantile(0.95F);
    const float cep99 = jitter.quantile(0.99F);
    CASTLE_SAMPLE_CHECK(cep50 > 0.9F && cep50 < 1.1F); // half the messages within ~1 ms
    CASTLE_SAMPLE_CHECK(cep68 > 1.2F && cep68 < 1.5F);
    CASTLE_SAMPLE_CHECK(cep95 > 1.8F && cep95 < 2.0F);
    CASTLE_SAMPLE_CHECK(cep99 > 9.5F);                  // the late messages dominate the 99 % bound
    CASTLE_SAMPLE_CHECK(jitter.max() == 10.0F);         // the extremes are exact

    const float within_2ms = jitter.cdf(2.0F); // fraction of messages that meet a 2 ms budget (truly 97 %)
    CASTLE_SAMPLE_CHECK(within_2ms > 0.95F && within_2ms < 0.98F);

    // 2) Per-minute digests from two receivers, combined into one report with merge().
    castle::dsa::t_digest<float, 32U> primary;
    castle::dsa::t_digest<float, 32U> backup;
    for (uint32_t message = 0U; message < 600U; ++message)
    {
        CASTLE_SAMPLE_CHECK(primary.add(0.5F + 0.001F * static_cast<float>(message)) == status::ok); // 0.5 .. 1.1 ms
        CASTLE_SAMPLE_CHECK(backup.add(1.5F + 0.001F * static_cast<float>(message)) == status::ok);  // 1.5 .. 2.1 ms
    }
    castle::dsa::t_digest<float, 32U> report;
    CASTLE_SAMPLE_CHECK(report.merge(primary) == status::ok);
    CASTLE_SAMPLE_CHECK(report.merge(backup) == status::ok);
    CASTLE_SAMPLE_CHECK(report.total() == 1200U);
    CASTLE_SAMPLE_CHECK(report.min() == 0.5F);
    CASTLE_SAMPLE_CHECK(report.quantile(0.25F) < 1.1F); // lower half from the primary receiver
    CASTLE_SAMPLE_CHECK(report.quantile(0.75F) > 1.5F); // upper half from the backup receiver

    return 0;
}
