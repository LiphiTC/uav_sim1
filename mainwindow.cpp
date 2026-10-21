#include "mainwindow.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Практика 1 — Управление БПЛА");
    resize(500, 460);
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    setCentralWidget(central);

    auto *state_group = new QGroupBox("Состояние БПЛА", central);
    auto *state_layout = new QVBoxLayout(state_group);
    state_label = new QLabel(state_group);
    state_label->setObjectName("stateLabel");
    state_label->setWordWrap(true);
    battery_bar = new QProgressBar(state_group);
    battery_bar->setRange(0, 100);
    state_layout->addWidget(state_label);
    state_layout->addWidget(battery_bar);
    layout->addWidget(state_group);

    auto *input_group = new QGroupBox("Координаты и телеметрия", central);
    auto *form = new QFormLayout(input_group);
    auto make_input = [input_group](const char *name, double minimum, double maximum,
                                   double value, const QString& suffix) {
        auto *input = new QDoubleSpinBox(input_group);
        input->setObjectName(name);
        input->setRange(minimum, maximum);
        input->setDecimals(2);
        input->setValue(value);
        input->setSuffix(suffix);
        return input;
    };
    x_input = make_input("xInput", -1000000.0, 1000000.0, 10.0, " м");
    y_input = make_input("yInput", -1000000.0, 1000000.0, 20.0, " м");
    altitude_input = make_input("altitudeInput", 0.0, 100000.0, 50.0, " м");
    battery_input = make_input("batteryInput", 0.0, 100.0, 100.0, " %");
    form->addRow("X:", x_input);
    form->addRow("Y:", y_input);
    form->addRow("Высота:", altitude_input);
    form->addRow("Заряд:", battery_input);
    layout->addWidget(input_group);

    auto *buttons = new QGridLayout;
    auto add_button = [central, buttons](const QString& text, const char *name,
                                         int row, int column) {
        auto *button = new QPushButton(text, central);
        button->setObjectName(name);
        buttons->addWidget(button, row, column);
        return button;
    };
    auto *takeoff_button = add_button("Взлёт", "takeoffButton", 0, 0);
    auto *land_button = add_button("Посадка", "landButton", 0, 1);
    auto *move_button = add_button("Переместить", "moveButton", 1, 0);
    auto *emergency_button = add_button("Аварийная посадка", "emergencyButton", 1, 1);
    auto *battery_button = add_button("Установить заряд", "batteryButton", 2, 0);
    auto *telemetry_button = add_button("Обновить телеметрию", "telemetryButton", 2, 1);
    layout->addLayout(buttons);

    message_label = new QLabel("Задайте координаты и выберите действие.", central);
    message_label->setObjectName("messageLabel");
    message_label->setWordWrap(true);
    message_label->setMinimumHeight(40);
    layout->addWidget(message_label);

    connect(takeoff_button, &QPushButton::clicked, this, [this] {
        const bool was_flying = drone.is_flying();
        drone.takeoff();
        message_label->setText(was_flying ? "БПЛА уже в полёте."
            : drone.is_flying() ? "Взлёт выполнен."
                               : "Взлёт запрещён: требуется заряд не менее 10%.");
        refresh_state();
    });
    connect(land_button, &QPushButton::clicked, this, [this] {
        const bool was_flying = drone.is_flying();
        drone.land();
        message_label->setText(was_flying ? "Посадка выполнена." : "БПЛА уже на земле.");
        refresh_state();
    });
    connect(move_button, &QPushButton::clicked, this, [this] {
        const double previous_battery = drone.get_battery();
        drone.move_to(x_input->value(), y_input->value(), altitude_input->value());
        if (!drone.is_flying()) {
            message_label->setText("Перемещение запрещено: сначала выполните взлёт.");
        } else if (drone.get_battery() == previous_battery) {
            message_label->setText("Перемещение запрещено: требуется заряд не менее 5%.");
        } else {
            message_label->setText("БПЛА перемещён в заданную точку.");
        }
        refresh_state();
    });
    connect(emergency_button, &QPushButton::clicked, this, [this] {
        drone.emergency_land();
        message_label->setText("Аварийная посадка: высота сброшена, полёт прекращён.");
        refresh_state();
    });
    connect(battery_button, &QPushButton::clicked, this, [this] {
        drone.set_battery(battery_input->value());
        message_label->setText("Заряд обновлён.");
        refresh_state();
    });
    connect(telemetry_button, &QPushButton::clicked, this, [this] {
        drone.update_telemetry(x_input->value(), y_input->value(),
                               altitude_input->value(), battery_input->value());
        message_label->setText("Телеметрия обновлена.");
        refresh_state();
    });
    refresh_state();
}

void MainWindow::refresh_state()
{
    state_label->setText(QString("ID: %1   |   %2\nX: %3 м   Y: %4 м   Высота: %5 м\n"
                                 "Заряд: %6%   |   Телеметрия: %7")
        .arg(drone.get_id())
        .arg(drone.is_flying() ? "В полёте" : "На земле")
        .arg(drone.get_x(), 0, 'f', 2)
        .arg(drone.get_y(), 0, 'f', 2)
        .arg(drone.get_altitude(), 0, 'f', 2)
        .arg(drone.get_battery(), 0, 'f', 2)
        .arg(drone.has_valid_telemetry() ? "Корректна" : "Некорректна"));
    battery_bar->setValue(static_cast<int>(std::round(drone.get_battery())));
}
