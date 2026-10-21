#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "drone.h"
#include <QMainWindow>

class QDoubleSpinBox;
class QLabel;
class QProgressBar;
class Scene3D;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

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

    void refresh_state();
};

#endif // MAINWINDOW_H
