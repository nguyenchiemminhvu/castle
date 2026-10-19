#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/geodetic/geohash.hpp"

namespace
{

using castle::geodetic::geo_point;
using castle::geodetic::geohash;
using castle::geodetic::geohash_bounds;
using castle::geodetic::geohash_max_depth;
using castle::status;

static_assert(geohash<1U>::depth == 1U, "one-character precision must be supported");
static_assert(geohash<8U>::depth == 8U, "eight-character precision must be supported");
static_assert(geohash<12U>::depth == geohash_max_depth, "twelve is the maximum precision");
static_assert(geohash<8U>::depth != geohash<9U>::depth, "different depths are different codec types");
static_assert(castle::meta::is_same<geohash<8U>::point_type, geo_point<double>>::value,
              "the default coordinate type must be double");
static_assert(castle::meta::is_same<geohash<8U, float>::point_type, geo_point<float>>::value,
              "the second template parameter selects the point coordinate type");
static_assert(castle::meta::is_same<geohash<8U, float>::bounds_type, geohash_bounds<float>>::value,
              "bounds use the selected coordinate type");
static_assert(castle::meta::is_same<geohash<12U, float>::point_type, geo_point<float>>::value,
              "the maximum depth also supports a custom coordinate type");
static_assert(castle::meta::is_same<geohash<8U>::bits_type, uint64_t>::value,
              "packed binary Geohash bits use uint64_t");
static_assert(geohash<1U>::bit_count == 5U, "one base-32 character carries five bits");
static_assert(geohash<12U>::bit_count == 60U, "twelve base-32 characters carry sixty bits");
static_assert(castle::meta::is_same<geohash<8U>::binary_string_type, castle::container::string<40U>>::value,
              "binary text capacity equals bit_count");

TEST(GeodeticGeohash, KnownDaNangEncodingStoresEveryForm)
{
    const geo_point<double> da_nang(16.066389083862305, 108.21378707885742);
    geohash<8U> hash;

    EXPECT_TRUE(hash.empty());
    EXPECT_EQ(hash.encode(da_nang), status::ok);
    EXPECT_FALSE(hash.empty());
    EXPECT_STREQ(hash.c_str(), "w6ugqkyc");
    EXPECT_EQ(hash.base32().size(), 8U);
    EXPECT_EQ(hash.base32().capacity(), 8U);
    EXPECT_EQ(hash.binary().view().compare(castle::container::string_view("1110000110110100111110110100101111001011")), 0);
    EXPECT_EQ(hash.bits(), 969404009419ULL);

    const geohash_bounds<double>& bounds = hash.bounds();
    EXPECT_NEAR(hash.center().latitude(), 16.066389083862305, 1.0e-12);
    EXPECT_NEAR(hash.center().longitude(), 108.21378707885742, 1.0e-12);
    EXPECT_NEAR(bounds.min_latitude, 16.066303253173828, 1.0e-12);
    EXPECT_NEAR(bounds.max_latitude, 16.066474914550781, 1.0e-12);
    EXPECT_NEAR(bounds.min_longitude, 108.21361541748047, 1.0e-12);
    EXPECT_NEAR(bounds.max_longitude, 108.21395874023438, 1.0e-12);
    EXPECT_DOUBLE_EQ(bounds.latitude_error(), 8.58306884765625e-05);
    EXPECT_DOUBLE_EQ(bounds.longitude_error(), 0.000171661376953125);
    EXPECT_DOUBLE_EQ(bounds.center_latitude(), hash.center().latitude());
    EXPECT_DOUBLE_EQ(bounds.center_longitude(), hash.center().longitude());
    EXPECT_TRUE(bounds.center() == hash.center());
}

TEST(GeodeticGeohash, CompileTimeDepthAndStaticBufferApi)
{
    const geo_point<double> origin(0.0, 0.0);
    char output[geohash<12U>::depth + 1U] = {};

    EXPECT_EQ(geohash<9U>::encode(origin, output, sizeof(output)), status::ok);
    EXPECT_STREQ(output, "s00000000");

    EXPECT_EQ(geohash<1U>::encode(origin, output, sizeof(output)), status::ok);
    EXPECT_STREQ(output, "s");

    EXPECT_EQ(geohash<12U>::encode(origin, output, sizeof(output)), status::ok);
    EXPECT_EQ(castle::container::string_view(output).size(), 12U);
    EXPECT_EQ(output[12U], '\0');
}

TEST(GeodeticGeohash, ConfigurableFloatingPointType)
{
    using float_hash = geohash<8U, float>;
    const float_hash::point_type position(16.0664F, 108.2138F);
    float_hash hash;

    EXPECT_TRUE(hash.empty());
    EXPECT_EQ(hash.encode(position), status::ok);
    EXPECT_EQ(hash.base32().size(), 8U);
    EXPECT_EQ(hash.binary().size(), 40U);

    const float_hash::bounds_type& bounds = hash.bounds();
    EXPECT_FLOAT_EQ(hash.center().latitude(), bounds.center_latitude());
    EXPECT_FLOAT_EQ(hash.center().longitude(), bounds.center_longitude());
    EXPECT_LE(bounds.min_latitude, position.latitude());
    EXPECT_LE(position.latitude(), bounds.max_latitude);
    EXPECT_LE(bounds.min_longitude, position.longitude());
    EXPECT_LE(position.longitude(), bounds.max_longitude);
    EXPECT_TRUE(bounds.center() == hash.center());

    // A float point can also be encoded and decoded through the static API.
    char output[float_hash::depth + 1U] = {};
    EXPECT_EQ(float_hash::encode(position, output, sizeof(output)), status::ok);
    float_hash::point_type static_center;
    EXPECT_EQ(float_hash::decode(castle::container::string_view(output), static_center), status::ok);
    EXPECT_TRUE(static_center == hash.center());
}

TEST(GeodeticGeohash, DecodeStaticAndInstanceApis)
{
    const castle::container::string_view encoded("w6ugqkyc", 8U);
    geo_point<double> static_center;
    geohash_bounds<double> static_bounds;
    geohash<8U> hash;

    EXPECT_EQ(geohash<8U>::decode(encoded, static_center, &static_bounds), status::ok);
    ASSERT_EQ(hash.assign(encoded), status::ok);
    EXPECT_TRUE(static_center == hash.center());
    EXPECT_DOUBLE_EQ(static_bounds.center_latitude(), static_center.latitude());
    EXPECT_DOUBLE_EQ(static_bounds.center_longitude(), static_center.longitude());
}

TEST(GeodeticGeohash, AssignAcceptsUppercaseAndAllAlphabetSymbols)
{
    geohash<8U> lower;
    geohash<8U> upper;
    ASSERT_EQ(lower.assign("w6ugqkyc"), status::ok);
    ASSERT_EQ(upper.assign("W6UGQKYC"), status::ok);

    EXPECT_TRUE(lower.center() == upper.center());
    EXPECT_EQ(lower.bits(), upper.bits());
    EXPECT_STREQ(upper.c_str(), "w6ugqkyc");

    const char alphabet[] = "0123456789bcdefghjkmnpqrstuvwxyz";
    for (castle::size_type i = 0U; i < 32U; ++i)
    {
        geohash<1U> single_character;
        EXPECT_EQ(single_character.assign(castle::container::string_view(alphabet + i, 1U)), status::ok)
            << "alphabet index " << i;
        EXPECT_EQ(single_character.bits(), static_cast<geohash<1U>::bits_type>(i)) << "alphabet index " << i;
    }
}

TEST(GeodeticGeohash, EncodeBoundaryCoordinates)
{
    const geo_point<double> north_east(90.0, 180.0);
    const geo_point<double> south_west(-90.0, -180.0);
    geohash<12U> north_hash;
    geohash<12U> south_hash;

    ASSERT_EQ(north_hash.encode(north_east), status::ok);
    ASSERT_EQ(south_hash.encode(south_west), status::ok);
    EXPECT_NE(north_hash.view().compare(south_hash.view()), 0);
    EXPECT_EQ(north_hash.bits(), (1ULL << 60U) - 1U);
    EXPECT_EQ(south_hash.bits(), 0U);

    const geohash_bounds<double>& north_bounds = north_hash.bounds();
    const geohash_bounds<double>& south_bounds = south_hash.bounds();
    EXPECT_DOUBLE_EQ(north_bounds.max_latitude, 90.0);
    EXPECT_DOUBLE_EQ(north_bounds.max_longitude, 180.0);
    EXPECT_DOUBLE_EQ(south_bounds.min_latitude, -90.0);
    EXPECT_DOUBLE_EQ(south_bounds.min_longitude, -180.0);
    EXPECT_LE(north_hash.center().latitude(), 90.0);
    EXPECT_LE(north_hash.center().longitude(), 180.0);
    EXPECT_GE(south_hash.center().latitude(), -90.0);
    EXPECT_GE(south_hash.center().longitude(), -180.0);
}

TEST(GeodeticGeohash, RawBufferReportsNullAndCapacityErrors)
{
    const geo_point<double> valid_point(16.0, 108.0);
    char small_buffer[8] = "KEEP";

    EXPECT_EQ(geohash<8U>::encode(valid_point, static_cast<char*>(nullptr), 9U), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::encode(valid_point, small_buffer, sizeof(small_buffer)), status::full);
    EXPECT_STREQ(small_buffer, "KEEP");
}

TEST(GeodeticGeohash, AssignReportsEmptyLengthAndAlphabetErrors)
{
    geohash<8U> hash;
    ASSERT_EQ(hash.assign("w6ugqkyc"), status::ok);

    EXPECT_EQ(hash.assign(castle::container::string_view()), status::empty);
    EXPECT_EQ(hash.assign(static_cast<const char*>(nullptr)), status::invalid_argument);
    EXPECT_EQ(hash.assign(castle::container::string_view(nullptr, 8U)), status::invalid_argument);
    EXPECT_EQ(hash.assign("w6ugqky"), status::invalid_argument);
    EXPECT_EQ(hash.assign("w6ugqkyc0"), status::out_of_range);
    EXPECT_EQ(hash.assign("w6ugqkyy"), status::ok);
    EXPECT_EQ(hash.assign("w6ugqkyi"), status::invalid_argument);
    EXPECT_EQ(hash.assign("w6ugqkyl"), status::invalid_argument);
    EXPECT_EQ(hash.assign("w6ugqkyo"), status::invalid_argument);
    EXPECT_EQ(hash.assign("w6ugqky?"), status::invalid_argument);
    EXPECT_STREQ(hash.c_str(), "w6ugqkyy");
}

TEST(GeodeticGeohash, DecodeRejectsEmptyAndMismatchedDepthWithoutChangingOutputs)
{
    geo_point<double> center(12.0, 34.0);
    geohash_bounds bounds;
    bounds.min_latitude = -1.0;
    bounds.max_latitude = 1.0;
    bounds.min_longitude = -2.0;
    bounds.max_longitude = 2.0;

    EXPECT_EQ(geohash<8U>::decode(castle::container::string_view(), center, &bounds), status::empty);
    EXPECT_EQ(geohash<8U>::decode(castle::container::string_view(nullptr, 8U), center, &bounds), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::decode(castle::container::string_view("w6ugqky", 7U), center, &bounds), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::decode(castle::container::string_view("w6ugqkyc0", 9U), center, &bounds), status::out_of_range);
    EXPECT_EQ(geohash<8U>::decode(castle::container::string_view("w6ugqky?", 8U), center, &bounds), status::invalid_argument);

    geohash<8U> empty_hash;
    EXPECT_TRUE(empty_hash.empty());
    EXPECT_DOUBLE_EQ(center.latitude(), 12.0);
    EXPECT_DOUBLE_EQ(center.longitude(), 34.0);
    EXPECT_DOUBLE_EQ(bounds.min_latitude, -1.0);
    EXPECT_DOUBLE_EQ(bounds.max_latitude, 1.0);
    EXPECT_DOUBLE_EQ(bounds.min_longitude, -2.0);
    EXPECT_DOUBLE_EQ(bounds.max_longitude, 2.0);
}

TEST(GeodeticGeohash, EncodeBitsMatchesReferenceIntegers)
{
    // Reference: python encode_binary(lat, lon, precision) read as a base-2 integer.
    const geo_point<double> da_nang(16.066389083862305, 108.21378707885742);
    geohash<8U>::bits_type bits = 0U;

    ASSERT_EQ(geohash<8U>::encode_bits(da_nang, bits), status::ok);
    EXPECT_EQ(bits, 969404009419ULL);

    ASSERT_EQ(geohash<9U>::encode_bits(geo_point<double>(0.0, 0.0), bits), status::ok);
    EXPECT_EQ(bits, 3ULL << 43U);

    ASSERT_EQ(geohash<12U>::encode_bits(geo_point<double>(90.0, 180.0), bits), status::ok);
    EXPECT_EQ(bits, (1ULL << 60U) - 1U);

    ASSERT_EQ(geohash<12U>::encode_bits(geo_point<double>(-90.0, -180.0), bits), status::ok);
    EXPECT_EQ(bits, 0U);
}

TEST(GeodeticGeohash, EncodeBinaryMatchesReferenceStrings)
{
    geohash<8U>::binary_string_type binary;
    ASSERT_EQ(geohash<8U>::encode_binary(geo_point<double>(16.066389083862305, 108.21378707885742), binary), status::ok);
    EXPECT_EQ(binary.size(), 40U);
    EXPECT_EQ(binary.view().compare(castle::container::string_view("1110000110110100111110110100101111001011")), 0);

    geohash<6U>::binary_string_type sydney;
    ASSERT_EQ(geohash<6U>::encode_binary(geo_point<double>(-33.8688, 151.2093), sydney), status::ok);
    EXPECT_EQ(sydney.view().compare(castle::container::string_view("101110001101111111010001001110")), 0);

    geohash<10U>::binary_string_type london;
    ASSERT_EQ(geohash<10U>::encode_binary(geo_point<double>(51.5074, -0.1278), london), status::ok);
    EXPECT_EQ(london.view().compare(castle::container::string_view("01111010111010111011100010000001100110101011000101")), 0);

    char london_base32[geohash<10U>::depth + 1U] = {};
    ASSERT_EQ(geohash<10U>::encode(geo_point<double>(51.5074, -0.1278), london_base32, sizeof(london_base32)), status::ok);
    EXPECT_STREQ(london_base32, "gcpvj0duq5");
}

template <castle::size_type Depth>
void expect_binary_and_base32_agree(const geo_point<double>& point)
{
    using hash_type = geohash<Depth>;

    char base32[Depth + 1U] = {};
    typename hash_type::binary_string_type binary;
    typename hash_type::binary_string_type binary_from_base32;
    typename hash_type::string_type base32_from_binary;
    typename hash_type::bits_type bits = 0U;

    ASSERT_EQ(hash_type::encode(point, base32, sizeof(base32)), status::ok);
    ASSERT_EQ(hash_type::encode_binary(point, binary), status::ok);
    ASSERT_EQ(hash_type::encode_bits(point, bits), status::ok);

    ASSERT_EQ(hash_type::base32_to_binary(castle::container::string_view(base32), binary_from_base32), status::ok);
    EXPECT_TRUE(binary_from_base32 == binary);

    ASSERT_EQ(hash_type::binary_to_base32(binary.view(), base32_from_binary), status::ok);
    EXPECT_EQ(base32_from_binary.view().compare(castle::container::string_view(base32)), 0);

    geo_point<double> base32_center;
    geo_point<double> binary_center;
    geo_point<double> bits_center;
    geohash_bounds<double> base32_bounds;
    geohash_bounds<double> binary_bounds;

    ASSERT_EQ(hash_type::decode(castle::container::string_view(base32), base32_center, &base32_bounds), status::ok);
    ASSERT_EQ(hash_type::decode_binary(binary.view(), binary_center, &binary_bounds), status::ok);
    ASSERT_EQ(hash_type::decode_bits(bits, bits_center), status::ok);

    EXPECT_TRUE(base32_center == binary_center);
    EXPECT_TRUE(base32_center == bits_center);
    EXPECT_DOUBLE_EQ(base32_bounds.min_latitude, binary_bounds.min_latitude);
    EXPECT_DOUBLE_EQ(base32_bounds.max_latitude, binary_bounds.max_latitude);
    EXPECT_DOUBLE_EQ(base32_bounds.min_longitude, binary_bounds.min_longitude);
    EXPECT_DOUBLE_EQ(base32_bounds.max_longitude, binary_bounds.max_longitude);
}

TEST(GeodeticGeohash, BinaryAndBase32FormsAgreeAcrossAllDepths)
{
    const geo_point<double> points[] = {
        geo_point<double>(16.066389083862305, 108.21378707885742),
        geo_point<double>(0.0, 0.0),
        geo_point<double>(90.0, 180.0),
        geo_point<double>(-90.0, -180.0),
        geo_point<double>(-33.8688, 151.2093),
        geo_point<double>(51.5074, -0.1278)
    };

    for (const geo_point<double>& point : points)
    {
        expect_binary_and_base32_agree<1U>(point);
        expect_binary_and_base32_agree<2U>(point);
        expect_binary_and_base32_agree<3U>(point);
        expect_binary_and_base32_agree<4U>(point);
        expect_binary_and_base32_agree<5U>(point);
        expect_binary_and_base32_agree<6U>(point);
        expect_binary_and_base32_agree<7U>(point);
        expect_binary_and_base32_agree<8U>(point);
        expect_binary_and_base32_agree<9U>(point);
        expect_binary_and_base32_agree<10U>(point);
        expect_binary_and_base32_agree<11U>(point);
        expect_binary_and_base32_agree<12U>(point);
    }
}

TEST(GeodeticGeohash, DecodeBinaryMatchesBase32AndReportsErrors)
{
    const char bits_text[] = "1110000110110100111110110100101111001011";
    geo_point<double> center;
    geohash_bounds bounds;

    ASSERT_EQ(geohash<8U>::decode_binary(castle::container::string_view(bits_text), center, &bounds), status::ok);
    EXPECT_NEAR(center.latitude(), 16.066389083862305, 1.0e-12);
    EXPECT_NEAR(center.longitude(), 108.21378707885742, 1.0e-12);
    EXPECT_NEAR(bounds.min_latitude, 16.066303253173828, 1.0e-12);
    EXPECT_NEAR(bounds.max_latitude, 16.066474914550781, 1.0e-12);
    EXPECT_NEAR(bounds.min_longitude, 108.21361541748047, 1.0e-12);
    EXPECT_NEAR(bounds.max_longitude, 108.21395874023438, 1.0e-12);

    geo_point<double> base32_center;
    ASSERT_EQ(geohash<8U>::decode(castle::container::string_view("w6ugqkyc", 8U), base32_center), status::ok);
    EXPECT_TRUE(center == base32_center);

    geo_point<double> unchanged(12.0, 34.0);
    char bad_bits[sizeof(bits_text)] = {};
    for (castle::size_type i = 0U; i < sizeof(bits_text); ++i)
    {
        bad_bits[i] = bits_text[i];
    }
    bad_bits[39] = '2';

    EXPECT_EQ(geohash<8U>::decode_binary(castle::container::string_view(), unchanged, &bounds), status::empty);
    EXPECT_EQ(geohash<8U>::decode_binary(castle::container::string_view(nullptr, 40U), unchanged, &bounds), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::decode_binary(castle::container::string_view(bits_text, 39U), unchanged, &bounds), status::invalid_argument);
    EXPECT_EQ(geohash<7U>::decode_binary(castle::container::string_view(bits_text, 40U), unchanged, &bounds), status::out_of_range);
    EXPECT_EQ(geohash<8U>::decode_binary(castle::container::string_view(bad_bits, 40U), unchanged, &bounds), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::decode_bits(1ULL << 40U, unchanged, &bounds), status::out_of_range);
    EXPECT_DOUBLE_EQ(unchanged.latitude(), 12.0);
    EXPECT_DOUBLE_EQ(unchanged.longitude(), 34.0);
    EXPECT_DOUBLE_EQ(bounds.min_latitude, 16.066303253173828);
}

TEST(GeodeticGeohash, BinaryConversionsAcceptUppercaseBase32AndRejectInvalidText)
{
    geohash<8U>::binary_string_type binary;
    geohash<8U>::string_type text;

    EXPECT_EQ(geohash<8U>::base32_to_binary(castle::container::string_view("W6UGQKYC"), binary), status::ok);
    EXPECT_EQ(binary.view().compare(castle::container::string_view("1110000110110100111110110100101111001011")), 0);

    EXPECT_EQ(geohash<8U>::binary_to_base32(binary.view(), text), status::ok);
    EXPECT_STREQ(text.c_str(), "w6ugqkyc");

    EXPECT_EQ(geohash<8U>::base32_to_binary(castle::container::string_view("w6ugqky"), binary), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::base32_to_binary(castle::container::string_view("w6ugqkyi"), binary), status::invalid_argument);
    EXPECT_EQ(geohash<8U>::binary_to_base32(castle::container::string_view(), text), status::empty);
    geohash<1U>::string_type single;
    EXPECT_EQ(geohash<1U>::binary_to_base32(castle::container::string_view("01a10", 5U), single), status::invalid_argument);
    EXPECT_TRUE(single.empty());
    EXPECT_STREQ(text.c_str(), "w6ugqkyc");
}

TEST(GeodeticGeohash, ObjectBinaryRoundTripPreservesValueOnFailure)
{
    geohash<8U> hash;
    EXPECT_TRUE(hash.empty());
    EXPECT_EQ(hash.binary().size(), 0U);

    ASSERT_EQ(hash.assign("w6ugqkyc"), status::ok);
    EXPECT_EQ(hash.binary().view().compare(castle::container::string_view("1110000110110100111110110100101111001011")), 0);

    EXPECT_EQ(hash.assign_binary(castle::container::string_view("0101")), status::invalid_argument);
    EXPECT_STREQ(hash.c_str(), "w6ugqkyc");
    EXPECT_EQ(hash.bits(), 969404009419ULL);

    geohash<8U> restored;
    ASSERT_EQ(restored.assign_binary(hash.binary().view()), status::ok);
    EXPECT_EQ(restored.view().compare(hash.view()), 0);
    EXPECT_EQ(restored.assign_binary(castle::container::string_view()), status::empty);
    EXPECT_STREQ(restored.c_str(), "w6ugqkyc");
}

TEST(GeodeticGeohash, AssignBitsAndBinaryInitializeEveryForm)
{
    geohash<8U> from_base32;
    ASSERT_EQ(from_base32.assign("w6ugqkyc"), status::ok);

    geohash<8U> from_binary;
    ASSERT_EQ(from_binary.assign_binary(from_base32.binary().view()), status::ok);

    geohash<8U> from_bits;
    ASSERT_EQ(from_bits.assign_bits(from_base32.bits()), status::ok);

    EXPECT_EQ(from_binary.bits(), from_base32.bits());
    EXPECT_EQ(from_bits.bits(), from_base32.bits());
    EXPECT_TRUE(from_binary.base32() == from_base32.base32());
    EXPECT_TRUE(from_bits.base32() == from_base32.base32());
    EXPECT_TRUE(from_bits.binary() == from_base32.binary());
    EXPECT_TRUE(from_binary.center() == from_base32.center());
    EXPECT_TRUE(from_bits.center() == from_base32.center());
    EXPECT_DOUBLE_EQ(from_bits.bounds().min_latitude, from_base32.bounds().min_latitude);
    EXPECT_DOUBLE_EQ(from_bits.bounds().max_longitude, from_base32.bounds().max_longitude);
}

TEST(GeodeticGeohash, FailedAssignPreservesEveryForm)
{
    geohash<8U> hash;
    ASSERT_EQ(hash.encode(geo_point<double>(16.066389083862305, 108.21378707885742)), status::ok);
    const geohash<8U> before = hash;

    EXPECT_EQ(hash.assign_bits(1ULL << 40U), status::out_of_range);
    EXPECT_EQ(hash.assign("w6ugqky"), status::invalid_argument);
    EXPECT_EQ(hash.assign_binary(castle::container::string_view("0101")), status::invalid_argument);

    EXPECT_EQ(hash.bits(), before.bits());
    EXPECT_TRUE(hash.base32() == before.base32());
    EXPECT_STREQ(before.c_str(), hash.c_str());
    EXPECT_TRUE(hash.binary() == before.binary());
    EXPECT_TRUE(hash.center() == before.center());
    EXPECT_DOUBLE_EQ(hash.bounds().min_latitude, before.bounds().min_latitude);
}

TEST(GeodeticGeohash, MatchesPythonReferenceVectors)
{
    // Reference: geohash.py encode/decode (base32, binary, lat_err, lon_err, bounds).
    geohash<8U> tnd;
    ASSERT_EQ(tnd.assign("tnd0c8k1"), status::ok);
    EXPECT_EQ(tnd.binary().view().compare(castle::container::string_view("1100110100011000000001011010001001000001")), 0);
    EXPECT_DOUBLE_EQ(tnd.center().latitude(), 36.695966720581055);
    EXPECT_DOUBLE_EQ(tnd.center().longitude(), 47.88408279418945);
    EXPECT_DOUBLE_EQ(tnd.bounds().min_latitude, 36.69588088989258);
    EXPECT_DOUBLE_EQ(tnd.bounds().max_latitude, 36.69605255126953);
    EXPECT_DOUBLE_EQ(tnd.bounds().min_longitude, 47.8839111328125);
    EXPECT_DOUBLE_EQ(tnd.bounds().max_longitude, 47.884254455566406);
    EXPECT_DOUBLE_EQ(tnd.bounds().latitude_error(), 8.58306884765625e-05);
    EXPECT_DOUBLE_EQ(tnd.bounds().longitude_error(), 0.000171661376953125);

    geohash<9U> origin;
    ASSERT_EQ(origin.encode(geo_point<double>(0.0, 0.0)), status::ok);
    EXPECT_STREQ(origin.c_str(), "s00000000");
    EXPECT_EQ(origin.binary().view().compare(castle::container::string_view("110000000000000000000000000000000000000000000")), 0);
    EXPECT_DOUBLE_EQ(origin.bounds().max_latitude, 4.291534423828125e-05);
    EXPECT_DOUBLE_EQ(origin.bounds().latitude_error(), 2.1457672119140625e-05);

    geohash<12U> zenith;
    ASSERT_EQ(zenith.encode(geo_point<double>(90.0, 180.0)), status::ok);
    EXPECT_STREQ(zenith.c_str(), "zzzzzzzzzzzz");
    EXPECT_DOUBLE_EQ(zenith.bounds().min_latitude, 89.99999983236194);
    EXPECT_DOUBLE_EQ(zenith.bounds().min_longitude, 179.99999966472387);

    geohash<6U> sydney;
    ASSERT_EQ(sydney.encode(geo_point<double>(-33.8688, 151.2093)), status::ok);
    EXPECT_STREQ(sydney.c_str(), "r3gx2f");
    EXPECT_DOUBLE_EQ(sydney.center().latitude(), -33.86810302734375);
    EXPECT_DOUBLE_EQ(sydney.center().longitude(), 151.2103271484375);
    EXPECT_DOUBLE_EQ(sydney.bounds().latitude_error(), 0.00274658203125);
    EXPECT_DOUBLE_EQ(sydney.bounds().longitude_error(), 0.0054931640625);
}

} // namespace
