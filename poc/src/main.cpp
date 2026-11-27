#include "gnss/components.hpp"
#include "castle/dsa/array.hpp"
#include "castle/events/signal_ipc_event.hpp"
#include "castle/events/signal_ipc_config.hpp"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

namespace
{

volatile sig_atomic_t g_tick_pending = 0;
volatile sig_atomic_t g_stop_requested = 0;

using SignalEvents = castle::events::signal_ipc_event<
    castle::events::signal_ipc_config<castle::events::signal::sigusr1, 1U, 32U>,
    castle::events::signal_ipc_config<castle::events::signal::sigint, 1U, 32U>,
    castle::events::signal_ipc_config<castle::events::signal::sigterm, 1U, 32U>>;

void print_status(const char* label, castle::status status) noexcept
{
    castle::string_builder<128U> line;
    line.build(label, " status=", static_cast<uint8_t>(status), "\n");
    static_cast<void>(::write(STDOUT_FILENO, line.c_str(), line.size()));
}

bool create_pty(int& master_fd, char* slave_path, size_t path_capacity) noexcept
{
    master_fd = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (master_fd < 0)
    {
        return false;
    }
    if (::grantpt(master_fd) != 0 || ::unlockpt(master_fd) != 0)
    {
        static_cast<void>(::close(master_fd));
        master_fd = -1;
        return false;
    }
    char* path = ::ptsname(master_fd);
    if (path == nullptr || ::strlen(path) + 1U > path_capacity)
    {
        static_cast<void>(::close(master_fd));
        master_fd = -1;
        return false;
    }
    static_cast<void>(::strcpy(slave_path, path));
    return true;
}

void pty_write(int master_fd, const uint8_t* bytes, size_t size) noexcept
{
    if (master_fd >= 0 && bytes != nullptr && size > 0U)
    {
        static_cast<void>(::write(master_fd, bytes, size));
    }
}

template <typename Message>
void send_ubx(int master_fd, const Message& message) noexcept
{
    castle::dsa::array<uint8_t, 512U> frame{};
    castle::size_type frame_size = 0U;
    if (castle::protocols::ubx::encode_message(message, frame.data(), frame.size(), frame_size)
        == castle::status::ok)
    {
        pty_write(master_fd, frame.data(), frame_size);
    }
}

void send_simulated_config_reply(int master_fd, const gnss::UbxController& controller) noexcept
{
    castle::protocols::ubx::messages::cfg_valget<> current{};
    const auto& desired = controller.desired();
    for (auto it = desired.begin(); it != desired.end(); ++it)
    {
        castle::protocols::ubx::messages::cfg_valget_entry value{};
        value.key_id = it->first;
        value.value = 0U;
        if (current.entries.push_back(value) != castle::status::ok)
        {
            return;
        }
    }
    send_ubx(master_fd, current);

    castle::protocols::ubx::messages::ack_ack ack{};
    ack.cls_id = castle::protocols::ubx::UBX_CLASS_CFG;
    ack.acked_msg_id = castle::protocols::ubx::UBX_ID_CFG_VALSET;
    send_ubx(master_fd, ack);
}

void send_simulated_pvt(int master_fd, uint32_t sequence) noexcept
{
    castle::protocols::ubx::messages::nav_pvt pvt{};
    pvt.year = 2026U;
    pvt.month = 11U;
    pvt.day = 27U;
    pvt.hour = 14U;
    pvt.minute = static_cast<uint8_t>((sequence / 60U) % 60U);
    pvt.second = static_cast<uint8_t>(sequence % 60U);
    pvt.valid = static_cast<uint8_t>(pvt.VALID_DATE | pvt.VALID_TIME | pvt.VALID_FULLY);
    pvt.fix_type = castle::protocols::ubx::messages::nav_pvt_fix_type::fix_3d;
    pvt.flags = pvt.FLAGS_GNSS_FIX_OK;
    pvt.num_sv = 15U;
    pvt.lat = 160678000; // 16.0678 degrees N
    pvt.lon = 1082208000 + static_cast<int32_t>(sequence * 6500U); // move east ~55 m/update
    pvt.h_msl = 12500;
    pvt.h_acc = 850U;
    pvt.p_dop = 95U;
    send_ubx(master_fd, pvt);
}

void send_simulated_nmea(int master_fd) noexcept
{
    static const uint8_t sentence[] =
        "$GPGGA,123519,1604.068,N,10813.248,E,1,08,0.9,12.5,M,46.9,M,,*7B\r\n";
    pty_write(master_fd, sentence, sizeof(sentence) - 1U);
}

uint32_t parse_u32(const char* text, uint32_t fallback) noexcept
{
    if (text == nullptr || *text == '\0')
    {
        return fallback;
    }
    const unsigned long value = ::strtoul(text, nullptr, 10);
    return value > 0xFFFFFFFFUL ? fallback : static_cast<uint32_t>(value);
}

} // namespace

int main(int argc, char** argv)
{
    bool simulation = true;
    bool simulation_stall = false;
    uint32_t run_for_seconds = 12U;
    const char* requested_uart = nullptr;
    const char* requested_config = nullptr;

    for (int i = 1; i < argc; ++i)
    {
        if (::strcmp(argv[i], "--simulate") == 0)
        {
            simulation = true;
        }
        else if (::strcmp(argv[i], "--hardware") == 0)
        {
            simulation = false;
        }
        else if (::strcmp(argv[i], "--simulate-stall") == 0)
        {
            simulation = true;
            simulation_stall = true;
        }
        else if (::strcmp(argv[i], "--uart") == 0 && (i + 1) < argc)
        {
            requested_uart = argv[++i];
        }
        else if (::strcmp(argv[i], "--config") == 0 && (i + 1) < argc)
        {
            requested_config = argv[++i];
        }
        else if (::strcmp(argv[i], "--run-for") == 0 && (i + 1) < argc)
        {
            run_for_seconds = parse_u32(argv[++i], 12U);
        }
    }

    auto tick_subscription = SignalEvents::register_callback<castle::events::signal::sigusr1>(
        []() noexcept
        {
            g_tick_pending = 1;
        });
    auto int_subscription = SignalEvents::register_callback<castle::events::signal::sigint>(
        []() noexcept
        {
            g_stop_requested = 1;
        });
    auto term_subscription = SignalEvents::register_callback<castle::events::signal::sigterm>(
        []() noexcept
        {
            g_stop_requested = 1;
        });
    const castle::status signal_status = SignalEvents::install();
    if (signal_status != castle::status::ok)
    {
        print_status("signal_install", signal_status);
        return 2;
    }

    int pty_master = -1;
    castle::dsa::array<char, 128U> pty_slave{};
    if (simulation && !create_pty(pty_master, pty_slave.data(), pty_slave.size()))
    {
        print_status("pty_create", castle::status::system_call_error);
        static_cast<void>(SignalEvents::uninstall());
        return 3;
    }

    gnss::LocationService service;
    service.config().simulation = simulation;
    if (simulation)
    {
        static_cast<void>(service.config().uart_path.assign(pty_slave.data()));
        static_cast<void>(service.config().config_path.assign(
            requested_config != nullptr ? requested_config : "./ubx_config.cfg"));
    }
    else
    {
        if (requested_uart != nullptr)
        {
            static_cast<void>(service.config().uart_path.assign(requested_uart));
        }
        if (requested_config != nullptr)
        {
            static_cast<void>(service.config().config_path.assign(requested_config));
        }
    }

    const castle::status started = service.init();
    print_status("location_service_init", started);
    if (started != castle::status::ok)
    {
        static_cast<void>(service.shutdown());
        if (pty_master >= 0)
        {
            static_cast<void>(::close(pty_master));
        }
        static_cast<void>(SignalEvents::uninstall());
        return 4;
    }

    if (simulation)
    {
        send_simulated_config_reply(pty_master, service.controller());
        send_simulated_nmea(pty_master);
        send_simulated_pvt(pty_master, 0U);
    }

    uint32_t last_tick_ms = gnss::monotonic_milliseconds();
    const uint32_t simulation_end = last_tick_ms + (run_for_seconds * 1000U);
    uint32_t next_simulated_pvt = last_tick_ms + 1000U;
    uint32_t pvt_sequence = 1U;
    bool config_sync_reported = false;

    while (g_stop_requested == 0)
    {
        const uint32_t now_ms = gnss::monotonic_milliseconds();
        if (simulation && static_cast<int32_t>(now_ms - simulation_end) >= 0)
        {
            break;
        }

        if (g_tick_pending != 0)
        {
            g_tick_pending = 0;
            const uint32_t elapsed = static_cast<uint32_t>(now_ms - last_tick_ms) / 10U;
            const uint32_t tick_count = elapsed == 0U ? 1U : (elapsed > 100U ? 100U : elapsed);
            service.update(now_ms, tick_count);
            last_tick_ms = now_ms;
            if (!config_sync_reported && service.controller().sync_state() >= 3U)
            {
                castle::string_builder<128U> line;
                line.build("UBX config sync state=", service.controller().sync_state(),
                           " differing_items=", service.controller().differing_items(), "\n");
                static_cast<void>(::write(STDOUT_FILENO, line.c_str(), line.size()));
                config_sync_reported = true;
            }
        }

        if (simulation && !simulation_stall && static_cast<int32_t>(now_ms - next_simulated_pvt) >= 0)
        {
            send_simulated_pvt(pty_master, pvt_sequence++);
            next_simulated_pvt += 1000U;
        }

        struct timespec sleep_time{};
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 1000000L;
        static_cast<void>(::nanosleep(&sleep_time, nullptr));
    }

    const castle::status stopped = service.shutdown();
    print_status("location_service_shutdown", stopped);
    static_cast<void>(SignalEvents::uninstall());
    tick_subscription.unsubscribe();
    int_subscription.unsubscribe();
    term_subscription.unsubscribe();
    if (pty_master >= 0)
    {
        static_cast<void>(::close(pty_master));
    }
    return stopped == castle::status::ok ? 0 : 5;
}
