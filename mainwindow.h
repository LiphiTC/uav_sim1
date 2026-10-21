#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "drone.h"
#include <QMainWindow>
#include <QElapsedTimer>
#include <QSet>
#include <initializer_list>

class QDoubleSpinBox;
class QLabel;
class QProgressBar;
class Scene3D;
class QMediaPlayer;
class BatteryGraph;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void advance_simulation(double seconds);

private:
    Drone drone;
    QDoubleSpinBox *x_input;
    QDoubleSpinBox *y_input;
    QDoubleSpinBox *altitude_input;
    QDoubleSpinBox *battery_input;
    QLabel *state_label;
    QLabel *message_label;
    QProgressBar *battery_bar;
    Scene3D *scene;
    QMediaPlayer *error_player;
    QMediaPlayer *low_battery_player;
    bool low_battery_warning_active = false;
    QSet<QDoubleSpinBox *> invalid_inputs;
    QElapsedTimer frame_clock;
    BatteryGraph *battery_graph;
    double simulation_seconds = 0.0;

    bool eventFilter(QObject *object, QEvent *event) override;
    bool validate_inputs(std::initializer_list<QDoubleSpinBox *> inputs);
    void show_message(const QString& text, bool error = false);
    void refresh_state();
};

#endif // MAINWINDOW_H
