#ifndef SCENE3D_H
#define SCENE3D_H

#include "drone.h"
#include "point3d.h"
#include <QWidget>
#include <QVector3D>

namespace Qt3DCore { class QEntity; class QTransform; }
namespace Qt3DExtras { class Qt3DWindow; }
class DroneVisual;

// Геометрия и камера перенесены из Practical_03_Classes/ew_route_3d.
class Scene3D : public QWidget
{
public:
    explicit Scene3D(QWidget *parent = nullptr);
    ~Scene3D() override;
    void updateDrone(const Drone &drone);
    void setTarget(double x, double y, double altitude);
    void zoom(float factor);
    void resetCamera();
    void focusDrone();

private:
    static QVector3D toScene(const Point3D &position);
    Qt3DCore::QEntity *addBox(const Point3D &center, const Point3D &size,
                            const QColor &color, Qt3DCore::QEntity *parent);
    void addLine(const Point3D &from, const Point3D &to, const QColor &color,
                 Qt3DCore::QEntity *parent, float width = 1.5f);
    void addZone(const Point3D &center, double radius, Qt3DCore::QEntity *parent);
    void updateAspectRatio();

    Qt3DExtras::Qt3DWindow *m_view;
    QWidget *m_container;
    Qt3DCore::QEntity *m_root;
    DroneVisual *m_droneVisual;
    Qt3DCore::QTransform *m_targetTransform;
    QVector3D m_dronePosition;
};

#endif // SCENE3D_H
