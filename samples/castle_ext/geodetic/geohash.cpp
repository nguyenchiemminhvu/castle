#include "sample_support.hpp"

#include "castle_ext/geodetic/geohash.hpp"

int main()
{
    using namespace castle::geodetic;

    using castle::container::string_view;
    const geo_point<double> da_nang(16.066389083862305, 108.21378707885742);

    // Precision is a type parameter; encode() stores every form of the same cell.
    geohash<8U> hash;
    CASTLE_SAMPLE_CHECK(hash.empty());
    CASTLE_SAMPLE_CHECK(hash.encode(da_nang) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(hash.base32().view().compare(string_view("w6ugqkyc")) == 0);
    CASTLE_SAMPLE_CHECK(hash.binary().view().compare(string_view("1110000110110100111110110100101111001011")) == 0);
    CASTLE_SAMPLE_CHECK(hash.bits() == 969404009419ULL);

    // Center and bounds are stored with the text forms; errors match the Python lat_err/lon_err.
    CASTLE_SAMPLE_CHECK(hash.bounds().min_latitude < da_nang.latitude());
    CASTLE_SAMPLE_CHECK(hash.bounds().max_latitude > da_nang.latitude());
    CASTLE_SAMPLE_CHECK(hash.bounds().min_longitude < da_nang.longitude());
    CASTLE_SAMPLE_CHECK(hash.bounds().max_longitude > da_nang.longitude());
    CASTLE_SAMPLE_CHECK(hash.center().latitude() == hash.bounds().center_latitude());
    CASTLE_SAMPLE_CHECK(hash.bounds().latitude_error() == 8.58306884765625e-05);
    CASTLE_SAMPLE_CHECK(hash.bounds().longitude_error() == 0.000171661376953125);

    // Base-32, binary, and packed-bit inputs each initialize the same cell.
    geohash<8U> from_base32;
    CASTLE_SAMPLE_CHECK(from_base32.assign(string_view("W6UGQKYC")) == castle::status::ok);
    geohash<8U> from_binary;
    CASTLE_SAMPLE_CHECK(from_binary.assign_binary(hash.binary().view()) == castle::status::ok);
    geohash<8U> from_bits;
    CASTLE_SAMPLE_CHECK(from_bits.assign_bits(hash.bits()) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_base32.base32() == hash.base32());
    CASTLE_SAMPLE_CHECK(from_binary.bits() == hash.bits());
    CASTLE_SAMPLE_CHECK(from_bits.center() == hash.center());

    // A rejected initialization keeps the previous value.
    CASTLE_SAMPLE_CHECK(hash.assign_bits(1ULL << 40U) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(hash.base32().view().compare(string_view("w6ugqkyc")) == 0);

    // Python reference: decode tnd0c8k1 gives lat 36.695966720581055, lon 47.88408279418945.
    geohash<8U> reference;
    CASTLE_SAMPLE_CHECK(reference.assign("tnd0c8k1") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(reference.center().latitude() == 36.695966720581055);
    CASTLE_SAMPLE_CHECK(reference.center().longitude() == 47.88408279418945);

    // Stateless raw-buffer encode and static decode use the depth from the type.
    char raw_hash[geohash<12U>::depth + 1U];
    CASTLE_SAMPLE_CHECK(geohash<12U>::encode(da_nang, raw_hash, sizeof(raw_hash)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(raw_hash[geohash<12U>::depth] == '\0');

    geo_point<double> raw_center;
    CASTLE_SAMPLE_CHECK(geohash<12U>::decode(string_view(raw_hash), raw_center) == castle::status::ok);

    // Each depth is a distinct compile-time type, and twelve is the maximum.
    geohash<9U> default_for_application;
    CASTLE_SAMPLE_CHECK(default_for_application.encode(da_nang) == castle::status::ok);
    geohash<12U> maximum_depth;
    CASTLE_SAMPLE_CHECK(maximum_depth.encode(da_nang) == castle::status::ok);

    // The second template parameter selects the point and bounds value type.
    using float_geohash = geohash<8U, float>;
    const float_geohash::point_type float_position(16.0664F, 108.2138F);
    float_geohash float_hash;
    CASTLE_SAMPLE_CHECK(float_hash.encode(float_position) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(float_hash.bounds().min_latitude <= float_position.latitude());
    CASTLE_SAMPLE_CHECK(float_position.latitude() <= float_hash.bounds().max_latitude);
    CASTLE_SAMPLE_CHECK(float_hash.bounds().min_longitude <= float_position.longitude());
    CASTLE_SAMPLE_CHECK(float_position.longitude() <= float_hash.bounds().max_longitude);

    return 0;
}
