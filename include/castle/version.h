#ifndef CASTLE_VERSION_H
#define CASTLE_VERSION_H

/**
 * @file    version.h
 * @brief   CASTLE library version information.
 *
 * Semantic Versioning 2.0.0 (https://semver.org):
 *   MAJOR - incompatible API changes
 *   MINOR - backward-compatible functionality
 *   PATCH - backward-compatible bug fixes
 */

#define CASTLE_VERSION_MAJOR 0
#define CASTLE_VERSION_MINOR 1
#define CASTLE_VERSION_PATCH 0

/* Encoded as 0xMMmmpp (8 bits per field) for cheap numeric comparison. */
#define CASTLE_VERSION_ENCODE(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

#define CASTLE_VERSION \
    CASTLE_VERSION_ENCODE(CASTLE_VERSION_MAJOR, CASTLE_VERSION_MINOR, CASTLE_VERSION_PATCH)

/* Stringification helpers (two-level expansion required by the preprocessor). */
#define CASTLE_STRINGIFY_IMPL(x) #x
#define CASTLE_STRINGIFY(x)      CASTLE_STRINGIFY_IMPL(x)

#define CASTLE_VERSION_STRING           \
    CASTLE_STRINGIFY(CASTLE_VERSION_MAJOR) "." \
    CASTLE_STRINGIFY(CASTLE_VERSION_MINOR) "." \
    CASTLE_STRINGIFY(CASTLE_VERSION_PATCH)

/* Usage:
 *   #if CASTLE_VERSION_AT_LEAST(1, 0, 0)
 *       // code that requires CASTLE >= 1.0.0
 *   #endif
 */
#define CASTLE_VERSION_AT_LEAST(major, minor, patch) \
    (CASTLE_VERSION >= CASTLE_VERSION_ENCODE(major, minor, patch))

namespace castle
{
    static constexpr int version_encoded = CASTLE_VERSION;
    static constexpr int version_major = CASTLE_VERSION_MAJOR;
    static constexpr int version_minor = CASTLE_VERSION_MINOR;
    static constexpr int version_patch = CASTLE_VERSION_PATCH;
}

#endif /* CASTLE_VERSION_H */
