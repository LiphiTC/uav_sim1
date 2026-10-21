#include "mainwindow.h"
#include "batterygraph.h"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFocusEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/QRenderCapture>
#include <cmath>
#include <cstdlib>
#include <iostream>

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool is_playing(QMediaPlayer *player)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return player->playbackState() == QMediaPlayer::PlayingState;
#else
    return player->state() == QMediaPlayer::PlayingState;
#endif
}

int play_count(const QSignalSpy& events)
{
    int count = 0;
    for (const auto& event : events) {
        if (event.at(0).toInt() == QMediaPlayer::PlayingState)
            ++count;
    }
    return count;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    check(!QImage(":/assets/drone.jpg").isNull(), "embedded drone image");
    MainWindow window;
    auto *timer = window.findChild<QTimer *>("flightTimer");
    check(timer, "animation timer exists");
    timer->stop(); // В тестах продвигаем время явно, без ожидания полного полёта.
    Qt3DExtras::Qt3DWindow *view = nullptr;
    for (QWindow *candidate : QApplication::allWindows()) {
        if (auto *scene = qobject_cast<Qt3DExtras::Qt3DWindow *>(candidate)) {
            view = scene;
            break;
        }
    }
    check(view, "3D window exists");
    QObject *root = view->camera();
    while (root->parent()) {
        root = root->parent();
    }
    auto *drone = root->findChild<Qt3DCore::QTransform *>("droneTransform");
    auto *target = root->findChild<Qt3DCore::QTransform *>("targetTransform");
    check(drone && target, "drone and target transforms exist");
    check(root->findChildren<Qt3DExtras::QSphereMesh *>().isEmpty(), "reference sphere removed");
    auto click = [&window](const char *name) {
        auto *button = window.findChild<QPushButton *>(name);
        check(button, name);
        button->click();
    };
    auto input = [&window](const char *name, double value) {
        auto *field = window.findChild<QDoubleSpinBox *>(name);
        check(field, name);
        field->setValue(value);
    };
    check(drone->translation() == QVector3D(-300, 15, 200), "initial ground position");
    input("xInput", 100);
    input("yInput", 120);
    input("altitudeInput", 80);
    check(target->translation() == QVector3D(-200, 80, 80), "target previews input");
    click("moveButton");
    check(drone->translation() == QVector3D(-300, 15, 200), "refused movement preserves position");
    click("takeoffButton");
    check(drone->translation() == QVector3D(-300, 15, 200), "takeoff starts at current position");
    window.advance_simulation(1.0);
    check(drone->translation() == QVector3D(-300, 40, 200), "takeoff animated midpoint");
    window.advance_simulation(1.0);
    check(drone->translation() == QVector3D(-300, 65, 200), "takeoff reaches 50 metres");
    check(window.findChild<QDoubleSpinBox *>("altitudeInput")->value() == 50.0,
          "takeoff sets the height field to 50");
    input("altitudeInput", 80);
    click("moveButton");
    check(drone->translation() == QVector3D(-300, 65, 200), "move does not teleport");
    window.advance_simulation(0.5);
    check(drone->translation().x() > -300 && drone->translation().x() < -200,
          "movement has intermediate rendered positions");
    window.advance_simulation(100.0);
    check(drone->translation() == QVector3D(-200, 95, 80), "movement maps X Y and altitude");
    click("landButton");
    window.advance_simulation(100.0);
    check(drone->translation() == QVector3D(-200, 15, 80), "landing updates scene");
    click("takeoffButton");
    window.advance_simulation(2.0);
    click("moveButton");
    window.advance_simulation(100.0);
    click("emergencyButton");
    window.advance_simulation(1.0);
    check(drone->translation().y() > 15.0 && drone->translation().y() < 65.0,
          "emergency landing descends smoothly");
    window.advance_simulation(100.0);
    check(drone->translation() == QVector3D(-200, 15, 80), "emergency landing updates scene");
    input("xInput", -1000);
    input("altitudeInput", 200);
    click("telemetryButton");
    check(drone->translation() == QVector3D(-1300, 215, 80), "telemetry outside ground grid");
    click("focusDroneButton");
    check(view->camera()->viewCenter() == QVector3D(-1300, 200, 80), "focus current coordinates");
    const float distance = view->camera()->viewVector().length();
    click("zoomInButton");
    check(view->camera()->viewVector().length() < distance, "zoom in");
    click("zoomOutButton");
    check(std::abs(view->camera()->viewVector().length() - distance) < 0.1f, "zoom out");
    click("resetCameraButton");
    check(view->camera()->position() == QVector3D(700, 700, 900), "restore overview");

    auto *bar = window.findChild<QProgressBar *>("batteryBar");
    auto *error_sound = window.findChild<QMediaPlayer *>("errorSoundPlayer");
    auto *low_sound = window.findChild<QMediaPlayer *>("lowBatterySoundPlayer");
    auto *battery_input = window.findChild<QDoubleSpinBox *>("batteryInput");
    auto *editor = battery_input->findChild<QLineEdit *>();
    auto *message = window.findChild<QLabel *>("messageLabel");
    check(bar && error_sound && low_sound && editor && message, "battery and audio controls");
    check(QFile::exists(":/assets/critical.mp3") && QFile::exists(":/assets/lowbattery.mp3"),
          "embedded warning sounds");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QSignalSpy low_events(low_sound, &QMediaPlayer::playbackStateChanged);
#else
    QSignalSpy low_events(low_sound, &QMediaPlayer::stateChanged);
#endif
    input("batteryInput", 50);
    click("batteryButton");
    check(bar->styleSheet().contains("#2e7d32"), "50 percent is green");
    input("batteryInput", 49.99);
    click("batteryButton");
    check(bar->styleSheet().contains("#f9a825"), "below 50 percent is amber");
    input("batteryInput", 20);
    click("batteryButton");
    check(bar->styleSheet().contains("#f9a825") && play_count(low_events) == 0,
          "20 percent is amber with no critical sound");
    click("takeoffButton"); // 20 -> 19 после расхода заряда на взлёт.
    check(bar->styleSheet().contains("#c62828") && bar->format().contains("19.00"),
          "flight consumption updates battery color");
    check(QTest::qWaitFor([low_sound] { return is_playing(low_sound); }, 5000),
          "crossing below 20 starts lowbattery sound");
    window.advance_simulation(100.0);
    click("moveButton");
    window.advance_simulation(100.0);
    click("landButton");
    window.advance_simulation(100.0);
    check(play_count(low_events) == 1, "low battery does not repeat on every update");
    input("batteryInput", 0);
    click("batteryButton");
    check(bar->value() == 0 && bar->styleSheet().contains("border: 2px solid #c62828"),
          "empty battery retains red outline");
    click("takeoffButton");
    check(QTest::qWaitFor([error_sound] { return is_playing(error_sound); }, 5000),
          "rejected takeoff plays error sound");
    check(!is_playing(low_sound), "error warning takes priority over low battery warning");
    input("batteryInput", 70);
    click("batteryButton");
    check(!is_playing(low_sound) && !is_playing(error_sound), "recovery stops warnings");
    input("batteryInput", 19.99);
    click("telemetryButton");
    check(QTest::qWaitFor([low_sound] { return is_playing(low_sound); }, 5000)
          && play_count(low_events) == 2, "new low battery episode replays warning");
    input("batteryInput", 70);
    click("batteryButton");

    battery_input->selectAll();
    QTest::keyClicks(editor, "x");
    check(QTest::qWaitFor([error_sound] { return is_playing(error_sound); }, 5000),
          "invalid character plays error sound");
    click("batteryButton");
    check(bar->format().contains("70.00"), "rejected input does not change drone battery");
    battery_input->selectAll();
    QTest::keyClicks(editor, "101");
    click("batteryButton");
    check(bar->format().contains("70.00") && message->text().contains("Ошибка"),
          "out-of-range input is not applied as a partial number");
    battery_input->selectAll();
    QTest::keyClicks(editor, "60");
    click("batteryButton");
    check(bar->format().contains("60.00") && !is_playing(error_sound), "valid correction is accepted");
    battery_input->selectAll();
    QTest::keyClick(editor, Qt::Key_Backspace);
    QFocusEvent focus_out(QEvent::FocusOut);
    QApplication::sendEvent(battery_input, &focus_out);
    click("batteryButton");
    check(bar->format().contains("60.00") && message->text().contains("Ошибка"),
          "incomplete input remains rejected after Qt restores previous text");
    battery_input->selectAll();
    QTest::keyClicks(editor, "80");
    click("batteryButton");
    check(bar->format().contains("80.00"), "correction clears incomplete-input error");
    check(error_sound->error() == QMediaPlayer::NoError && low_sound->error() == QMediaPlayer::NoError,
          "both warning sounds decode without errors");

    auto *graph = dynamic_cast<BatteryGraph *>(window.findChild<QWidget *>("batteryGraph"));
    check(graph && !graph->history().isEmpty() && graph->history().last().y() == 80.0,
          "battery graph follows charge changes");
    const double last_time = graph->history().last().x();
    window.advance_simulation(2.0);
    check(graph->history().last().x() > last_time && graph->history().last().y() == 80.0,
          "graph records time even while drone is stationary");
    BatteryGraph bounded_graph;
    for (int i = 0; i <= 500; ++i)
        bounded_graph.add_sample(i, 100.0 - i % 100);
    check(bounded_graph.history().size() <= 122, "graph retains only recent two minutes");
    bounded_graph.add_sample(-1.0, 40.0);
    bounded_graph.add_sample(501.0, 101.0);
    check(bounded_graph.history().last().x() == 500.0, "graph rejects invalid samples");

    click("emergencyButton");
    window.advance_simulation(100.0);
    click("takeoffButton");
    timer->start(16);
    check(QTest::qWaitFor([drone] { return drone->translation().y() > 15.0; }, 1000),
          "real timer advances the drone");
    timer->stop();
    window.advance_simulation(100.0);

    // Необязательная проверка настоящего рендера на рабочем столе.
    const QString screenshot = qEnvironmentVariable("UAV_SCREENSHOT");
    if (!screenshot.isEmpty()) {
        input("xInput", 100);
        input("yInput", 100);
        input("altitudeInput", 120);
        click("telemetryButton");
        window.show();
        QTest::qWait(1000);
        Qt3DRender::QRenderCapture *capture = nullptr;
        const auto nodes = view->activeFrameGraph()->findChildren<Qt3DRender::QFrameGraphNode *>();
        for (auto *node : nodes) {
            if (!qobject_cast<Qt3DRender::QNoDraw *>(node)
                && node->findChildren<Qt3DRender::QFrameGraphNode *>(
                    QString(), Qt::FindDirectChildrenOnly).isEmpty()) {
                capture = new Qt3DRender::QRenderCapture(node);
                break;
            }
        }
        check(capture, "render capture branch");
        auto *reply = capture->requestCapture();
        check(QTest::qWaitFor([reply] { return reply->isComplete(); }, 10000), "capture 3D framebuffer");
        check(reply->image().save(screenshot), "save rendered scene");
        check(graph->grab().save(screenshot + ".battery.png"), "save battery graph");
        delete reply;
        delete capture;
        window.close();
    }
    std::cout << "3D window checks passed\n";
    return 0;
}
