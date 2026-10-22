// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file io.hpp
 * @brief Umbrella header for CASTLE's POSIX file, UART, SPI, I2C and CAN extensions.
 *
 * Include the individual transport headers when minimizing compile-time
 * dependencies is important. Linux-specific UART/SPI/I2C support is guarded by
 * `CASTLE_USING_POSIX_APIS` and the relevant platform UAPI availability checks;
 * CAN selects SocketCAN, Zephyr or the host mock at compile time.
 */
#ifndef CASTLE_EXT_IO_IO_HPP
#define CASTLE_EXT_IO_IO_HPP

#include "castle_ext/io/fd/fd.hpp"
#include "castle_ext/io/file/linux_file.hpp"
#include "castle_ext/io/uart/uart.hpp"
#include "castle_ext/io/spi/spi.hpp"
#include "castle_ext/io/i2c/i2c.hpp"

#endif // CASTLE_EXT_IO_IO_HPP
