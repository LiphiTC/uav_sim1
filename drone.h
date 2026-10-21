#ifndef DRONE_H
#define DRONE_H

class Drone
{
private:
    int id;
    double x;
    double y;
    double z;
    double battery;
    bool flying;

    static bool valid_position(double new_x, double new_y, double new_z);

public:
    Drone();
    Drone(int drone_id, double initial_battery);

    void set_position(double new_x, double new_y, double new_z);
    void takeoff();
    void land();
    void move_to(double new_x, double new_y, double new_z);
    void set_battery(double value);
    void emergency_land();
    void update_telemetry(double new_x, double new_y, double new_z,
                          double new_battery);

    int get_id() const;
    double get_x() const;
    double get_y() const;
    double get_altitude() const;
    double get_battery() const;
    bool is_flying() const;
    bool has_valid_telemetry() const;
    void print_info() const;
};

#endif // DRONE_H
