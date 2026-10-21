#include "mainwindow.h"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QImage>
#include <QPushButton>
#include <QTest>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    check(!QImage(":/assets/drone.jpg").isNull(), "embedded drone image");
    MainWindow window;
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
    check(drone->translation() == QVector3D(-300, 16, 200), "takeoff updates altitude");
    click("moveButton");
    check(drone->translation() == QVector3D(-200, 95, 80), "movement maps X Y and altitude");
    click("landButton");
    check(drone->translation() == QVector3D(-200, 15, 80), "landing updates scene");
    click("takeoffButton");
    click("moveButton");
    click("emergencyButton");
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
        delete reply;
        delete capture;
        window.close();
    }
    std::cout << "3D window checks passed\n";
    return 0;
}
