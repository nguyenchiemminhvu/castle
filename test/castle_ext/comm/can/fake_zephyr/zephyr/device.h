// Minimal host-side stand-in for <zephyr/device.h>, used only to compile and test the CASTLE
// Zephyr CAN backend without the Zephyr SDK. Signatures follow Zephyr 3.7.
#ifndef FAKE_ZEPHYR_DEVICE_H
#define FAKE_ZEPHYR_DEVICE_H

struct device
{
    const char* name;
    bool ready;
};

inline bool device_is_ready(const struct device* dev)
{
    return dev != nullptr && dev->ready;
}

#endif // FAKE_ZEPHYR_DEVICE_H
