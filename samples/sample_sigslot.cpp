#include "castle/events/sigslot.h"

#include <cstdint>
#include <iostream>

//=====================================================
// Free function
//=====================================================

void free_function(std::uint16_t speed)
{
    std::cout
        << "  free_function      : speed = "
        << speed
        << std::endl;
}

//=====================================================
// Functor
//=====================================================

struct SpeedLogger
{
    void operator()(std::uint16_t speed) const
    {
        std::cout
            << "  SpeedLogger        : speed = "
            << speed
            << std::endl;
    }
};

//=====================================================
// Handler class
//=====================================================

class Vehicle
{
public:

    void on_speed_changed(std::uint16_t speed)
    {
        std::cout
            << "  Vehicle::speed     : speed = "
            << speed
            << std::endl;
    }

    void on_speed_changed_const(std::uint16_t speed) const
    {
        std::cout
            << "  Vehicle::const     : speed = "
            << speed
            << std::endl;
    }

    void on_fault(std::uint8_t code)
    {
        std::cout
            << "  Vehicle::fault     : code = "
            << static_cast<unsigned int>(code)
            << std::endl;
    }
};

//=====================================================
// Static member function
//=====================================================

class Diagnostics
{
public:

    static void on_speed_changed(std::uint16_t speed)
    {
        std::cout
            << "  Diagnostics::speed : speed = "
            << speed
            << std::endl;
    }
};

//=====================================================
// Main
//=====================================================

int main()
{
    using speed_signal_t =
        castle::sigslot::signal<
            4,
            void(std::uint16_t)>;

    using fault_signal_t =
        castle::sigslot::signal<
            2,
            void(std::uint8_t)>;

    speed_signal_t speed_changed;
    fault_signal_t fault_detected;

    Vehicle vehicle;

    //=====================================================
    // Member function
    //=====================================================

    std::cout << "==============================" << std::endl;
    std::cout << "sigslot - member function" << std::endl;
    std::cout << "==============================" << std::endl;

    auto vehicle_connection =
        speed_changed.connect(
            vehicle,
            &Vehicle::on_speed_changed);

    std::cout
        << "connection : "
        << (vehicle_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(100) --" << std::endl;

    speed_changed.emit(100U);

    //=====================================================
    // Const member function
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - const member function" << std::endl;
    std::cout << "==============================" << std::endl;

    const Vehicle const_vehicle;

    auto const_connection =
        speed_changed.connect(
            const_vehicle,
            &Vehicle::on_speed_changed_const);

    std::cout
        << "connection : "
        << (const_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(110) --" << std::endl;

    speed_changed.emit(110U);

    //=====================================================
    // Free function
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - free function" << std::endl;
    std::cout << "==============================" << std::endl;

    auto free_connection =
        speed_changed.connect(
            &free_function);

    std::cout
        << "connection : "
        << (free_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(120) --" << std::endl;

    speed_changed.emit(120U);

    //=====================================================
    // Static member function
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - static member function" << std::endl;
    std::cout << "==============================" << std::endl;

    auto static_connection =
        speed_changed.connect(
            &Diagnostics::on_speed_changed);

    std::cout
        << "connection : "
        << (static_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(130) --" << std::endl;

    speed_changed.emit(130U);

    //=====================================================
    // Functor
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - functor" << std::endl;
    std::cout << "==============================" << std::endl;

    SpeedLogger logger;

    auto functor_connection =
        speed_changed.connect(logger);

    std::cout
        << "connection : "
        << (functor_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(140) --" << std::endl;

    speed_changed.emit(140U);

    //=====================================================
    // Multiple slots
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - multiple slots" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout
        << "active connections : "
        << speed_changed.size()
        << std::endl;

    std::cout << "-- emit(150) --" << std::endl;

    speed_changed.emit(150U);

    //=====================================================
    // Lambda
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - lambda" << std::endl;
    std::cout << "==============================" << std::endl;

    auto lambda_connection =
        fault_detected.connect(
            [](std::uint8_t code)
            {
                std::cout
                    << "  lambda             : fault = "
                    << static_cast<unsigned int>(code)
                    << std::endl;
            });

    std::cout
        << "connection : "
        << (lambda_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(3) --" << std::endl;

    fault_detected.emit(3U);

    //=====================================================
    // Stateful lambda
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - stateful lambda" << std::endl;
    std::cout << "==============================" << std::endl;

    std::uint32_t counter = 0U;

    auto counter_connection =
        speed_changed.connect(
            [&counter](std::uint16_t)
            {
                ++counter;

                std::cout
                    << "  counter            : "
                    << counter
                    << std::endl;
            });

    std::cout << "connection : "
        << (counter_connection ? "ok" : "failed")
        << std::endl;

    std::cout << "-- emit(160) --" << std::endl;

    speed_changed.emit(160U);

    std::cout << "-- emit(170) --" << std::endl;

    speed_changed.emit(170U);

    std::cout
        << "counter = "
        << counter
        << std::endl;

    //=====================================================
    // Explicit disconnect
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - disconnect" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout
        << "before disconnect : "
        << speed_changed.size()
        << std::endl;

    vehicle_connection.disconnect();

    std::cout
        << "after disconnect  : "
        << speed_changed.size()
        << std::endl;

    std::cout << "-- emit(180) --" << std::endl;

    speed_changed.emit(180U);

    //=====================================================
    // RAII connection
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - RAII connection" << std::endl;
    std::cout << "==============================" << std::endl;

    {
        auto temporary_connection =
            speed_changed.connect(
                [](std::uint16_t speed)
                {
                    std::cout
                        << "  temporary lambda   : speed = "
                        << speed
                        << std::endl;
                });

        std::cout
            << "inside scope size : "
            << speed_changed.size()
            << std::endl;

        std::cout << "-- emit(190) --" << std::endl;

        speed_changed.emit(190U);
    }

    std::cout
        << "outside scope size: "
        << speed_changed.size()
        << std::endl;

    std::cout << "-- emit(200) --" << std::endl;

    speed_changed.emit(200U);

    //=====================================================
    // Capacity boundary
    //=====================================================

    std::cout << "\n==============================" << std::endl;
    std::cout << "sigslot - capacity boundary" << std::endl;
    std::cout << "==============================" << std::endl;

    speed_signal_t limited_signal;

    auto connection_1 =
        limited_signal.connect(
            [](std::uint16_t)
            {
                std::cout << "  slot 1" << std::endl;
            });

    auto connection_2 =
        limited_signal.connect(
            [](std::uint16_t)
            {
                std::cout << "  slot 2" << std::endl;
            });

    auto connection_3 =
        limited_signal.connect(
            [](std::uint16_t)
            {
                std::cout << "  slot 3" << std::endl;
            });

    auto connection_4 =
        limited_signal.connect(
            [](std::uint16_t)
            {
                std::cout << "  slot 4" << std::endl;
            });

    auto connection_5 =
        limited_signal.connect(
            [](std::uint16_t)
            {
                std::cout << "  slot 5" << std::endl;
            });

    std::cout
        << "capacity : "
        << limited_signal.capacity()
        << std::endl;

    std::cout
        << "size     : "
        << limited_signal.size()
        << std::endl;

    std::cout
        << "slot 1   : "
        << (connection_1 ? "connected" : "failed")
        << std::endl;

    std::cout
        << "slot 2   : "
        << (connection_2 ? "connected" : "failed")
        << std::endl;

    std::cout
        << "slot 3   : "
        << (connection_3 ? "connected" : "failed")
        << std::endl;

    std::cout
        << "slot 4   : "
        << (connection_4 ? "connected" : "failed")
        << std::endl;

    std::cout
        << "slot 5   : "
        << (connection_5 ? "connected" : "failed")
        << std::endl;

    std::cout << "-- emit(210) --" << std::endl;

    limited_signal.emit(210U);

    return 0;
}
