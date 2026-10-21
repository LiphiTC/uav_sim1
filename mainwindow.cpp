#include "mainwindow.h"
#include "scene3d.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Практика 1 — Управление БПЛА");
    resize(1150, 680);
    auto *central = new QWidget(this);
    auto *main_layout = new QHBoxLayout(central);
    auto *controls = new QWidget(central);
    controls->setMaximumWidth(410);
    auto *layout = new QVBoxLayout(controls);
    layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(controls);
    setCentralWidget(central);

    auto *state_group = new QGroupBox("Состояние БПЛА", central);
    auto *state_layout = new QVBoxLayout(state_group);
    state_label = new QLabel(state_group);
    state_label->setObjectName("stateLabel");
    state_label->setWordWrap(true);
    battery_bar = new QProgressBar(state_group);
    battery_bar->setObjectName("batteryBar");
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
    layout->addStretch();

    error_player = new QMediaPlayer(this);
    error_player->setObjectName("errorSoundPlayer");
    low_battery_player = new QMediaPlayer(this);
    low_battery_player->setObjectName("lowBatterySoundPlayer");
    for (auto *player : {error_player, low_battery_player}) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto *audio_output = new QAudioOutput(player);
        audio_output->setVolume(0.65f);
        player->setAudioOutput(audio_output);
        connect(player, &QMediaPlayer::errorOccurred, this, [] { QApplication::beep(); });
#else
        player->setVolume(65);
        connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
                this, [] { QApplication::beep(); });
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    error_player->setSource(QUrl("qrc:/assets/critical.mp3"));
    low_battery_player->setSource(QUrl("qrc:/assets/lowbattery.mp3"));
#else
    error_player->setMedia(QUrl("qrc:/assets/critical.mp3"));
    low_battery_player->setMedia(QUrl("qrc:/assets/lowbattery.mp3"));
#endif
    for (auto *input : {x_input, y_input, altitude_input, battery_input}) {
        auto *editor = input->findChild<QLineEdit *>();
        input->installEventFilter(this);
        editor->installEventFilter(this);
        connect(editor, &QLineEdit::inputRejected, this, [this, input] {
            invalid_inputs.insert(input);
            show_message("Ошибка ввода: введите число в допустимом диапазоне.", true);
        });
        connect(editor, &QLineEdit::textEdited, this, [this, input] {
            if (input->hasAcceptableInput())
                invalid_inputs.remove(input);
        });
        connect(input, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, input] {
            invalid_inputs.remove(input);
        });
    }

    auto *view_layout = new QVBoxLayout;
    auto *camera_buttons = new QHBoxLayout;
    auto *zoom_in = new QPushButton("Приблизить", central);
    auto *zoom_out = new QPushButton("Отдалить", central);
    auto *focus = new QPushButton("К БПЛА", central);
    auto *reset_camera = new QPushButton("Общий вид", central);
    zoom_in->setObjectName("zoomInButton");
    zoom_out->setObjectName("zoomOutButton");
    focus->setObjectName("focusDroneButton");
    reset_camera->setObjectName("resetCameraButton");
    camera_buttons->addWidget(zoom_in);
    camera_buttons->addWidget(zoom_out);
    camera_buttons->addWidget(focus);
    camera_buttons->addWidget(reset_camera);
    view_layout->addLayout(camera_buttons);
    scene = new Scene3D(central);
    scene->setObjectName("scene3d");
    view_layout->addWidget(scene, 1);
    auto *legend = new QLabel("Зелёный куб — начало координат; красный — заданная точка.\n"
                             "Жёлтая сфера — ориентир. Оси: X — красная, Y — зелёная, Z — синяя.\n"
                             "ПКМ — вращение, ЛКМ — сдвиг, колесо — масштаб.", central);
    legend->setWordWrap(true);
    view_layout->addWidget(legend);
    main_layout->addLayout(view_layout, 1);
    connect(zoom_in, &QPushButton::clicked, this, [this] { scene->zoom(0.8f); });
    connect(zoom_out, &QPushButton::clicked, this, [this] { scene->zoom(1.25f); });
    connect(focus, &QPushButton::clicked, scene, &Scene3D::focusDrone);
    connect(reset_camera, &QPushButton::clicked, scene, &Scene3D::resetCamera);
    auto update_target = [this] {
        scene->setTarget(x_input->value(), y_input->value(), altitude_input->value());
    };
    for (auto *input : {x_input, y_input, altitude_input}) {
        connect(input, qOverload<double>(&QDoubleSpinBox::valueChanged), this, update_target);
    }
    update_target();

    connect(takeoff_button, &QPushButton::clicked, this, [this] {
        const bool was_flying = drone.is_flying();
        drone.takeoff();
        show_message(was_flying ? "БПЛА уже в полёте."
            : drone.is_flying() ? "Взлёт выполнен."
                               : "Взлёт запрещён: требуется заряд не менее 10%.",
                     was_flying || !drone.is_flying());
        refresh_state();
    });
    connect(land_button, &QPushButton::clicked, this, [this] {
        const bool was_flying = drone.is_flying();
        drone.land();
        show_message(was_flying ? "Посадка выполнена." : "БПЛА уже на земле.", !was_flying);
        refresh_state();
    });
    connect(move_button, &QPushButton::clicked, this, [this] {
        if (!validate_inputs({x_input, y_input, altitude_input}))
            return;
        const double previous_battery = drone.get_battery();
        drone.move_to(x_input->value(), y_input->value(), altitude_input->value());
        if (!drone.is_flying()) {
            show_message("Перемещение запрещено: сначала выполните взлёт.", true);
        } else if (drone.get_battery() == previous_battery) {
            show_message("Перемещение запрещено: требуется заряд не менее 5%.", true);
        } else {
            show_message("БПЛА перемещён в заданную точку.");
        }
        refresh_state();
    });
    connect(emergency_button, &QPushButton::clicked, this, [this] {
        drone.emergency_land();
        show_message("Аварийная посадка: высота сброшена, полёт прекращён.");
        refresh_state();
    });
    connect(battery_button, &QPushButton::clicked, this, [this] {
        if (!validate_inputs({battery_input}))
            return;
        drone.set_battery(battery_input->value());
        show_message("Заряд обновлён.");
        refresh_state();
    });
    connect(telemetry_button, &QPushButton::clicked, this, [this] {
        if (!validate_inputs({x_input, y_input, altitude_input, battery_input}))
            return;
        drone.update_telemetry(x_input->value(), y_input->value(),
                               altitude_input->value(), battery_input->value());
        show_message("Телеметрия обновлена.");
        refresh_state();
    });
    refresh_state();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    auto *input = qobject_cast<QDoubleSpinBox *>(object);
    if (!input)
        input = qobject_cast<QDoubleSpinBox *>(object->parent());
    bool finishing = event->type() == QEvent::FocusOut;
    if (event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent *>(event)->key();
        finishing = key == Qt::Key_Return || key == Qt::Key_Enter;
    }
    if (input && finishing && !input->hasAcceptableInput()) {
        // Qt восстанавливает старое число при потере фокуса. Не применяем его
        // вместо незавершённого ввода, пока пользователь не исправит поле.
        invalid_inputs.insert(input);
        show_message("Ошибка ввода: завершите число в поле.", true);
    }
    return QMainWindow::eventFilter(object, event);
}

bool MainWindow::validate_inputs(std::initializer_list<QDoubleSpinBox *> inputs)
{
    for (auto *input : inputs) {
        if (!input->hasAcceptableInput() || invalid_inputs.contains(input)) {
            show_message("Ошибка ввода: исправьте выделенное поле.", true);
            input->setFocus();
            input->selectAll();
            return false;
        }
    }
    return true;
}

void MainWindow::show_message(const QString& text, bool error)
{
    message_label->setText(text);
    message_label->setStyleSheet(error ? "color: #c62828;" : "");
    error_player->stop();
    if (error) {
        low_battery_player->stop();
        error_player->setPosition(0);
        error_player->play();
    }
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
    const double battery = drone.get_battery();
    const QString color = battery >= 50.0 ? "#2e7d32" : battery >= 20.0 ? "#f9a825" : "#c62828";
    const QString level = battery >= 50.0 ? "Норма" : battery >= 20.0 ? "Низкий" : "Критический";
    battery_bar->setFormat(QString("%1% — %2").arg(battery, 0, 'f', 2).arg(level));
    battery_bar->setStyleSheet(QString(
        "QProgressBar { border: 2px solid %1; border-radius: 4px; background: #eeeeee;"
        " text-align: center; color: #111111; min-height: 22px; }"
        "QProgressBar::chunk { background-color: %1; }").arg(color));
    // Одно предупреждение при входе в критический диапазон, без повтора
    // при каждом обновлении координат или нажатии кнопки.
    const bool low_battery = battery < 20.0;
    if (low_battery && !low_battery_warning_active) {
        low_battery_player->setPosition(0);
        low_battery_player->play();
    } else if (!low_battery) {
        low_battery_player->stop();
    }
    low_battery_warning_active = low_battery;
    scene->updateDrone(drone);
}
