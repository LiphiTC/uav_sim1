#ifndef DRONE_H
#define DRONE_H

#include "point3d.h"

class Drone
{
public:
    enum class Motion { Idle, TakingOff, Moving, Landing, EmergencyLanding };
    static constexpr double takeoff_height = 50.0;

private:
    int id;
    double x;
    double y;
    double z;
    double battery;
    bool flying;
    Motion motion = Motion::Idle;
    Point3D motion_start;
    Point3D motion_target;
    double motion_elapsed = 0.0;
    double motion_duration = 0.0;

    static bool valid_position(double new_x, double new_y, double new_z);
    void start_motion(const Point3D& target, Motion next_motion, double speed);

public:
    Drone();
    Drone(int drone_id, double initial_battery);

    void set_position(double new_x, double new_y, double new_z);
    void takeoff();
    void land();
    void move_to(double new_x, double new_y, double new_z);
    void set_battery(double value);
    void emergency_land();
    void advance(double seconds);
    void update_telemetry(double new_x, double new_y, double new_z,
                          double new_battery);

    int get_id() const;
    double get_x() const;
    double get_y() const;
    double get_altitude() const;
    double get_battery() const;
    bool is_flying() const;
    Motion get_motion() const;
    bool is_moving() const;
    bool is_landing() const;
    bool has_valid_telemetry() const;
    void print_info() const;
};

#endif // DRONE_H
