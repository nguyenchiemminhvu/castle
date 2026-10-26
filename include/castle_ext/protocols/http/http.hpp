// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file http.hpp
 * @brief Umbrella header of CASTLE's allocation-free HTTP/1.1 and HTTP/2 client.
 *
 * Includes the shared types, header collection, HPACK codec, both protocol layers and the polled clients.
 *
 * @code
 * #include "castle_ext/protocols/http/http.hpp"
 *
 * using namespace castle::protocols::http;
 *
 * castle::net::tcp_client<512U> tcp;
 * basic_http_client<castle::net::tcp_client<512U>, http2_policy> client(tcp);   // or http1_policy
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_HTTP_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_HTTP_HPP

#include "castle_ext/protocols/http/types.hpp"
#include "castle_ext/protocols/http/headers.hpp"
#include "castle_ext/protocols/http/hpack.hpp"
#include "castle_ext/protocols/http/http1.hpp"
#include "castle_ext/protocols/http/http2.hpp"
#include "castle_ext/protocols/http/client.hpp"

#endif // CASTLE_EXT_PROTOCOLS_HTTP_HTTP_HPP
