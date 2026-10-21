#include "drone.h"

#include <QCoreApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::cout << "Practice 1 / Task 1 / Variant 1: flight control\n";
    Drone drone;
    drone.print_info();
    drone.move_to(1.0, 2.0, 10.0); // Движение до взлёта запрещено.
    drone.takeoff();
    drone.takeoff();              // Повторный взлёт не расходует заряд.
    drone.advance(2.0);
    drone.move_to(10.0, 20.0, 50.0);
    drone.advance(10.0);
    drone.print_info();
    drone.land();
    drone.advance(2.0);
    drone.print_info();
    drone.takeoff();
    drone.advance(1.0);
    drone.emergency_land();
    drone.advance(2.0);
    drone.print_info();

    Drone low_battery_drone(2, 5.0);
    low_battery_drone.takeoff();
    low_battery_drone.print_info();

    std::cout << "\nVariant 2: telemetry control\n";
    Drone survey_drone(3, 80.0);
    survey_drone.takeoff();
    survey_drone.update_telemetry(30.0, -10.0, 120.0, 75.0);
    survey_drone.print_info();
    survey_drone.update_telemetry(40.0, 20.0, -5.0, 70.0);
    survey_drone.print_info(); // Ошибочное обновление не меняет данные.
    survey_drone.update_telemetry(40.0, 20.0, 150.0, 120.0);
    survey_drone.print_info(); // Заряд ограничен значением 100%.
    survey_drone.emergency_land();
    survey_drone.advance(10.0);
    survey_drone.print_info();

    Drone invalid_drone(-1, 50.0);
    invalid_drone.print_info();
    return 0;
}
