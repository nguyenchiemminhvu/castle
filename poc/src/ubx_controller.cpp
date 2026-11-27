#include "gnss/components.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace gnss
{
namespace
{

uint64_t parse_decimal(castle::dsa::string_view value, bool& valid) noexcept
{
    uint64_t result = 0U;
    valid = !value.empty();
    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        const char c = value[i];
        if (c < '0' || c > '9')
        {
            valid = false;
            return 0U;
        }
        const uint64_t digit = static_cast<uint64_t>(c - '0');
        if (result > ((UINT64_MAX - digit) / 10U))
        {
            valid = false;
            return 0U;
        }
        result = (result * 10U) + digit;
    }
    return result;
}

template <typename Message>
castle::status send_encoded(GnssTransport& transport, const Message& message) noexcept
{
    castle::dsa::array<uint8_t, 512U> frame{};
    castle::size_type frame_size = 0U;
    const castle::status encoded = castle::protocols::ubx::encode_message(
        message, frame.data(), frame.size(), frame_size);
    if (encoded != castle::status::ok)
    {
        return encoded;
    }
    castle::size_type sent = 0U;
    const castle::status written = transport.write(frame.data(), frame_size, sent);
    return (written == castle::status::ok && sent == frame_size)
        ? castle::status::ok : written;
}

} // namespace

UbxController::UbxController(const GnssTransConfig& config, GnssTransport& transport) noexcept
    : config_(config)
    , transport_(transport)
{
}

castle::status UbxController::init() noexcept
{
    return load_config();
}

castle::status UbxController::deinit() noexcept
{
    return castle::status::ok;
}

castle::status UbxController::suspend() noexcept
{
    restart_sync_on_resume_ = (sync_state_ == 1U || sync_state_ == 2U);
    if (restart_sync_on_resume_)
    {
        sync_state_ = 0U;
    }
    return castle::status::ok;
}

castle::status UbxController::resume() noexcept
{
    if (!restart_sync_on_resume_)
    {
        return castle::status::ok;
    }
    restart_sync_on_resume_ = false;
    return begin_boot_sync();
}

castle::status UbxController::shutdown() noexcept
{
    sync_state_ = 0U;
    return castle::status::ok;
}

castle::status UbxController::load_config() noexcept
{
    int fd = ::open(config_.config_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        return castle::status::system_call_error;
    }

    castle::dsa::array<char, 4096U> input{};
    const ssize_t read_count = ::read(fd, input.data(), input.size() - 1U);
    const int saved_errno = errno;
    static_cast<void>(::close(fd));
    errno = saved_errno;
    if (read_count <= 0 || static_cast<size_t>(read_count) >= input.size())
    {
        return castle::status::invalid_config;
    }

    using Document = castle::serialization::ini::document<64U, 1U, 16U, 32U>;
    Document document{};
    const castle::serialization::ini::result parsed =
        castle::serialization::ini::parse(document,
            castle::dsa::string_view(input.data(), static_cast<castle::size_type>(read_count)));
    if (parsed.status != castle::status::ok)
    {
        return parsed.status;
    }

    desired_.clear();
    for (castle::size_type i = 0U; i < document.size(); ++i)
    {
        bool key_valid = false;
        bool value_valid = false;
        const uint64_t raw_key = parse_decimal(document.key(i), key_valid);
        const uint64_t raw_value = parse_decimal(document.value(i), value_valid);
        if (!key_valid || !value_valid || raw_key > UINT32_MAX)
        {
            return castle::status::invalid_config;
        }
        const uint8_t value_width = castle::protocols::ubx::value_byte_size(
            static_cast<uint32_t>(raw_key));
        if (value_width == 0U ||
            (value_width < 8U && raw_value >= (static_cast<uint64_t>(1U) << (value_width * 8U))))
        {
            return castle::status::invalid_config;
        }
        const castle::status inserted = desired_.insert(
            static_cast<uint32_t>(raw_key), raw_value);
        if (inserted != castle::status::ok)
        {
            return inserted;
        }
    }
    return desired_.empty() ? castle::status::invalid_config : castle::status::ok;
}

castle::status UbxController::begin_boot_sync() noexcept
{
    if (sync_state_ != 0U)
    {
        return castle::status::invalid_state;
    }
    castle::protocols::ubx::messages::cfg_valget_poll<> poll{};
    poll.layer = castle::protocols::ubx::config::UBX_CFG_VALGET_LAYER_RAM;
    for (auto it = desired_.begin(); it != desired_.end(); ++it)
    {
        if (poll.keys.push_back(it->first) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    const castle::status result = send_encoded(transport_, poll);
    sync_state_ = (result == castle::status::ok) ? 1U : 4U;
    return result;
}

castle::status UbxController::on_valget(
    const castle::protocols::ubx::messages::cfg_valget<>& current) noexcept
{
    if (sync_state_ != 1U)
    {
        return castle::status::invalid_state;
    }

    castle::protocols::ubx::messages::cfg_valset<> update{};
    update.layers = castle::protocols::ubx::messages::cfg_valset_layers::LAYERS_RAM |
                    castle::protocols::ubx::messages::cfg_valset_layers::LAYERS_BBR |
                    castle::protocols::ubx::messages::cfg_valset_layers::LAYERS_FLASH;
    differing_items_ = 0U;

    for (auto desired = desired_.begin(); desired != desired_.end(); ++desired)
    {
        bool found = false;
        uint64_t current_value = 0U;
        for (castle::size_type i = 0U; i < current.entries.size(); ++i)
        {
            if (current.entries[i].key_id == desired->first)
            {
                found = true;
                current_value = current.entries[i].value;
                break;
            }
        }
        if (!found || current_value != desired->second)
        {
            castle::protocols::ubx::config::config_entry entry{};
            entry.key_id = desired->first;
            entry.value = castle::protocols::ubx::config::config_value(desired->second);
            if (update.entries.push_back(entry) != castle::status::ok)
            {
                sync_state_ = 4U;
                return castle::status::full;
            }
            ++differing_items_;
        }
    }

    if (update.entries.empty())
    {
        sync_state_ = 3U;
        return castle::status::ok;
    }
    const castle::status result = send_encoded(transport_, update);
    sync_state_ = (result == castle::status::ok) ? 2U : 4U;
    return result;
}

void UbxController::on_ack(bool accepted, uint8_t msg_class, uint8_t msg_id) noexcept
{
    if (sync_state_ != 2U ||
        msg_class != castle::protocols::ubx::UBX_CLASS_CFG ||
        msg_id != castle::protocols::ubx::UBX_ID_CFG_VALSET)
    {
        return;
    }
    sync_state_ = accepted ? 3U : 4U;
}

castle::status UbxController::cold_start() noexcept
{
    const castle::protocols::ubx::messages::cfg_rst reset =
        castle::protocols::ubx::messages::cfg_rst::cold_start();
    return send_encoded(transport_, reset);
}

const UbxController::ConfigMap& UbxController::desired() const noexcept
{
    return desired_;
}

uint8_t UbxController::sync_state() const noexcept
{
    return sync_state_;
}

uint32_t UbxController::differing_items() const noexcept
{
    return differing_items_;
}

} // namespace gnss
