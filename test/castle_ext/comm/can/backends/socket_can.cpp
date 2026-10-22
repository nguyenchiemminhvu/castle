#include <gtest/gtest.h>

#include <atomic>

#include "castle/chrono/clocks.hpp"
#include "castle/chrono/duration.hpp"
#include "castle_ext/comm/can/backends/socket_can.hpp"
#include "castle_ext/comm/can/bus.hpp"

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    #include <errno.h>
    #include <fcntl.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>

    #include <thread>
#endif

namespace
{

using namespace castle::comm::can;

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

static_assert(sizeof(struct can_frame) == 16U, "the backend exchanges one 16-byte classic frame per datagram");

constexpr int sentinel_errno = 777;

long long elapsed_ms(castle::chrono::steady_clock::time_point start)
{
    return castle::chrono::duration_cast<castle::chrono::milliseconds>(castle::chrono::steady_clock::now() - start)
        .count();
}

struct can_frame make_raw(canid_t id, const uint8_t* bytes, size_t length)
{
    struct can_frame raw = can_frame();
    raw.can_id = id;
    raw.can_dlc = static_cast<uint8_t>(length);
    for (size_t i = 0U; bytes != nullptr && i < length && i < sizeof(raw.data); ++i)
    {
        raw.data[i] = bytes[i];
    }
    return raw;
}

struct can_frame make_error(canid_t classes, uint8_t controller_problem, uint8_t tx_counter = 0U, uint8_t rx_counter = 0U)
{
    struct can_frame raw = can_frame();
    raw.can_id = CAN_ERR_FLAG | classes;
    raw.can_dlc = CAN_ERR_DLC;
    raw.data[1] = controller_problem;
    raw.data[6] = tx_counter;
    raw.data[7] = rx_counter;
    return raw;
}

bool same_wire_frame(const struct can_frame& lhs, const struct can_frame& rhs)
{
    return lhs.can_id == rhs.can_id && lhs.can_dlc == rhs.can_dlc && ::memcmp(lhs.data, rhs.data, sizeof(lhs.data)) == 0;
}

bool can_raw_socket_available()
{
    const int probe = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (probe < 0)
    {
        return false;
    }
    (void)::close(probe);
    return true;
}

/** Backend over one end of a unix datagram socketpair; the other end plays the CAN controller. */
struct link
{
    int peer = -1;
    int local = -1;
    socket_can::backend backend;

    bool connect()
    {
        int sv[2] = {-1, -1};
        if (::socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) != 0)
        {
            return false;
        }
        peer = sv[1];
        local = sv[0];
        return backend.adopt(sv[0]) == castle::status::ok;
    }

    bool write_peer(const struct can_frame& raw) const
    {
        return ::write(peer, &raw, sizeof(raw)) == static_cast<ssize_t>(sizeof(raw));
    }

    bool read_peer(struct can_frame& raw) const
    {
        return ::recv(peer, &raw, sizeof(raw), MSG_DONTWAIT) == static_cast<ssize_t>(sizeof(raw));
    }

    void close_peer()
    {
        if (peer >= 0)
        {
            (void)::close(peer);
            peer = -1;
        }
    }

    ~link() { close_peer(); }
};

/** Runs `action` on another thread after `delay_ms`; joins on destruction. */
template <typename Action>
class delayed
{
public:
    delayed(int delay_ms, Action action)
        : thread_([delay_ms, action]() mutable
        {
            (void)::usleep(static_cast<useconds_t>(delay_ms) * 1000U);
            action();
        })
    {
    }

    ~delayed() { thread_.join(); }

private:
    std::thread thread_;
};

/** Fills the socket buffer of the backend's end until the kernel answers EAGAIN. */
size_t fill_send_buffer(int descriptor)
{
    const int small = 1024;
    EXPECT_EQ(::setsockopt(descriptor, SOL_SOCKET, SO_SNDBUF, &small, sizeof(small)), 0);
    const struct can_frame filler = make_raw(0x100U, nullptr, 0U);
    size_t written = 0U;
    while (written < 100000U)
    {
        if (::write(descriptor, &filler, sizeof(filler)) != static_cast<ssize_t>(sizeof(filler)))
        {
            break;
        }
        ++written;
    }
    return written;
}

/**
 * Plays a controller that keeps emptying the peer end after `delay_ms` until destroyed, recording
 * what it read. Draining repeatedly (instead of a fixed frame count) keeps the send-with-timeout
 * tests independent of how many frames a given kernel buffers before answering EAGAIN.
 */
class draining_peer
{
public:
    static const size_t capacity = 1024U;

    draining_peer(const link& wire, int delay_ms)
        : wire_(wire)
        , stop_(false)
        , count_(0U)
        , thread_([this, delay_ms]()
        {
            (void)::usleep(static_cast<useconds_t>(delay_ms) * 1000U);
            bool last_round = false;
            while (!last_round)
            {
                last_round = stop_.load(); // one more drain after the stop request catches the final frame
                struct can_frame raw;
                while (count_ < capacity && wire_.read_peer(raw))
                {
                    seen_[count_++] = raw;
                }
                (void)::usleep(1000U);
            }
        })
    {
    }

    /** Stops the thread; frames still queued on the peer afterwards were never read by it. */
    void finish()
    {
        if (thread_.joinable())
        {
            stop_.store(true);
            thread_.join();
        }
    }

    ~draining_peer() { finish(); }

    size_t count() const { return count_; }
    const struct can_frame& seen(size_t index) const { return seen_[index]; }

private:
    const link& wire_;
    std::atomic<bool> stop_;
    size_t count_;
    struct can_frame seen_[capacity];
    std::thread thread_;
};

/** Kills the test process instead of hanging forever if a blocking send never returns. */
class watchdog
{
public:
    explicit watchdog(unsigned int seconds) { (void)::alarm(seconds); }
    ~watchdog() { (void)::alarm(0U); }
};

// ---------------------------------------------------------------------------------------------
// native:: pure conversions
// ---------------------------------------------------------------------------------------------

TEST(CanSocketCan, ReportsAvailability)
{
    EXPECT_TRUE(socket_can::socket_can_available);
}

TEST(CanSocketCan, ToKernelStandardDataFrameZeroPadsPayload)
{
    frame value(0x123U);
    const uint8_t bytes[3] = {0x11U, 0x22U, 0x33U};
    ASSERT_EQ(value.set_data(bytes, 3U), castle::status::ok);

    struct can_frame raw;
    ::memset(&raw, 0xFF, sizeof(raw));
    socket_can::native::to_kernel(value, raw);

    EXPECT_EQ(raw.can_id, 0x123U);
    EXPECT_EQ(raw.can_dlc, 3U);
    EXPECT_EQ(raw.data[0], 0x11U);
    EXPECT_EQ(raw.data[1], 0x22U);
    EXPECT_EQ(raw.data[2], 0x33U);
    for (size_t i = 3U; i < sizeof(raw.data); ++i)
    {
        EXPECT_EQ(raw.data[i], 0U) << "padding byte " << i;
    }
    // The 8-byte tail of the kernel structure is wiped, not inherited from the previous contents.
    EXPECT_EQ(raw.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG), 0U);
}

TEST(CanSocketCan, ToKernelFullPayloadAndEmptyPayload)
{
    frame full(0x7FFU);
    const uint8_t bytes[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 0xFFU};
    ASSERT_EQ(full.set_data(bytes, 8U), castle::status::ok);

    struct can_frame raw;
    socket_can::native::to_kernel(full, raw);
    EXPECT_EQ(raw.can_id, 0x7FFU);
    EXPECT_EQ(raw.can_dlc, 8U);
    EXPECT_EQ(::memcmp(raw.data, bytes, 8U), 0);

    const frame empty(0U);
    socket_can::native::to_kernel(empty, raw);
    EXPECT_EQ(raw.can_id, 0U);
    EXPECT_EQ(raw.can_dlc, 0U);
    for (size_t i = 0U; i < sizeof(raw.data); ++i)
    {
        EXPECT_EQ(raw.data[i], 0U);
    }
}

TEST(CanSocketCan, ToKernelSetsExtendedFlag)
{
    frame value(0x1ABCDEF0U, id_format::extended);
    const uint8_t bytes[2] = {0xAAU, 0xBBU};
    ASSERT_EQ(value.set_data(bytes, 2U), castle::status::ok);

    struct can_frame raw;
    socket_can::native::to_kernel(value, raw);
    EXPECT_EQ(raw.can_id, 0x1ABCDEF0U | CAN_EFF_FLAG);
    EXPECT_EQ(raw.can_dlc, 2U);
    EXPECT_EQ(raw.data[0], 0xAAU);
    EXPECT_EQ(raw.data[1], 0xBBU);
    EXPECT_EQ(raw.can_id & CAN_RTR_FLAG, 0U);
}

TEST(CanSocketCan, ToKernelSetsRemoteFlagAndKeepsRequestedLength)
{
    frame standard(0x456U);
    ASSERT_EQ(standard.set_remote(5U), castle::status::ok);
    struct can_frame raw;
    socket_can::native::to_kernel(standard, raw);
    EXPECT_EQ(raw.can_id, 0x456U | CAN_RTR_FLAG);
    EXPECT_EQ(raw.can_dlc, 5U);
    for (size_t i = 0U; i < sizeof(raw.data); ++i)
    {
        EXPECT_EQ(raw.data[i], 0U);
    }

    frame extended(0x12345678U, id_format::extended);
    ASSERT_EQ(extended.set_remote(0U), castle::status::ok);
    socket_can::native::to_kernel(extended, raw);
    EXPECT_EQ(raw.can_id, 0x12345678U | CAN_EFF_FLAG | CAN_RTR_FLAG);
    EXPECT_EQ(raw.can_dlc, 0U);
}

TEST(CanSocketCan, ToKernelMasksIdentifierToItsFormat)
{
    struct can_frame raw;

    // A standard identifier is cut to 11 bits and never leaks into the flag bits.
    socket_can::native::to_kernel(frame(0xFFFFFFFFU), raw);
    EXPECT_EQ(raw.can_id, static_cast<canid_t>(CAN_SFF_MASK));
    socket_can::native::to_kernel(frame(0x800U), raw);
    EXPECT_EQ(raw.can_id, 0U);

    // An extended identifier is cut to 29 bits; bits 29..31 (ERR/RTR/EFF positions) are not copied.
    socket_can::native::to_kernel(frame(0xFFFFFFFFU, id_format::extended), raw);
    EXPECT_EQ(raw.can_id, static_cast<canid_t>(CAN_EFF_MASK | CAN_EFF_FLAG));
    socket_can::native::to_kernel(frame(CAN_ERR_FLAG | CAN_RTR_FLAG, id_format::extended), raw);
    EXPECT_EQ(raw.can_id, static_cast<canid_t>(CAN_EFF_FLAG));
    socket_can::native::to_kernel(frame(0x20000000U, id_format::extended), raw);
    EXPECT_EQ(raw.can_id & CAN_ERR_FLAG, 0U);
}

TEST(CanSocketCan, FromKernelRoundTripsStandardExtendedAndRemoteFrames)
{
    frame standard(0x7AAU);
    const uint8_t bytes[8] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    ASSERT_EQ(standard.set_data(bytes, 8U), castle::status::ok);

    frame extended(0x1555AAAAU, id_format::extended);
    ASSERT_EQ(extended.set_data(bytes, 4U), castle::status::ok);

    frame remote_standard(0x001U);
    ASSERT_EQ(remote_standard.set_remote(7U), castle::status::ok);

    frame remote_extended(0x1FFFFFFFU, id_format::extended);
    ASSERT_EQ(remote_extended.set_remote(8U), castle::status::ok);

    const frame originals[4] = {standard, extended, remote_standard, remote_extended};
    for (const frame& original : originals)
    {
        struct can_frame raw;
        socket_can::native::to_kernel(original, raw);
        frame decoded(0x55U, id_format::extended);
        EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::ok);
        EXPECT_EQ(decoded, original);
        EXPECT_EQ(decoded.id, original.id);
        EXPECT_EQ(decoded.format, original.format);
        EXPECT_EQ(decoded.type, original.type);
        EXPECT_EQ(decoded.dlc, original.dlc);
    }
}

TEST(CanSocketCan, FromKernelIgnoresBytesBeyondDlcAndPayloadOfRemoteFrames)
{
    const uint8_t bytes[8] = {9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U};
    struct can_frame raw = make_raw(0x321U, bytes, 8U);
    raw.can_dlc = 2U;
    frame decoded;
    ASSERT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::ok);
    EXPECT_EQ(decoded.dlc, 2U);
    EXPECT_EQ(decoded.data[0], 9U);
    EXPECT_EQ(decoded.data[1], 9U);
    for (size_t i = 2U; i < max_data_length; ++i)
    {
        EXPECT_EQ(decoded.data[i], 0U);
    }

    raw = make_raw(0x321U | CAN_RTR_FLAG, bytes, 8U);
    raw.can_dlc = 4U;
    ASSERT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::ok);
    EXPECT_TRUE(decoded.is_remote());
    EXPECT_EQ(decoded.dlc, 4U);
    EXPECT_EQ(decoded.data_length(), 0U);
    for (size_t i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(decoded.data[i], 0U);
    }
}

TEST(CanSocketCan, FromKernelReportsErrorFramesAndOversizedDlcAsDataLoss)
{
    const frame untouched(0x55U, id_format::extended);

    struct can_frame raw = make_error(CAN_ERR_BUSOFF, 0U);
    frame decoded = untouched;
    EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::data_loss);
    EXPECT_EQ(decoded, untouched);

    raw = make_raw(0x123U, nullptr, 0U);
    raw.can_dlc = 9U;
    EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::data_loss);
    EXPECT_EQ(decoded, untouched);
    raw.can_dlc = 15U;
    EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::data_loss);
    raw.can_dlc = 255U;
    EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::data_loss);
    EXPECT_EQ(decoded, untouched);

    raw.can_dlc = 8U;
    EXPECT_EQ(socket_can::native::from_kernel(raw, decoded), castle::status::ok);
    EXPECT_EQ(decoded.dlc, 8U);
}

TEST(CanSocketCan, ToKernelFilterAlwaysIncludesExtendedFlagInMask)
{
    struct can_filter converted = socket_can::native::to_kernel_filter(filter(0x120U, 0x7F0U));
    EXPECT_EQ(converted.can_id, 0x120U);
    EXPECT_EQ(converted.can_mask, 0x7F0U | CAN_EFF_FLAG);

    converted = socket_can::native::to_kernel_filter(filter(0x1234567U, 0x1FFFFFF0U, id_format::extended));
    EXPECT_EQ(converted.can_id, 0x1234567U | CAN_EFF_FLAG);
    EXPECT_EQ(converted.can_mask, 0x1FFFFFF0U | CAN_EFF_FLAG);

    // accept-all filters of each format differ only in the required EFF bit.
    converted = socket_can::native::to_kernel_filter(filter::any(id_format::standard));
    EXPECT_EQ(converted.can_id, 0U);
    EXPECT_EQ(converted.can_mask, static_cast<canid_t>(CAN_EFF_FLAG));
    converted = socket_can::native::to_kernel_filter(filter::any(id_format::extended));
    EXPECT_EQ(converted.can_id, static_cast<canid_t>(CAN_EFF_FLAG));
    EXPECT_EQ(converted.can_mask, static_cast<canid_t>(CAN_EFF_FLAG));

    converted = socket_can::native::to_kernel_filter(filter::exact(0x7FFU));
    EXPECT_EQ(converted.can_id, 0x7FFU);
    EXPECT_EQ(converted.can_mask, 0x7FFU | CAN_EFF_FLAG);
}

TEST(CanSocketCan, ToKernelFilterMasksIdentifierAndMaskPerFormat)
{
    struct can_filter converted = socket_can::native::to_kernel_filter(filter(0xFFFFF123U, 0xFFFFFFFFU));
    EXPECT_EQ(converted.can_id, 0x123U);
    EXPECT_EQ(converted.can_mask, static_cast<canid_t>(CAN_SFF_MASK | CAN_EFF_FLAG));

    converted = socket_can::native::to_kernel_filter(filter(0xFFFFFFFFU, 0xFFFFFFFFU, id_format::extended));
    EXPECT_EQ(converted.can_id, static_cast<canid_t>(CAN_EFF_MASK | CAN_EFF_FLAG));
    EXPECT_EQ(converted.can_mask, static_cast<canid_t>(CAN_EFF_MASK | CAN_EFF_FLAG));
    // No RTR or ERR bit sneaks in from the upper identifier bits.
    EXPECT_EQ(converted.can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG), 0U);
    EXPECT_EQ(converted.can_mask & (CAN_RTR_FLAG | CAN_ERR_FLAG), 0U);
}

TEST(CanSocketCan, ErrorMaskRequestsExactlyTheDecodedClasses)
{
    const can_err_mask_t expected = CAN_ERR_TX_TIMEOUT | CAN_ERR_CRTL | CAN_ERR_BUSOFF | CAN_ERR_RESTARTED;
    EXPECT_EQ(socket_can::native::error_mask, expected);
    EXPECT_NE(socket_can::native::error_mask & CAN_ERR_TX_TIMEOUT, 0U);
    EXPECT_NE(socket_can::native::error_mask & CAN_ERR_CRTL, 0U);
    EXPECT_NE(socket_can::native::error_mask & CAN_ERR_BUSOFF, 0U);
    EXPECT_NE(socket_can::native::error_mask & CAN_ERR_RESTARTED, 0U);
    EXPECT_EQ(socket_can::native::error_mask & (CAN_ERR_LOSTARB | CAN_ERR_PROT | CAN_ERR_TRX | CAN_ERR_ACK | CAN_ERR_BUSERROR),
              0U);
}

// ---------------------------------------------------------------------------------------------
// native::apply_error_frame
// ---------------------------------------------------------------------------------------------

bus_status active_status()
{
    bus_status status;
    status.state = bus_state::error_active;
    return status;
}

TEST(CanSocketCan, ApplyErrorFrameCountsTransmitTimeouts)
{
    bus_status status = active_status();
    status.state = bus_state::error_warning;
    socket_can::native::apply_error_frame(make_error(CAN_ERR_TX_TIMEOUT, 0U), status);
    EXPECT_EQ(status.tx_failures, 1U);
    EXPECT_EQ(status.state, bus_state::error_warning);
    socket_can::native::apply_error_frame(make_error(CAN_ERR_TX_TIMEOUT, 0U), status);
    EXPECT_EQ(status.tx_failures, 2U);
    EXPECT_EQ(status.rx_overruns, 0U);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 0U);
}

TEST(CanSocketCan, ApplyErrorFrameTracksBusOffAndRestart)
{
    bus_status status = active_status();
    socket_can::native::apply_error_frame(make_error(CAN_ERR_BUSOFF, 0U), status);
    EXPECT_EQ(status.state, bus_state::bus_off);

    socket_can::native::apply_error_frame(make_error(CAN_ERR_RESTARTED, 0U), status);
    EXPECT_EQ(status.state, bus_state::error_active);

    // Restart and bus-off reported together: the later bus-off wins.
    socket_can::native::apply_error_frame(make_error(CAN_ERR_RESTARTED | CAN_ERR_BUSOFF, 0U), status);
    EXPECT_EQ(status.state, bus_state::bus_off);
    EXPECT_EQ(status.tx_failures, 0U);
}

TEST(CanSocketCan, ApplyErrorFrameCountsControllerOverflows)
{
    bus_status status = active_status();
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_RX_OVERFLOW), status);
    EXPECT_EQ(status.rx_overruns, 1U);
    EXPECT_EQ(status.tx_failures, 0U);
    EXPECT_EQ(status.state, bus_state::error_active);

    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_TX_OVERFLOW), status);
    EXPECT_EQ(status.rx_overruns, 1U);
    EXPECT_EQ(status.tx_failures, 1U);
    EXPECT_EQ(status.state, bus_state::error_active);

    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_RX_OVERFLOW | CAN_ERR_CRTL_TX_OVERFLOW),
                                          status);
    EXPECT_EQ(status.rx_overruns, 2U);
    EXPECT_EQ(status.tx_failures, 2U);
}

TEST(CanSocketCan, ApplyErrorFrameMapsEachStateBit)
{
    struct state_case
    {
        uint8_t problem;
        bus_state expected;
    };
    const state_case cases[] = {
        {CAN_ERR_CRTL_RX_PASSIVE, bus_state::error_passive},
        {CAN_ERR_CRTL_TX_PASSIVE, bus_state::error_passive},
        {CAN_ERR_CRTL_RX_WARNING, bus_state::error_warning},
        {CAN_ERR_CRTL_TX_WARNING, bus_state::error_warning},
        {CAN_ERR_CRTL_ACTIVE, bus_state::error_active},
    };
    for (const state_case& item : cases)
    {
        bus_status status;
        status.state = bus_state::bus_off; // a distinct starting state proves the bit changed it
        socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, item.problem), status);
        EXPECT_EQ(status.state, item.expected) << "problem bits 0x" << std::hex << static_cast<int>(item.problem);
        EXPECT_EQ(status.rx_overruns, 0U);
        EXPECT_EQ(status.tx_failures, 0U);
    }
}

TEST(CanSocketCan, ApplyErrorFrameStatePrecedenceIsPassiveThenWarningThenActive)
{
    bus_status status = active_status();
    socket_can::native::apply_error_frame(
        make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_ACTIVE | CAN_ERR_CRTL_RX_WARNING | CAN_ERR_CRTL_TX_PASSIVE), status);
    EXPECT_EQ(status.state, bus_state::error_passive);

    status = active_status();
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_ACTIVE | CAN_ERR_CRTL_TX_WARNING),
                                          status);
    EXPECT_EQ(status.state, bus_state::error_warning);

    status = active_status();
    socket_can::native::apply_error_frame(
        make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_RX_PASSIVE | CAN_ERR_CRTL_RX_WARNING | CAN_ERR_CRTL_RX_OVERFLOW), status);
    EXPECT_EQ(status.state, bus_state::error_passive);
    EXPECT_EQ(status.rx_overruns, 1U);

    // "Unspecified" controller problem leaves the state alone.
    status.state = bus_state::error_warning;
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_UNSPEC), status);
    EXPECT_EQ(status.state, bus_state::error_warning);
}

TEST(CanSocketCan, ApplyErrorFrameReadsControllerErrorCounters)
{
    bus_status status = active_status();
#if defined(CAN_ERR_CNT)
    // The kernel flags valid counters with CAN_ERR_CNT; without it data[6]/data[7] are ignored.
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_TX_WARNING, 96U, 130U), status);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 0U);

    struct can_frame with_counters = make_error(CAN_ERR_CRTL | CAN_ERR_CNT, CAN_ERR_CRTL_TX_WARNING, 96U, 130U);
    socket_can::native::apply_error_frame(with_counters, status);
    EXPECT_EQ(status.tx_error_counter, 96U);
    EXPECT_EQ(status.rx_error_counter, 130U);
#else
    // This kernel header predates CAN_ERR_CNT: every controller error frame carries counters in data[6]/data[7].
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_TX_WARNING, 96U, 130U), status);
    EXPECT_EQ(status.tx_error_counter, 96U);
    EXPECT_EQ(status.rx_error_counter, 130U);
    socket_can::native::apply_error_frame(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_ACTIVE, 0U, 255U), status);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 255U);
    EXPECT_EQ(status.state, bus_state::error_active);
#endif
}

TEST(CanSocketCan, ApplyErrorFrameIgnoresClassesItDoesNotDecode)
{
    bus_status status = active_status();
    status.tx_error_counter = 3U;
    status.rx_error_counter = 4U;
    // Controller data is only meaningful together with CAN_ERR_CRTL.
    struct can_frame raw = make_error(CAN_ERR_BUSERROR | CAN_ERR_ACK | CAN_ERR_LOSTARB,
                                      CAN_ERR_CRTL_RX_OVERFLOW | CAN_ERR_CRTL_TX_PASSIVE, 200U, 201U);
    socket_can::native::apply_error_frame(raw, status);
    EXPECT_EQ(status.state, bus_state::error_active);
    EXPECT_EQ(status.rx_overruns, 0U);
    EXPECT_EQ(status.tx_failures, 0U);
    EXPECT_EQ(status.tx_error_counter, 3U);
    EXPECT_EQ(status.rx_error_counter, 4U);
}

// ---------------------------------------------------------------------------------------------
// Backend over an adopted unix datagram socketpair
// ---------------------------------------------------------------------------------------------

TEST(CanSocketCan, ClosedBackendReportsNotConfiguredEverywhere)
{
    socket_can::backend backend;
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);

    int system_error = sentinel_errno;
    EXPECT_EQ(backend.send(frame(0x1U), no_wait, &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);

    frame out(0x7U);
    system_error = sentinel_errno;
    EXPECT_EQ(backend.receive(out, no_wait, &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x7U);

    filter_handle handle = 5;
    system_error = sentinel_errno;
    EXPECT_EQ(backend.add_filter(filter::any(), handle, &system_error), castle::status::not_configured);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(system_error, 0);

    system_error = sentinel_errno;
    EXPECT_EQ(backend.remove_filter(0, &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    system_error = sentinel_errno;
    EXPECT_EQ(backend.clear_filters(&system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);

    bus_status status;
    status.state = bus_state::error_passive;
    status.rx_overruns = 9U;
    system_error = sentinel_errno;
    EXPECT_EQ(backend.query_status(status, &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(status.state, bus_state::stopped);
    EXPECT_EQ(status.rx_overruns, 0U);

    system_error = sentinel_errno;
    EXPECT_EQ(backend.close(&system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    // The system_error pointer is optional on every call.
    EXPECT_EQ(backend.send(frame(0x1U), no_wait, nullptr), castle::status::not_configured);
    EXPECT_EQ(backend.close(nullptr), castle::status::not_configured);
}

TEST(CanSocketCan, AdoptTakesOwnershipAndConfiguresDescriptor)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    EXPECT_TRUE(socket.backend.is_open());
    EXPECT_EQ(socket.backend.native_handle(), socket.local);

    const int fd_flags = ::fcntl(socket.local, F_GETFD);
    ASSERT_GE(fd_flags, 0);
    EXPECT_NE(fd_flags & FD_CLOEXEC, 0);
    const int status_flags = ::fcntl(socket.local, F_GETFL);
    ASSERT_GE(status_flags, 0);
    EXPECT_NE(status_flags & O_NONBLOCK, 0);

    bus_status status;
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.query_status(status, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(status.state, bus_state::error_active);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 0U);
    EXPECT_EQ(status.rx_overruns, 0U);
    EXPECT_EQ(status.tx_failures, 0U);
}

TEST(CanSocketCan, AdoptRejectsNegativeDescriptor)
{
    socket_can::backend backend;
    int system_error = sentinel_errno;
    EXPECT_EQ(backend.adopt(-1, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.adopt(-42), castle::status::invalid_argument);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);
}

TEST(CanSocketCan, AdoptWhileOpenFailsAndClosesTheDescriptorItWasGiven)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const int original = socket.backend.native_handle();

    int other[2] = {-1, -1};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, other), 0);
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.adopt(other[0], &system_error), castle::status::invalid_config);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(socket.backend.native_handle(), original);

    // Ownership transferred even though adopt failed: the descriptor is gone.
    errno = 0;
    EXPECT_EQ(::fcntl(other[0], F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_NE(::fcntl(other[1], F_GETFD), -1);
    (void)::close(other[1]);
    EXPECT_NE(::fcntl(original, F_GETFD), -1);
}

TEST(CanSocketCan, AdoptReportsFcntlFailureForUnusableDescriptorNumber)
{
    int pair[2] = {-1, -1};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, pair), 0);
    ASSERT_EQ(::close(pair[0]), 0);
    ASSERT_EQ(::close(pair[1]), 0);

    socket_can::backend backend;
    int system_error = 0;
    EXPECT_EQ(backend.adopt(pair[0], &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);
}

TEST(CanSocketCan, CloseReleasesDescriptorAndRejectsSecondClose)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const int handle = socket.backend.native_handle();

    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.close(&system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_FALSE(socket.backend.is_open());
    EXPECT_EQ(socket.backend.native_handle(), -1);
    errno = 0;
    EXPECT_EQ(::fcntl(handle, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.close(&system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);

    bus_status status;
    status.state = bus_state::error_active;
    EXPECT_EQ(socket.backend.query_status(status, nullptr), castle::status::not_configured);
    EXPECT_EQ(status.state, bus_state::stopped);
    EXPECT_EQ(socket.backend.send(frame(0x1U), no_wait, nullptr), castle::status::not_configured);
    frame out;
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::not_configured);
}

TEST(CanSocketCan, ReopenAfterCloseStartsWithFreshStatus)
{
    link first;
    ASSERT_TRUE(first.connect());
    ASSERT_TRUE(first.write_peer(make_error(CAN_ERR_TX_TIMEOUT | CAN_ERR_BUSOFF, 0U)));
    frame out;
    EXPECT_EQ(first.backend.receive(out, no_wait, nullptr), castle::status::empty);
    bus_status status;
    ASSERT_EQ(first.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::bus_off);
    EXPECT_EQ(status.tx_failures, 1U);

    ASSERT_EQ(first.backend.close(nullptr), castle::status::ok);

    // The same backend object takes a new descriptor and forgets everything learned before.
    int pair[2] = {-1, -1};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, pair), 0);
    ASSERT_EQ(first.backend.adopt(pair[0]), castle::status::ok);
    EXPECT_TRUE(first.backend.is_open());
    EXPECT_EQ(first.backend.native_handle(), pair[0]);
    ASSERT_EQ(first.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_active);
    EXPECT_EQ(status.tx_failures, 0U);

    const struct can_frame wire = make_raw(0x42U, nullptr, 0U);
    ASSERT_EQ(::write(pair[1], &wire, sizeof(wire)), static_cast<ssize_t>(sizeof(wire)));
    EXPECT_EQ(first.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x42U);
    (void)::close(pair[1]);
}

TEST(CanSocketCan, SendWritesExactlyOneKernelFrame)
{
    link socket;
    ASSERT_TRUE(socket.connect());

    frame value(0x1ABCDEU, id_format::extended);
    const uint8_t bytes[5] = {0xDEU, 0xADU, 0xBEU, 0xEFU, 0x01U};
    ASSERT_EQ(value.set_data(bytes, 5U), castle::status::ok);

    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.send(value, no_wait, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);

    struct can_frame wire;
    ASSERT_TRUE(socket.read_peer(wire));
    struct can_frame expected = make_raw(0x1ABCDEU | CAN_EFF_FLAG, bytes, 5U);
    EXPECT_EQ(wire.can_id, expected.can_id);
    EXPECT_EQ(wire.can_dlc, 5U);
    EXPECT_TRUE(same_wire_frame(wire, expected));
    // Exactly one datagram of exactly 16 bytes was produced.
    struct can_frame second;
    errno = 0;
    EXPECT_EQ(::recv(socket.peer, &second, sizeof(second), MSG_DONTWAIT), -1);
    EXPECT_EQ(errno, EAGAIN);
}

TEST(CanSocketCan, SendWritesStandardAndRemoteFrames)
{
    link socket;
    ASSERT_TRUE(socket.connect());

    frame standard(0x123U);
    const uint8_t bytes[2] = {0x01U, 0x02U};
    ASSERT_EQ(standard.set_data(bytes, 2U), castle::status::ok);
    frame remote(0x7FFU);
    ASSERT_EQ(remote.set_remote(3U), castle::status::ok);
    frame remote_extended(0x1FFFFFFFU, id_format::extended);
    ASSERT_EQ(remote_extended.set_remote(8U), castle::status::ok);

    EXPECT_EQ(socket.backend.send(standard, castle::chrono::milliseconds(20), nullptr), castle::status::ok);
    EXPECT_EQ(socket.backend.send(remote, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(socket.backend.send(remote_extended, no_wait, nullptr), castle::status::ok);

    struct can_frame wire;
    ASSERT_TRUE(socket.read_peer(wire));
    EXPECT_TRUE(same_wire_frame(wire, make_raw(0x123U, bytes, 2U)));
    ASSERT_TRUE(socket.read_peer(wire));
    EXPECT_EQ(wire.can_id, 0x7FFU | CAN_RTR_FLAG);
    EXPECT_EQ(wire.can_dlc, 3U);
    ASSERT_TRUE(socket.read_peer(wire));
    EXPECT_EQ(wire.can_id, static_cast<canid_t>(0x1FFFFFFFU | CAN_EFF_FLAG | CAN_RTR_FLAG));
    EXPECT_EQ(wire.can_dlc, 8U);
    EXPECT_FALSE(socket.read_peer(wire));
}

TEST(CanSocketCan, ReceiveDecodesStandardExtendedAndRemoteFrames)
{
    link socket;
    ASSERT_TRUE(socket.connect());

    const uint8_t bytes[4] = {0x10U, 0x20U, 0x30U, 0x40U};
    ASSERT_TRUE(socket.write_peer(make_raw(0x321U, bytes, 4U)));
    ASSERT_TRUE(socket.write_peer(make_raw(0x15555555U | CAN_EFF_FLAG, bytes, 2U)));
    ASSERT_TRUE(socket.write_peer(make_raw(0x00AU | CAN_RTR_FLAG, nullptr, 6U)));
    ASSERT_TRUE(socket.write_peer(make_raw(0x1FFFFFFFU | CAN_EFF_FLAG | CAN_RTR_FLAG, nullptr, 0U)));

    int system_error = sentinel_errno;
    frame out;
    ASSERT_EQ(socket.backend.receive(out, no_wait, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x321U);
    EXPECT_FALSE(out.is_extended());
    EXPECT_FALSE(out.is_remote());
    EXPECT_EQ(out.dlc, 4U);
    EXPECT_EQ(::memcmp(out.data.data(), bytes, 4U), 0);

    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x15555555U);
    EXPECT_TRUE(out.is_extended());
    EXPECT_FALSE(out.is_remote());
    EXPECT_EQ(out.dlc, 2U);

    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x00AU);
    EXPECT_FALSE(out.is_extended());
    EXPECT_TRUE(out.is_remote());
    EXPECT_EQ(out.dlc, 6U);

    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x1FFFFFFFU);
    EXPECT_TRUE(out.is_extended());
    EXPECT_TRUE(out.is_remote());
    EXPECT_EQ(out.dlc, 0U);

    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::empty);
}

TEST(CanSocketCan, ReceiveReturnsEmptyWithoutBlockingForNoWait)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    frame out(0x99U);
    int system_error = sentinel_errno;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.receive(out, no_wait, &system_error), castle::status::empty);
    EXPECT_LT(elapsed_ms(start), 500);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x99U);
}

TEST(CanSocketCan, ReceiveWaitsForTheWholeTimeoutBeforeReportingEmpty)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    frame out;
    int system_error = sentinel_errno;

    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(40), &system_error), castle::status::empty);
    const long long waited = elapsed_ms(start);
    EXPECT_GE(waited, 38);
    EXPECT_LT(waited, 2000);
    EXPECT_EQ(system_error, 0);
}

TEST(CanSocketCan, ReceiveReturnsAFrameQueuedBeforeTheCallWithoutWaiting)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const uint8_t bytes[1] = {0x5AU};
    ASSERT_TRUE(socket.write_peer(make_raw(0x77U, bytes, 1U)));

    frame out;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    ASSERT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(1500), nullptr), castle::status::ok);
    EXPECT_LT(elapsed_ms(start), 1000);
    EXPECT_EQ(out.id, 0x77U);
    EXPECT_EQ(out.data[0], 0x5AU);
}

TEST(CanSocketCan, ReceiveWaitsForAFrameThatArrivesLater)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const uint8_t bytes[2] = {0xC0U, 0xDEU};
    frame out;
    {
        delayed late(30, [&socket, &bytes]() { (void)socket.write_peer(make_raw(0x2A5U, bytes, 2U)); });
        const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
        ASSERT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(5000), nullptr), castle::status::ok);
        const long long waited = elapsed_ms(start);
        EXPECT_GE(waited, 20);
        EXPECT_LT(waited, 4000);
    }
    EXPECT_EQ(out.id, 0x2A5U);
    EXPECT_EQ(out.dlc, 2U);
    EXPECT_EQ(out.data[0], 0xC0U);
    EXPECT_EQ(out.data[1], 0xDEU);
}

TEST(CanSocketCan, ReceiveWithForeverTimeoutBlocksUntilFrameArrives)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    frame out;
    {
        delayed late(20, [&socket]() { (void)socket.write_peer(make_raw(0x1U | CAN_EFF_FLAG, nullptr, 0U)); });
        ASSERT_EQ(socket.backend.receive(out, wait_forever, nullptr), castle::status::ok);
    }
    EXPECT_EQ(out.id, 0x1U);
    EXPECT_TRUE(out.is_extended());
}

TEST(CanSocketCan, ReceiveClampsTimeoutsBeyondPollRange)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    frame out;
    {
        // 4.0e9 ms does not fit the int milliseconds argument of poll(); the wait is clamped, not wrapped.
        delayed late(20, [&socket]() { (void)socket.write_peer(make_raw(0x3U, nullptr, 0U)); });
        const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
        ASSERT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(4000000000LL), nullptr),
                  castle::status::ok);
        EXPECT_LT(elapsed_ms(start), 4000);
    }
    EXPECT_EQ(out.id, 0x3U);
}

TEST(CanSocketCan, ReceiveConsumesErrorFramesAndUpdatesStatus)
{
    link socket;
    ASSERT_TRUE(socket.connect());

    const canid_t counters_flag =
#if defined(CAN_ERR_CNT)
        CAN_ERR_CNT;
#else
        0U;
#endif
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_TX_TIMEOUT, 0U)));
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_CRTL | counters_flag,
                                             CAN_ERR_CRTL_RX_OVERFLOW | CAN_ERR_CRTL_TX_OVERFLOW | CAN_ERR_CRTL_TX_PASSIVE,
                                             128U, 96U)));
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_TX_TIMEOUT, 0U)));

    frame out(0x5U);
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.receive(out, no_wait, &system_error), castle::status::empty);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x5U);

    bus_status status;
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_passive);
    EXPECT_EQ(status.tx_failures, 3U); // two TX timeouts and one controller TX overflow
    EXPECT_EQ(status.rx_overruns, 1U);
    EXPECT_EQ(status.tx_error_counter, 128U);
    EXPECT_EQ(status.rx_error_counter, 96U);

    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_BUSOFF, 0U)));
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::empty);
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::bus_off);

    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_RESTARTED, 0U)));
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::empty);
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_active);
}

TEST(CanSocketCan, ReceiveSkipsErrorFramesAndReturnsTheNextDataFrame)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const uint8_t bytes[1] = {0x33U};
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_RX_WARNING)));
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_RX_OVERFLOW)));
    ASSERT_TRUE(socket.write_peer(make_raw(0x222U, bytes, 1U)));
    ASSERT_TRUE(socket.write_peer(make_raw(0x223U, bytes, 1U)));

    frame out;
    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x222U);
    bus_status status;
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_warning);
    EXPECT_EQ(status.rx_overruns, 1U);

    // The second data frame is still queued: only the frames before the first data frame were consumed.
    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x223U);
}

TEST(CanSocketCan, ReceiveKeepsWaitingForTheRemainingTimeAfterAnErrorFrame)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    ASSERT_TRUE(socket.write_peer(make_error(CAN_ERR_BUSOFF, 0U)));

    frame out;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(40), nullptr), castle::status::empty);
    EXPECT_GE(elapsed_ms(start), 38);
    EXPECT_LT(elapsed_ms(start), 2000);

    bus_status status;
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::bus_off);
}

TEST(CanSocketCan, ReceiveWakesForADataFrameBehindAnErrorFrameThatArrivesLater)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    frame out;
    {
        delayed late(25, [&socket]()
        {
            (void)socket.write_peer(make_error(CAN_ERR_CRTL, CAN_ERR_CRTL_TX_WARNING));
            (void)::usleep(10000U);
            (void)socket.write_peer(make_raw(0x6B6U, nullptr, 0U));
        });
        ASSERT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(5000), nullptr), castle::status::ok);
    }
    EXPECT_EQ(out.id, 0x6B6U);
    bus_status status;
    ASSERT_EQ(socket.backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_warning);
}

TEST(CanSocketCan, ReceiveReportsShortDatagramAsDataLoss)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const uint8_t half[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    ASSERT_EQ(::write(socket.peer, half, sizeof(half)), static_cast<ssize_t>(sizeof(half)));

    frame out(0x31U);
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.receive(out, no_wait, &system_error), castle::status::data_loss);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x31U);

    // The bad datagram is consumed, so the next frame is received normally.
    ASSERT_TRUE(socket.write_peer(make_raw(0x32U, nullptr, 0U)));
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x32U);
}

TEST(CanSocketCan, ReceiveReportsOversizedDlcAsDataLoss)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    struct can_frame raw = make_raw(0x123U, nullptr, 0U);
    raw.can_dlc = 9U;
    ASSERT_TRUE(socket.write_peer(raw));
    raw.can_dlc = 8U;
    ASSERT_TRUE(socket.write_peer(raw));

    frame out;
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::data_loss);
    EXPECT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.dlc, 8U);
}

TEST(CanSocketCan, SendReportsFullWhenSocketBufferIsFull)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const size_t queued = fill_send_buffer(socket.local);
    ASSERT_GT(queued, 0U);
    ASSERT_LT(queued, 100000U);

    int system_error = sentinel_errno;
    const castle::chrono::steady_clock::time_point no_wait_start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.send(frame(0x321U), no_wait, &system_error), castle::status::full);
    EXPECT_LT(elapsed_ms(no_wait_start), 500);
    EXPECT_EQ(system_error, EAGAIN);

    // With a timeout the call waits for writability, finds none, and still ends as `full`.
    system_error = sentinel_errno;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.send(frame(0x321U), castle::chrono::milliseconds(40), &system_error), castle::status::full);
    EXPECT_GE(elapsed_ms(start), 38);
    EXPECT_LT(elapsed_ms(start), 2000);

    // Once the peer drained the queue, sending works again; the rejected frames were never queued.
    struct can_frame wire;
    size_t drained = 0U;
    while (socket.read_peer(wire))
    {
        EXPECT_EQ(wire.can_id, 0x100U);
        ++drained;
    }
    EXPECT_EQ(drained, queued);
    system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.send(frame(0x321U), no_wait, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    ASSERT_TRUE(socket.read_peer(wire));
    EXPECT_EQ(wire.can_id, 0x321U);
}

TEST(CanSocketCan, SendWithTimeoutSucceedsOnceThePeerFreesSpace)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const size_t queued = fill_send_buffer(socket.local);
    ASSERT_GT(queued, 0U);
    ASSERT_LT(queued, draining_peer::capacity - 1U);

    const watchdog guard(30U);
    int system_error = sentinel_errno;
    castle::status result = castle::status::unknown_error;
    int elapsed = 0;
    {
        draining_peer controller(socket, 30);
        // The timeout is larger than INT_MAX milliseconds, which also covers the clamp to poll()'s range.
        const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
        result = socket.backend.send(frame(0x321U), castle::chrono::milliseconds(3000000000LL), &system_error);
        elapsed = elapsed_ms(start);
        controller.finish();

        // FIFO: every filler frame precedes the late frame, which arrives exactly once and last.
        ASSERT_EQ(controller.count(), queued + 1U);
        for (size_t i = 0U; i < queued; ++i)
        {
            EXPECT_EQ(controller.seen(i).can_id, 0x100U);
        }
        EXPECT_EQ(controller.seen(queued).can_id, 0x321U);
    }
    EXPECT_EQ(result, castle::status::ok);
    EXPECT_GE(elapsed, 25);
    EXPECT_LT(elapsed, 5000);
    EXPECT_EQ(system_error, 0);
}

TEST(CanSocketCan, SendWithForeverTimeoutWaitsUntilThePeerFreesSpace)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const size_t queued = fill_send_buffer(socket.local);
    ASSERT_GT(queued, 0U);
    ASSERT_LT(queued, draining_peer::capacity - 1U);

    const watchdog guard(30U);
    castle::status result = castle::status::unknown_error;
    {
        draining_peer controller(socket, 30);
        result = socket.backend.send(frame(0x555U), wait_forever, nullptr);
        controller.finish();

        ASSERT_EQ(controller.count(), queued + 1U);
        EXPECT_EQ(controller.seen(queued).can_id, 0x555U);
    }
    EXPECT_EQ(result, castle::status::ok);
}

TEST(CanSocketCan, ReceiveAndSendReportReadAndWriteErrorsOfTheDescriptor)
{
    // A pipe is not a CAN socket, but it makes read() and write() fail in the way the backend must report:
    // reading the write end and writing the read end both yield EBADF.
    int pipe_ends[2] = {-1, -1};
    ASSERT_EQ(::pipe(pipe_ends), 0);
    socket_can::backend reader;
    ASSERT_EQ(reader.adopt(pipe_ends[1]), castle::status::ok);
    frame out(0x9U);
    int system_error = 0;
    EXPECT_EQ(reader.receive(out, no_wait, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(out.id, 0x9U);
    system_error = 0;
    EXPECT_EQ(reader.receive(out, castle::chrono::milliseconds(100), &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);

    socket_can::backend writer;
    ASSERT_EQ(writer.adopt(pipe_ends[0]), castle::status::ok);
    system_error = 0;
    EXPECT_EQ(writer.send(frame(0x9U), no_wait, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
}

TEST(CanSocketCan, SendToClosedPeerFailsWithSystemCallError)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    socket.close_peer();

    int system_error = 0;
    EXPECT_EQ(socket.backend.send(frame(0x10U), no_wait, &system_error), castle::status::system_call_error);
    // The kernel reports the vanished peer of a connected datagram socket as a connection error.
    EXPECT_TRUE(system_error == ECONNREFUSED || system_error == ENOTCONN) << "errno " << system_error;
    system_error = 0;
    EXPECT_EQ(socket.backend.send(frame(0x10U), castle::chrono::milliseconds(20), &system_error),
              castle::status::system_call_error);
    EXPECT_TRUE(system_error == ECONNREFUSED || system_error == ENOTCONN) << "errno " << system_error;
    // A hard failure is not retried after a wait: the call returns at once.
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.send(frame(0x10U), castle::chrono::milliseconds(2000), nullptr),
              castle::status::system_call_error);
    EXPECT_LT(elapsed_ms(start), 1000);
}

TEST(CanSocketCan, ReceiveFromClosedPeerStaysEmptyBecauseDatagramSocketsHaveNoEndOfFile)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    ASSERT_TRUE(socket.write_peer(make_raw(0x44U, nullptr, 0U)));
    socket.close_peer();

    // A frame queued before the peer vanished is still delivered.
    frame out;
    ASSERT_EQ(socket.backend.receive(out, no_wait, nullptr), castle::status::ok);
    EXPECT_EQ(out.id, 0x44U);

    // An unconnected-looking datagram socket is simply silent; there is no hang-up or error to report.
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.receive(out, no_wait, &system_error), castle::status::empty);
    EXPECT_EQ(system_error, 0);
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(socket.backend.receive(out, castle::chrono::milliseconds(30), &system_error), castle::status::empty);
    EXPECT_GE(elapsed_ms(start), 28);
    EXPECT_EQ(system_error, 0);
}

TEST(CanSocketCan, FilterOperationsFailOnSocketsWithoutCanRawOptions)
{
    link socket;
    ASSERT_TRUE(socket.connect());

    // A unix socket rejects SOL_CAN_RAW options; the kernel error surfaces and no filter is kept,
    // so the table never fills up even after far more than MaxFilters (8) attempts.
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        filter_handle handle = 3;
        int system_error = 0;
        EXPECT_EQ(socket.backend.add_filter(filter::exact(0x100U + static_cast<uint32_t>(attempt)), handle, &system_error),
                  castle::status::system_call_error)
            << "attempt " << attempt;
        EXPECT_EQ(handle, invalid_filter);
        EXPECT_TRUE(system_error == ENOPROTOOPT || system_error == EOPNOTSUPP) << "errno " << system_error;
    }

    // Nothing was kept, so no handle is known.
    int system_error = 0;
    EXPECT_EQ(socket.backend.remove_filter(0, &system_error), castle::status::not_found);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(socket.backend.remove_filter(7, nullptr), castle::status::not_found);
    EXPECT_EQ(socket.backend.remove_filter(-1, nullptr), castle::status::not_found);
    EXPECT_EQ(socket.backend.remove_filter(100, nullptr), castle::status::not_found);

    // clear_filters always pushes the (now empty) list to the kernel, which fails here.
    system_error = 0;
    EXPECT_EQ(socket.backend.clear_filters(&system_error), castle::status::system_call_error);
    EXPECT_TRUE(system_error == ENOPROTOOPT || system_error == EOPNOTSUPP) << "errno " << system_error;

    // The backend stays usable.
    EXPECT_TRUE(socket.backend.is_open());
    EXPECT_EQ(socket.backend.send(frame(0x1U), no_wait, nullptr), castle::status::ok);
}

TEST(CanSocketCan, FilterOperationsOnTinyBackendNeverReportFullWhenKernelRejects)
{
    int pair[2] = {-1, -1};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, pair), 0);
    socket_can::basic_backend<1U> backend;
    ASSERT_EQ(backend.adopt(pair[0]), castle::status::ok);
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        filter_handle handle = 0;
        EXPECT_EQ(backend.add_filter(filter::any(), handle, nullptr), castle::status::system_call_error);
        EXPECT_EQ(handle, invalid_filter);
    }
    (void)::close(pair[1]);
}

// ---------------------------------------------------------------------------------------------
// open() validation: independent of kernel CAN support
// ---------------------------------------------------------------------------------------------

TEST(CanSocketCan, OpenRejectsNullEmptyAndOverlongNames)
{
    socket_can::backend backend;
    int system_error = sentinel_errno;
    EXPECT_EQ(backend.open(nullptr, can_config(), &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);
    system_error = sentinel_errno;
    EXPECT_EQ(backend.open("", can_config(), &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);

    // IFNAMSIZ (16) includes the terminating NUL, so 16 visible characters never fit.
    system_error = sentinel_errno;
    EXPECT_EQ(backend.open("abcdefghijklmnop", can_config(), &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.open("a_very_long_interface_name_that_is_rejected", can_config(), nullptr),
              castle::status::invalid_argument);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);
}

TEST(CanSocketCan, OpenRejectsListenOnlyMode)
{
    socket_can::backend backend;
    int system_error = sentinel_errno;
    EXPECT_EQ(backend.open("any", can_config(0U, can_mode::listen_only), &system_error), castle::status::invalid_config);
    EXPECT_EQ(system_error, 0);
    EXPECT_FALSE(backend.is_open());
    // The name is validated before the mode.
    EXPECT_EQ(backend.open("", can_config(0U, can_mode::listen_only), nullptr), castle::status::invalid_argument);
}

TEST(CanSocketCan, OpenRejectsSecondOpenOfAnAdoptedBackend)
{
    link socket;
    ASSERT_TRUE(socket.connect());
    const int handle = socket.backend.native_handle();
    int system_error = sentinel_errno;
    EXPECT_EQ(socket.backend.open("any", can_config(), &system_error), castle::status::invalid_config);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(socket.backend.native_handle(), handle);
}

// ---------------------------------------------------------------------------------------------
// Real CAN_RAW sockets (no interface needed: "any" binds to ifindex 0)
// ---------------------------------------------------------------------------------------------

#define SKIP_WITHOUT_CAN_RAW()                                       \
    do                                                               \
    {                                                                \
        if (!can_raw_socket_available())                             \
        {                                                            \
            GTEST_SKIP() << "PF_CAN/CAN_RAW sockets are unavailable"; \
        }                                                            \
    } while (false)

int get_int_option(int descriptor, int option)
{
    int value = -1;
    socklen_t length = sizeof(value);
    if (::getsockopt(descriptor, SOL_CAN_RAW, option, &value, &length) != 0)
    {
        return -1;
    }
    return value;
}

/** Reads CAN_RAW_FILTER back; returns the number of installed kernel filters. */
size_t get_kernel_filters(int descriptor, struct can_filter* out, size_t capacity)
{
    socklen_t length = static_cast<socklen_t>(capacity * sizeof(struct can_filter));
    if (::getsockopt(descriptor, SOL_CAN_RAW, CAN_RAW_FILTER, out, &length) != 0)
    {
        return static_cast<size_t>(-1);
    }
    return length / sizeof(struct can_filter);
}

TEST(CanSocketCan, OpenAnyCreatesNonBlockingCloseOnExecSocketWithNoFilters)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    int system_error = sentinel_errno;
    ASSERT_EQ(backend.open("any", can_config(), &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_TRUE(backend.is_open());
    const int handle = backend.native_handle();
    ASSERT_GE(handle, 0);

    EXPECT_NE(::fcntl(handle, F_GETFD) & FD_CLOEXEC, 0);
    EXPECT_NE(::fcntl(handle, F_GETFL) & O_NONBLOCK, 0);
    EXPECT_EQ(get_int_option(handle, CAN_RAW_RECV_OWN_MSGS), 0);

    can_err_mask_t errors = 0U;
    socklen_t length = sizeof(errors);
    ASSERT_EQ(::getsockopt(handle, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &errors, &length), 0);
    EXPECT_EQ(errors, socket_can::native::error_mask);

    // An opened socket starts with an empty filter list (receives nothing).
    struct can_filter installed[4];
    EXPECT_EQ(get_kernel_filters(handle, installed, 4U), 0U);

    bus_status status;
    system_error = sentinel_errno;
    ASSERT_EQ(backend.query_status(status, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(status.state, bus_state::error_active);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 0U);
    EXPECT_EQ(status.rx_overruns, 0U);
    EXPECT_EQ(status.tx_failures, 0U);
}

TEST(CanSocketCan, OpenLoopbackEnablesOwnMessages)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    ASSERT_EQ(backend.open("any", can_config(0U, can_mode::loopback), nullptr), castle::status::ok);
    EXPECT_EQ(get_int_option(backend.native_handle(), CAN_RAW_RECV_OWN_MSGS), 1);

    bus_status status;
    ASSERT_EQ(backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_active);
    ASSERT_EQ(backend.close(nullptr), castle::status::ok);

    // Re-opening in normal mode turns it off again.
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    EXPECT_EQ(get_int_option(backend.native_handle(), CAN_RAW_RECV_OWN_MSGS), 0);
}

TEST(CanSocketCan, OpenTwiceFailsAndKeepsTheFirstSocket)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    const int handle = backend.native_handle();

    int system_error = sentinel_errno;
    EXPECT_EQ(backend.open("any", can_config(), &system_error), castle::status::invalid_config);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.native_handle(), handle);
    // adopt() on an open backend is rejected the same way (and consumes the given descriptor).
    int pair[2] = {-1, -1};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, pair), 0);
    EXPECT_EQ(backend.adopt(pair[0], nullptr), castle::status::invalid_config);
    (void)::close(pair[1]);
    EXPECT_EQ(backend.native_handle(), handle);
}

TEST(CanSocketCan, OpenUnknownInterfaceFailsWithNoDeviceAndDoesNotLeakTheSocket)
{
    SKIP_WITHOUT_CAN_RAW();
    const int before = ::open("/dev/null", O_RDONLY);
    ASSERT_GE(before, 0);
    ASSERT_EQ(::close(before), 0);

    socket_can::backend backend;
    int system_error = 0;
    // 19 characters do not fit IFNAMSIZ: rejected before any socket is created.
    EXPECT_EQ(backend.open("definitely_missing0", can_config(), &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.open("missing_can0", can_config(), &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ENODEV);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);

    // The longest accepted name (15 characters) passes validation and fails only in the kernel.
    system_error = 0;
    EXPECT_EQ(backend.open("abcdefghijklmno", can_config(), &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ENODEV);
    EXPECT_EQ(backend.open("vcan0", can_config(0U, can_mode::loopback), nullptr), castle::status::system_call_error);

    // The lowest free descriptor number is unchanged, so the failed opens closed their sockets.
    const int after = ::open("/dev/null", O_RDONLY);
    EXPECT_EQ(after, before);
    if (after >= 0)
    {
        (void)::close(after);
    }
}

TEST(CanSocketCan, OpenOnANonCanInterfaceFailsWhileBinding)
{
    SKIP_WITHOUT_CAN_RAW();
    // "lo" resolves with SIOCGIFINDEX, but the kernel refuses to bind a CAN socket to a non-CAN device.
    socket_can::backend backend;
    int system_error = 0;
    EXPECT_EQ(backend.open("lo", can_config(), &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ENODEV);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);
}

TEST(CanSocketCan, CloseAndReopenReal)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    const int handle = backend.native_handle();
    EXPECT_EQ(backend.close(nullptr), castle::status::ok);
    EXPECT_FALSE(backend.is_open());
    errno = 0;
    EXPECT_EQ(::fcntl(handle, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    int system_error = sentinel_errno;
    EXPECT_EQ(backend.close(&system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);

    bus_status status;
    status.state = bus_state::error_active;
    EXPECT_EQ(backend.query_status(status, &system_error), castle::status::not_configured);
    EXPECT_EQ(status.state, bus_state::stopped);

    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    EXPECT_TRUE(backend.is_open());
    ASSERT_EQ(backend.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_active);
}

TEST(CanSocketCan, AddFilterInstallsKernelFiltersAndHandsOutSlots)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::basic_backend<2U> backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    const int handle = backend.native_handle();
    struct can_filter installed[4];

    filter_handle first = invalid_filter;
    int system_error = sentinel_errno;
    ASSERT_EQ(backend.add_filter(filter::exact(0x123U), first, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(first, 0);
    ASSERT_EQ(get_kernel_filters(handle, installed, 4U), 1U);
    EXPECT_EQ(installed[0].can_id, 0x123U);
    EXPECT_EQ(installed[0].can_mask, 0x7FFU | CAN_EFF_FLAG);

    filter_handle second = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter(0x1000U, 0x1FFFF000U, id_format::extended), second, nullptr), castle::status::ok);
    EXPECT_EQ(second, 1);
    ASSERT_EQ(get_kernel_filters(handle, installed, 4U), 2U);
    EXPECT_EQ(installed[0].can_id, 0x123U);
    EXPECT_EQ(installed[1].can_id, 0x1000U | CAN_EFF_FLAG);
    EXPECT_EQ(installed[1].can_mask, 0x1FFFF000U | CAN_EFF_FLAG);

    // Capacity reached: `full`, the handle stays invalid and the kernel list is unchanged.
    filter_handle third = 9;
    system_error = sentinel_errno;
    EXPECT_EQ(backend.add_filter(filter::any(), third, &system_error), castle::status::full);
    EXPECT_EQ(third, invalid_filter);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(get_kernel_filters(handle, installed, 4U), 2U);
}

TEST(CanSocketCan, RemoveFilterFreesSlotForReuse)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::basic_backend<2U> backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    const int handle = backend.native_handle();
    struct can_filter installed[4];

    filter_handle first = invalid_filter;
    filter_handle second = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x10U), first, nullptr), castle::status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x20U), second, nullptr), castle::status::ok);

    int system_error = sentinel_errno;
    EXPECT_EQ(backend.remove_filter(first, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    ASSERT_EQ(get_kernel_filters(handle, installed, 4U), 1U);
    EXPECT_EQ(installed[0].can_id, 0x20U);

    // Removing the same handle again, or one that was never handed out, is `not_found`.
    system_error = sentinel_errno;
    EXPECT_EQ(backend.remove_filter(first, &system_error), castle::status::not_found);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.remove_filter(-1, nullptr), castle::status::not_found);
    EXPECT_EQ(backend.remove_filter(2, nullptr), castle::status::not_found);
    EXPECT_EQ(backend.remove_filter(1000, nullptr), castle::status::not_found);

    // The freed slot is handed out again and the table is full once more.
    filter_handle reused = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x30U), reused, nullptr), castle::status::ok);
    EXPECT_EQ(reused, first);
    ASSERT_EQ(get_kernel_filters(handle, installed, 4U), 2U);
    filter_handle overflow = invalid_filter;
    EXPECT_EQ(backend.add_filter(filter::any(), overflow, nullptr), castle::status::full);

    // Removing the last remaining filters leaves an empty (receive-nothing) kernel list.
    EXPECT_EQ(backend.remove_filter(reused, nullptr), castle::status::ok);
    EXPECT_EQ(backend.remove_filter(second, nullptr), castle::status::ok);
    EXPECT_EQ(get_kernel_filters(handle, installed, 4U), 0U);
}

TEST(CanSocketCan, ClearFiltersEmptiesTheTableAndKernelList)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::basic_backend<2U> backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    const int handle = backend.native_handle();
    struct can_filter installed[4];

    filter_handle first = invalid_filter;
    filter_handle second = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), first, nullptr), castle::status::ok);
    ASSERT_EQ(backend.add_filter(filter::any(id_format::extended), second, nullptr), castle::status::ok);
    ASSERT_EQ(get_kernel_filters(handle, installed, 4U), 2U);

    int system_error = sentinel_errno;
    EXPECT_EQ(backend.clear_filters(&system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(get_kernel_filters(handle, installed, 4U), 0U);
    EXPECT_EQ(backend.remove_filter(first, nullptr), castle::status::not_found);
    EXPECT_EQ(backend.remove_filter(second, nullptr), castle::status::not_found);

    // Clearing an empty table is fine and handles start from zero again.
    EXPECT_EQ(backend.clear_filters(nullptr), castle::status::ok);
    filter_handle again = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), again, nullptr), castle::status::ok);
    EXPECT_EQ(again, 0);
}

TEST(CanSocketCan, CloseForgetsFiltersSoReopenStartsEmpty)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::basic_backend<2U> backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    filter_handle handle = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), handle, nullptr), castle::status::ok);
    ASSERT_EQ(backend.add_filter(filter::any(id_format::extended), handle, nullptr), castle::status::ok);
    ASSERT_EQ(backend.close(nullptr), castle::status::ok);

    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    struct can_filter installed[4];
    EXPECT_EQ(get_kernel_filters(backend.native_handle(), installed, 4U), 0U);
    EXPECT_EQ(backend.remove_filter(0, nullptr), castle::status::not_found);
    ASSERT_EQ(backend.add_filter(filter::any(), handle, nullptr), castle::status::ok);
    EXPECT_EQ(handle, 0);
    ASSERT_EQ(backend.add_filter(filter::any(), handle, nullptr), castle::status::ok);
    EXPECT_EQ(handle, 1);
}

TEST(CanSocketCan, ReceiveOnAnyBindingIsEmpty)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);
    filter_handle handle = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), handle, nullptr), castle::status::ok);
    ASSERT_EQ(backend.add_filter(filter::any(id_format::extended), handle, nullptr), castle::status::ok);

    frame out(0x66U);
    int system_error = sentinel_errno;
    EXPECT_EQ(backend.receive(out, no_wait, &system_error), castle::status::empty);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(out.id, 0x66U);

    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(backend.receive(out, castle::chrono::milliseconds(20), &system_error), castle::status::empty);
    EXPECT_GE(elapsed_ms(start), 19);
    EXPECT_LT(elapsed_ms(start), 2000);
    EXPECT_EQ(system_error, 0);

    // Without any filter the socket is just as silent.
    EXPECT_EQ(backend.clear_filters(nullptr), castle::status::ok);
    EXPECT_EQ(backend.receive(out, castle::chrono::milliseconds(10), nullptr), castle::status::empty);
}

TEST(CanSocketCan, SendOnAnyBindingFailsBecauseTheInterfaceIsUnspecified)
{
    SKIP_WITHOUT_CAN_RAW();
    socket_can::backend backend;
    ASSERT_EQ(backend.open("any", can_config(), nullptr), castle::status::ok);

    // "any" is a receive-only binding: the kernel has no outgoing interface for a plain write().
    int system_error = 0;
    const castle::status result = backend.send(frame(0x123U), no_wait, &system_error);
    EXPECT_EQ(result, castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_NE(system_error, EAGAIN);
    EXPECT_NE(system_error, ENOBUFS);
    // The timeout never changes a hard failure into a wait.
    system_error = 0;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(backend.send(frame(0x123U), castle::chrono::milliseconds(200), &system_error),
              castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_LT(elapsed_ms(start), 150);
    EXPECT_TRUE(backend.is_open());
}

TEST(CanSocketCan, BusFacadeOpensAcceptsAllAndCloses)
{
    SKIP_WITHOUT_CAN_RAW();
    basic_bus<socket_can::backend> bus;
    EXPECT_FALSE(bus.is_open());

    int system_error = sentinel_errno;
    ASSERT_EQ(bus.open("any", can_config(bitrate_500k), &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_TRUE(bus.is_open());
    EXPECT_GE(bus.backend().native_handle(), 0);

    ASSERT_EQ(bus.accept_all(&system_error), castle::status::ok);
    struct can_filter installed[4];
    ASSERT_EQ(get_kernel_filters(bus.backend().native_handle(), installed, 4U), 2U);
    EXPECT_EQ(installed[0].can_id, 0U);
    EXPECT_EQ(installed[0].can_mask, static_cast<canid_t>(CAN_EFF_FLAG));
    EXPECT_EQ(installed[1].can_id, static_cast<canid_t>(CAN_EFF_FLAG));
    EXPECT_EQ(installed[1].can_mask, static_cast<canid_t>(CAN_EFF_FLAG));

    frame out;
    EXPECT_EQ(bus.receive(out, no_wait, nullptr), castle::status::empty);
    bus_status status;
    ASSERT_EQ(bus.query_status(status, nullptr), castle::status::ok);
    EXPECT_EQ(status.state, bus_state::error_active);

    EXPECT_EQ(bus.close(&system_error), castle::status::ok);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.close(nullptr), castle::status::not_configured);
    EXPECT_EQ(bus.backend().native_handle(), -1);
}

#else // !CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

TEST(CanSocketCan, StubReturnsNotConfiguredForEveryCall)
{
    EXPECT_FALSE(socket_can::socket_can_available);

    socket_can::backend backend;
    int system_error = 77;
    EXPECT_EQ(backend.open("can0", can_config(), &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(backend.open(nullptr, can_config(), nullptr), castle::status::invalid_argument);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.native_handle(), -1);

    system_error = 77;
    EXPECT_EQ(backend.send(frame(0x1U), no_wait, &system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, &system_error), castle::status::not_configured);
    filter_handle handle = 5;
    EXPECT_EQ(backend.add_filter(filter::any(), handle, &system_error), castle::status::not_configured);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(backend.remove_filter(0, &system_error), castle::status::not_configured);
    EXPECT_EQ(backend.clear_filters(&system_error), castle::status::not_configured);
    bus_status status;
    EXPECT_EQ(backend.query_status(status, &system_error), castle::status::not_configured);
    EXPECT_EQ(status.state, bus_state::stopped);
    EXPECT_EQ(backend.close(&system_error), castle::status::not_configured);
}

#endif

} // namespace
