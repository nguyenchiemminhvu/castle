// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Castle library version macros and constexpr mirrors.
// Include this header when build scripts, conditional compilation, or runtime
// metadata need to query the locked Castle release number. The interface is
// purely compile-time, follows semantic-version fields, and provides both an
// encoded integer form and a string form without introducing any dynamic state.

#ifndef CASTLE_VERSION_HPP
#define CASTLE_VERSION_HPP

#include "castle/core/compiler.hpp"

/// @def CASTLE_VERSION_MAJOR
/// @brief Castle major version number.
#define CASTLE_VERSION_MAJOR 2
/// @def CASTLE_VERSION_MINOR
/// @brief Castle minor version number.
#define CASTLE_VERSION_MINOR 0
/// @def CASTLE_VERSION_PATCH
/// @brief Castle patch version number.
#define CASTLE_VERSION_PATCH 0

/// @def CASTLE_VERSION_ENCODE
/// @brief Encodes semantic-version fields into a single integer.
/// @param major Major version component.
/// @param minor Minor version component.
/// @param patch Patch version component.
#define CASTLE_VERSION_ENCODE(major, minor, patch)     (((major) << 16) | ((minor) << 8) | (patch))

/// @def CASTLE_VERSION
/// @brief Encoded Castle version for numeric comparisons.
#define CASTLE_VERSION     CASTLE_VERSION_ENCODE(CASTLE_VERSION_MAJOR, CASTLE_VERSION_MINOR, CASTLE_VERSION_PATCH)

/// @def CASTLE_STRINGIFY_IMPL
/// @brief Converts a preprocessor token to a string literal without prior expansion.
/// @param x Token to stringify.
#define CASTLE_STRINGIFY_IMPL(x) #x
/// @def CASTLE_STRINGIFY
/// @brief Converts a preprocessor token to a fully expanded string literal.
/// @param x Token to stringify.
#define CASTLE_STRINGIFY(x)      CASTLE_STRINGIFY_IMPL(x)

/// @def CASTLE_VERSION_STRING
/// @brief Castle version rendered as a semantic-version string literal.
#define CASTLE_VERSION_STRING               CASTLE_STRINGIFY(CASTLE_VERSION_MAJOR) "."     CASTLE_STRINGIFY(CASTLE_VERSION_MINOR) "."     CASTLE_STRINGIFY(CASTLE_VERSION_PATCH)

/// @def CASTLE_VERSION_AT_LEAST
/// @brief Tests whether the current Castle version is at least the requested version.
/// @param major Required major version component.
/// @param minor Required minor version component.
/// @param patch Required patch version component.
#define CASTLE_VERSION_AT_LEAST(major, minor, patch)     (CASTLE_VERSION >= CASTLE_VERSION_ENCODE(major, minor, patch))

namespace castle
{
    /// @brief Encoded Castle version exposed as a constexpr integer.
    static CASTLE_CONSTEXPR int version_encoded = CASTLE_VERSION;
    /// @brief Castle major version exposed as a constexpr integer.
    static CASTLE_CONSTEXPR int version_major = CASTLE_VERSION_MAJOR;
    /// @brief Castle minor version exposed as a constexpr integer.
    static CASTLE_CONSTEXPR int version_minor = CASTLE_VERSION_MINOR;
    /// @brief Castle patch version exposed as a constexpr integer.
    static CASTLE_CONSTEXPR int version_patch = CASTLE_VERSION_PATCH;
}

#endif // CASTLE_VERSION_HPP
