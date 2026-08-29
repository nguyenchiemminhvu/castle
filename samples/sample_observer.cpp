#include "castle/design_patterns/observer.h"

#include <iostream>

using namespace castle::design_patterns;

struct imu_accel_data
{
    float x;
    float y;
    float z;
};

struct imu_gyro_data
{
    float x;
    float y;
    float z;
};

using imu_observer = observer<imu_accel_data, imu_gyro_data>;

class imu_sensor : public imu_observer
{
public:
    void notify(const imu_accel_data& data) override
    {
        std::cout << "Received IMU accel data: x=" << data.x << ", y=" << data.y << ", z=" << data.z << std::endl;
    }

    void notify(const imu_gyro_data& data) override
    {
        std::cout << "Received IMU gyro data: x=" << data.x << ", y=" << data.y << ", z=" << data.z << std::endl;
    }
};

class imu_accel_provider : public observable<imu_sensor, 3>
{
public:

};

class imu_gyro_provider : public observable<imu_sensor, 3>
{
public:

};

int main()
{
    imu_sensor sensor;
    imu_accel_provider accel_provider;
    imu_gyro_provider gyro_provider;

    accel_provider.add_observer(&sensor);
    gyro_provider.add_observer(&sensor);

    imu_accel_data accel_data{1.0f, 2.0f, 3.0f};
    imu_gyro_data gyro_data{4.0f, 5.0f, 6.0f};
    accel_provider.notify_observers(accel_data);
    gyro_provider.notify_observers(gyro_data);

    accel_provider.remove_observer(&sensor);
    gyro_provider.remove_observer(&sensor);

    return 0;
}