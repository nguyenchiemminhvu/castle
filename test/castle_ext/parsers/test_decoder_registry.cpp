#include <gtest/gtest.h>

#include "castle_ext/parsers/decoder_registry.hpp"

namespace
{

struct raw_message
{
    int id = 0;
    int value = 0;
};

struct accepted_message
{
    int value = 0;

    static bool matches(const raw_message& raw) noexcept
    {
        return raw.id == 1;
    }

    static bool decode(const raw_message& raw, accepted_message& out) noexcept
    {
        out.value = raw.value;
        return true;
    }
};

struct rejected_message
{
    int value = 0;

    static bool matches(const raw_message& raw) noexcept
    {
        return raw.id == 1;
    }

    static bool decode(const raw_message&, rejected_message&) noexcept
    {
        return false;
    }
};

using accepted_decoder = castle::parsers::message_decoder<accepted_message, raw_message>;
using rejected_decoder = castle::parsers::message_decoder<rejected_message, raw_message>;
using registry_type = castle::parsers::decoder_registry<raw_message, accepted_decoder, rejected_decoder>;

TEST(DecoderRegistryMirror, MessageDecoderConnectHandleAndSubscriberCount)
{
    accepted_decoder decoder;
    int hits = 0;
    int value = 0;
    auto connection = decoder.connect([&](const accepted_message& message)
                                      {
                                          ++hits;
                                          value = message.value;
                                      });
    EXPECT_TRUE(connection.connected());
    EXPECT_EQ(decoder.subscriber_count(), 1U);

    raw_message raw{1, 42};
    EXPECT_TRUE(decoder.matches(raw));
    EXPECT_TRUE(decoder.handle(raw));
    EXPECT_EQ(hits, 1);
    EXPECT_EQ(value, 42);

    raw.id = 2;
    EXPECT_FALSE(decoder.matches(raw));
    EXPECT_EQ(hits, 1);
}

TEST(DecoderRegistryMirror, RegistryReportsMatchedAndDecodedSeparately)
{
    registry_type registry;
    int accepted_hits = 0;
    int rejected_hits = 0;
    auto accepted_connection = registry.decoder<accepted_message>().connect([&](const accepted_message&) { ++accepted_hits; });
    auto rejected_connection = registry.decoder<rejected_message>().connect([&](const rejected_message&) { ++rejected_hits; });

    EXPECT_EQ(registry_type::decoder_count(), 2U);
    EXPECT_EQ(&registry.decoder_at<0U>(), &registry.decoder<accepted_message>());

    raw_message raw{1, 7};
    auto result = registry.dispatch(raw);
    EXPECT_EQ(result.matched, 2U);
    EXPECT_EQ(result.decoded, 1U);
    EXPECT_EQ(accepted_hits, 1);
    EXPECT_EQ(rejected_hits, 0);

    raw.id = 2;
    result = registry.dispatch(raw);
    EXPECT_EQ(result.matched, 0U);
    EXPECT_EQ(result.decoded, 0U);
}

} // namespace
