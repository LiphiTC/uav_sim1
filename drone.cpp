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
    motion = Motion::Idle;
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
    battery -= 1.0;
    start_motion({x, y, takeoff_height}, Motion::TakingOff, 25.0);
    std::cout << "Drone taking off to 50 m\n";
}

void Drone::land()
{
    if (!flying && z == 0.0) {
        std::cout << "Drone is already on the ground\n";
        return;
    }
    if (is_landing()) {
        std::cout << "Drone is already landing\n";
        return;
    }
    start_motion({x, y, 0.0}, Motion::Landing, 25.0);
    std::cout << "Drone landing\n";
}

void Drone::move_to(double new_x, double new_y, double new_z)
{
    if (!flying) {
        std::cout << "Move denied: drone is not flying\n";
        return;
    }
    if (is_landing()) {
        std::cout << "Move denied: drone is landing\n";
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
    battery -= 1.0;
    start_motion({new_x, new_y, new_z}, Motion::Moving, 80.0);
    std::cout << "Drone moving to new position\n";
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
    if (motion == Motion::EmergencyLanding)
        return;
    start_motion({x, y, 0.0}, Motion::EmergencyLanding, 25.0);
    std::cout << "WARNING: Emergency landing activated\n"
              << "Drone descending vertically\n";
}

void Drone::start_motion(const Point3D& target, Motion next_motion, double speed)
{
    motion_start = {x, y, z};
    motion_target = target;
    motion_elapsed = 0.0;
    motion_duration = std::max(0.2, distance(motion_start, target) / speed);
    motion = next_motion;
    if (distance(motion_start, target) < 1e-9) {
        advance(motion_duration);
    } else {
        flying = true;
    }
}

void Drone::advance(double seconds)
{
    if (!is_moving() || !std::isfinite(seconds) || seconds <= 0.0)
        return;
    motion_elapsed = std::min(motion_duration, motion_elapsed + seconds);
    const double progress = motion_elapsed / motion_duration;
    // Smoothstep плавно разгоняет и останавливает БПЛА на прямом отрезке.
    const double smooth = progress * progress * (3.0 - 2.0 * progress);
    const Point3D position = motion_start + (motion_target - motion_start) * smooth;
    x = position.x;
    y = position.y;
    z = position.z;
    if (motion_elapsed >= motion_duration) {
        x = motion_target.x;
        y = motion_target.y;
        z = motion_target.z;
        flying = z > 0.0;
        motion = Motion::Idle;
    }
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

Drone::Motion Drone::get_motion() const
{
    return motion;
}

bool Drone::is_moving() const
{
    return motion != Motion::Idle;
}

bool Drone::is_landing() const
{
    return motion == Motion::Landing || motion == Motion::EmergencyLanding;
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
