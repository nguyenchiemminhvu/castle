#include <gtest/gtest.h>

#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"
#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

namespace
{
using namespace castle::parsers::nmea_parser;
using namespace castle::protocols::nmea;
using castle::container::string_view;

TEST(NmeaDecoderRegistryMirror, FacadeAliasesAndAttachDispatchDecodedSentence)
{
    using decoder = gga_decoder<>;
    decoder_registry<decoder> registry;
    int hits = 0;
    auto connection = registry.decoder<castle::protocols::nmea::messages::gga>().connect(
        [&](const castle::protocols::nmea::messages::gga& value)
        {
            ++hits;
            EXPECT_TRUE(value.valid);
            EXPECT_EQ(value.num_satellites, 8U);
        });

    EXPECT_EQ(decoder_registry<decoder>::decoder_count(), 1U);
    nmea_parser<> parser;
    castle::parsers::nmea_parser::attach(parser, registry);
    parser.feed(string_view("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,\r\n"));
    EXPECT_EQ(hits, 1);
}

TEST(NmeaDecoderRegistryMirror, DefaultRegistryHasEveryBuiltInDecoder)
{
    EXPECT_EQ(default_decoder_registry<>::decoder_count(), 11U);
}

} // namespace
