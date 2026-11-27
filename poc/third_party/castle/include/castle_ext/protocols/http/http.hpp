// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file http.hpp
 * @brief Umbrella header for CASTLE's allocation-free HTTP client and HTTP/1.1 server.
 *
 * Includes shared types, fixed-capacity headers, HPACK, HTTP/1.1 and HTTP/2 client protocol layers,
 * polled HTTP client adapters, and the bounded synchronous HTTP/1.1 server.
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
#include "castle_ext/protocols/http/server.hpp"

#endif // CASTLE_EXT_PROTOCOLS_HTTP_HTTP_HPP
