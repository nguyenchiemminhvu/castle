// Host-side stand-in for <zephyr/drivers/can.h> (Zephyr 3.7 signatures), used only to compile and
// test the CASTLE Zephyr CAN backend without the Zephyr SDK.
//
// `fake_can` is a scriptable controller: tests set the *_result members to make a call fail,
// inspect what the backend asked the "driver" to do, and call `fake_can.deliver()` to make the
// registered receive callbacks fire the way the MCP2515 driver would.
#ifndef FAKE_ZEPHYR_DRIVERS_CAN_H
#define FAKE_ZEPHYR_DRIVERS_CAN_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <errno.h>
#include <stdint.h>

#define CAN_MODE_NORMAL     0U
#define CAN_MODE_LOOPBACK   (1U << 0)
#define CAN_MODE_LISTENONLY (1U << 1)

#define CAN_FRAME_IDE (1U << 0)
#define CAN_FRAME_RTR (1U << 1)
#define CAN_FRAME_FDF (1U << 2)

#define CAN_FILTER_IDE (1U << 0)

typedef uint32_t can_mode_t;

enum can_state
{
    CAN_STATE_ERROR_ACTIVE,
    CAN_STATE_ERROR_WARNING,
    CAN_STATE_ERROR_PASSIVE,
    CAN_STATE_BUS_OFF,
    CAN_STATE_STOPPED,
};

struct can_frame
{
    uint32_t id;
    uint8_t dlc;
    uint8_t flags;
    uint16_t reserved;
    union
    {
        uint8_t data[8];
        uint32_t data_32[2];
    };
};

struct can_filter
{
    uint32_t id;
    uint32_t mask;
    uint8_t flags;
};

struct can_bus_err_cnt
{
    uint8_t tx_err_cnt;
    uint8_t rx_err_cnt;
};

typedef void (*can_tx_callback_t)(const struct device* dev, int error, void* user_data);
typedef void (*can_rx_callback_t)(const struct device* dev, struct can_frame* frame, void* user_data);

struct fake_can_controller
{
    static const int filter_slots = 4;

    struct filter_slot
    {
        bool used;
        can_rx_callback_t callback;
        void* user_data;
        struct can_filter filter;
    };

    // Scripted return values; zero means success.
    int stop_result;
    int start_result;
    int set_mode_result;
    int set_bitrate_result;
    int send_result;
    int add_filter_result;
    int get_state_result;

    // Scripted controller state.
    enum can_state state;
    struct can_bus_err_cnt counters;

    // What the backend asked for.
    bool started;
    int stop_calls;
    int start_calls;
    int send_calls;
    int remove_calls;
    can_mode_t mode;
    uint32_t bitrate;
    struct can_frame sent_frame;
    k_timeout_t send_timeout;
    can_tx_callback_t tx_callback;
    void* tx_user_data;
    filter_slot filters[filter_slots];

    fake_can_controller() { reset(); }

    void reset()
    {
        stop_result = start_result = set_mode_result = set_bitrate_result = 0;
        send_result = add_filter_result = get_state_result = 0;
        state = CAN_STATE_ERROR_ACTIVE;
        counters = can_bus_err_cnt{0U, 0U};
        started = false;
        stop_calls = start_calls = send_calls = remove_calls = 0;
        mode = CAN_MODE_NORMAL;
        bitrate = 0U;
        sent_frame = can_frame{};
        send_timeout = K_NO_WAIT;
        tx_callback = nullptr;
        tx_user_data = nullptr;
        for (int i = 0; i < filter_slots; ++i)
        {
            filters[i] = filter_slot{false, nullptr, nullptr, can_filter{0U, 0U, 0U}};
        }
    }

    int installed_filters() const
    {
        int count = 0;
        for (int i = 0; i < filter_slots; ++i)
        {
            count += filters[i].used ? 1 : 0;
        }
        return count;
    }

    // Plays the controller receiving `frame`: every matching filter fires its callback.
    int deliver(const struct device* dev, struct can_frame frame)
    {
        int fired = 0;
        for (int i = 0; i < filter_slots; ++i)
        {
            const filter_slot& slot = filters[i];
            const bool extended = (frame.flags & CAN_FRAME_IDE) != 0U;
            const bool filter_extended = (slot.filter.flags & CAN_FILTER_IDE) != 0U;
            if (slot.used && extended == filter_extended
                && ((frame.id ^ slot.filter.id) & slot.filter.mask) == 0U)
            {
                slot.callback(dev, &frame, slot.user_data);
                ++fired;
            }
        }
        return fired;
    }
};

inline fake_can_controller fake_can;

inline int can_stop(const struct device*)
{
    ++fake_can.stop_calls;
    if (fake_can.stop_result != 0)
    {
        return fake_can.stop_result;
    }
    if (!fake_can.started)
    {
        return -EALREADY;
    }
    fake_can.started = false;
    return 0;
}

inline int can_start(const struct device*)
{
    ++fake_can.start_calls;
    if (fake_can.start_result != 0)
    {
        return fake_can.start_result;
    }
    fake_can.started = true;
    return 0;
}

inline int can_set_mode(const struct device*, can_mode_t mode)
{
    if (fake_can.set_mode_result == 0)
    {
        fake_can.mode = mode;
    }
    return fake_can.set_mode_result;
}

inline int can_set_bitrate(const struct device*, uint32_t bitrate)
{
    if (fake_can.set_bitrate_result == 0)
    {
        fake_can.bitrate = bitrate;
    }
    return fake_can.set_bitrate_result;
}

inline int can_send(const struct device*,
                    const struct can_frame* frame,
                    k_timeout_t timeout,
                    can_tx_callback_t callback,
                    void* user_data)
{
    ++fake_can.send_calls;
    fake_can.sent_frame = *frame;
    fake_can.send_timeout = timeout;
    fake_can.tx_callback = callback;
    fake_can.tx_user_data = user_data;
    return fake_can.send_result;
}

inline int can_add_rx_filter(const struct device*,
                             can_rx_callback_t callback,
                             void* user_data,
                             const struct can_filter* filter)
{
    if (fake_can.add_filter_result != 0)
    {
        return fake_can.add_filter_result;
    }
    for (int i = 0; i < fake_can_controller::filter_slots; ++i)
    {
        if (!fake_can.filters[i].used)
        {
            fake_can.filters[i] = fake_can_controller::filter_slot{true, callback, user_data, *filter};
            return i;
        }
    }
    return -ENOSPC;
}

inline void can_remove_rx_filter(const struct device*, int filter_id)
{
    ++fake_can.remove_calls;
    fake_can.filters[filter_id].used = false;
}

inline int can_get_state(const struct device*, enum can_state* state, struct can_bus_err_cnt* err_cnt)
{
    if (fake_can.get_state_result != 0)
    {
        return fake_can.get_state_result;
    }
    *state = fake_can.state;
    *err_cnt = fake_can.counters;
    return 0;
}

#endif // FAKE_ZEPHYR_DRIVERS_CAN_H
