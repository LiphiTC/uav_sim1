#include "drone.h"

#include <cstdlib>
#include <iostream>
#include <limits>

// В отличие от assert, проверка выполняется и при сборке Release.
void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    Drone drone;
    check(drone.get_id() == 0 && drone.get_battery() == 100.0
          && drone.get_x() == 0.0 && drone.get_y() == 0.0
          && drone.get_altitude() == 0.0 && !drone.is_flying()
          && drone.has_valid_telemetry(), "default state");

    drone.move_to(10.0, 20.0, 30.0);
    check(drone.get_x() == 0.0 && drone.get_battery() == 100.0,
          "cannot move before takeoff");
    drone.takeoff();
    check(drone.is_flying() && drone.get_altitude() == 1.0
          && drone.get_battery() == 99.0, "takeoff state and battery cost");
    drone.takeoff();
    check(drone.get_battery() == 99.0, "repeated takeoff has no cost");
    drone.move_to(10.0, -20.0, 50.0);
    check(drone.get_x() == 10.0 && drone.get_y() == -20.0
          && drone.get_altitude() == 50.0 && drone.get_battery() == 98.0,
          "movement coordinates and battery cost");
    drone.move_to(99.0, 99.0, -1.0);
    check(drone.get_x() == 10.0 && drone.get_altitude() == 50.0
          && drone.get_battery() == 98.0, "reject negative movement altitude");
    drone.land();
    drone.land();
    check(!drone.is_flying() && drone.get_altitude() == 0.0
          && drone.get_x() == 10.0 && drone.get_battery() == 98.0,
          "landing preserves horizontal position and battery");

    Drone low_battery(2, 9.99);
    low_battery.takeoff();
    check(!low_battery.is_flying(), "reject takeoff below 10 percent");
    low_battery.set_battery(10.0);
    low_battery.takeoff();
    check(low_battery.is_flying() && low_battery.get_battery() == 9.0,
          "allow takeoff at 10 percent");
    low_battery.set_battery(4.99);
    low_battery.move_to(2.0, 3.0, 4.0);
    check(low_battery.get_altitude() == 1.0 && low_battery.get_battery() == 4.99,
          "reject movement below 5 percent");
    low_battery.set_battery(5.0);
    low_battery.move_to(2.0, 3.0, 4.0);
    check(low_battery.get_altitude() == 4.0 && low_battery.get_battery() == 4.0,
          "allow movement at 5 percent");
    low_battery.set_battery(0.0);
    low_battery.emergency_land();
    check(!low_battery.is_flying() && low_battery.get_altitude() == 0.0,
          "emergency landing with empty battery");
    low_battery.set_position(2.0, 3.0, 20.0);
    low_battery.emergency_land();
    check(low_battery.get_altitude() == 0.0, "emergency landing always resets altitude");

    check(Drone(1, -20.0).get_battery() == 0.0, "constructor lower battery limit");
    check(Drone(1, 150.0).get_battery() == 100.0, "constructor upper battery limit");
    drone.update_telemetry(-30.0, 40.0, 120.0, 150.0);
    check(drone.get_x() == -30.0 && drone.get_y() == 40.0
          && drone.get_altitude() == 120.0 && drone.get_battery() == 100.0
          && drone.has_valid_telemetry(), "telemetry update and battery clamping");
    drone.update_telemetry(1.0, 2.0, -5.0, 50.0);
    check(drone.get_x() == -30.0 && drone.get_altitude() == 120.0
          && drone.get_battery() == 100.0, "reject entire invalid telemetry update");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    drone.update_telemetry(infinity, 0.0, 0.0, 10.0);
    drone.update_telemetry(1.0, 2.0, 3.0, nan);
    drone.set_position(1.0, nan, 3.0);
    drone.set_position(1.0, 2.0, infinity);
    drone.set_battery(nan);
    check(drone.get_x() == -30.0 && drone.get_y() == 40.0
          && drone.get_altitude() == 120.0 && drone.get_battery() == 100.0,
          "reject non-finite values without partial updates");
    drone.update_telemetry(1.0, 2.0, 3.0, -5.0);
    check(drone.get_battery() == 0.0 && drone.has_valid_telemetry(),
          "telemetry lower battery limit");

    Drone invalid(-1, 80.0);
    invalid.update_telemetry(1.0, 2.0, 3.0, 50.0);
    check(!invalid.has_valid_telemetry() && invalid.get_altitude() == 0.0
          && invalid.get_battery() == 80.0, "invalid identifier");
    std::cout << "All drone checks passed\n";
    return 0;
}
