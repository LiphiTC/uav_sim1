#include "drone.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

Drone::Drone()
    : id(0), x(0.0), y(0.0), z(0.0), battery(100.0), flying(false)
{
}

Drone::Drone(int drone_id, double initial_battery)
    : id(drone_id), x(0.0), y(0.0), z(0.0), battery(0.0), flying(false)
{
    set_battery(initial_battery);
}

bool Drone::valid_position(double new_x, double new_y, double new_z)
{
    return std::isfinite(new_x) && std::isfinite(new_y)
        && std::isfinite(new_z) && new_z >= 0.0;
}

void Drone::set_position(double new_x, double new_y, double new_z)
{
    if (!valid_position(new_x, new_y, new_z)) {
        std::cout << "Error: invalid coordinates or negative altitude\n";
        return;
    }
    x = new_x;
    y = new_y;
    z = new_z;
}

void Drone::takeoff()
{
    if (flying) {
        std::cout << "Drone is already flying\n";
        return;
    }
    if (battery < 10.0) {
        std::cout << "Takeoff denied: low battery\n";
        return;
    }
    flying = true;
    z = 1.0;
    battery -= 1.0;
    std::cout << "Drone took off\n";
}

void Drone::land()
{
    if (!flying) {
        std::cout << "Drone is already on the ground\n";
        return;
    }
    flying = false;
    z = 0.0;
    std::cout << "Drone landed\n";
}

void Drone::move_to(double new_x, double new_y, double new_z)
{
    if (!flying) {
        std::cout << "Move denied: drone is not flying\n";
        return;
    }
    if (!valid_position(new_x, new_y, new_z)) {
        std::cout << "Move denied: invalid coordinates or altitude\n";
        return;
    }
    if (battery < 5.0) {
        std::cout << "Move denied: critically low battery\n";
        return;
    }
    set_position(new_x, new_y, new_z);
    battery -= 1.0;
    std::cout << "Drone moved to new position\n";
}

void Drone::set_battery(double value)
{
    if (!std::isfinite(value)) {
        std::cout << "Error: battery must be a finite number\n";
        return;
    }
    battery = std::clamp(value, 0.0, 100.0);
}

void Drone::emergency_land()
{
    z = 0.0;
    flying = false;
    std::cout << "WARNING: Emergency landing activated\n"
              << "Drone landed immediately\n";
}

void Drone::update_telemetry(double new_x, double new_y, double new_z,
                             double new_battery)
{
    // Сначала проверяем весь набор, чтобы не записать данные частично.
    if (id < 0 || !valid_position(new_x, new_y, new_z)
        || !std::isfinite(new_battery)) {
        std::cout << "Telemetry rejected: invalid data or drone ID\n";
        return;
    }
    set_position(new_x, new_y, new_z);
    set_battery(new_battery);
    std::cout << "Telemetry updated successfully\n";
}

int Drone::get_id() const
{
    return id;
}

double Drone::get_x() const
{
    return x;
}

double Drone::get_y() const
{
    return y;
}

double Drone::get_altitude() const
{
    return z;
}

double Drone::get_battery() const
{
    return battery;
}

bool Drone::is_flying() const
{
    return flying;
}

bool Drone::has_valid_telemetry() const
{
    // ID 0 допустим: он используется конструктором по умолчанию в задании.
    return id >= 0 && valid_position(x, y, z) && std::isfinite(battery)
        && battery >= 0.0 && battery <= 100.0;
}

void Drone::print_info() const
{
    std::cout << std::fixed << std::setprecision(2)
              << "Drone ID: " << id << '\n'
              << "Position: (" << x << ", " << y << ", " << z << ")\n"
              << "Battery: " << battery << "%\n"
              << "Status: " << (flying ? "Flying" : "On ground") << '\n'
              << "Telemetry status: "
              << (has_valid_telemetry() ? "Valid" : "Invalid") << '\n';
}
