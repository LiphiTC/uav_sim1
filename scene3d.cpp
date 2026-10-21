#include "scene3d.h"
#include "dronevisual.h"

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QDirectionalLight>
#include <QVBoxLayout>
#include <algorithm>

QVector3D Scene3D::toScene(const Point3D &p)
{
    return {float(p.x - 300), float(p.z), float(200 - p.y)};
}

Scene3D::Scene3D(QWidget *parent)
    : QWidget(parent), m_view(new Qt3DExtras::Qt3DWindow),
      m_container(QWidget::createWindowContainer(m_view, this)),
      m_root(new Qt3DCore::QEntity)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_container);
    m_container->setMinimumSize(480, 360);
    m_container->setFocusPolicy(Qt::StrongFocus);
    m_view->defaultFrameGraph()->setClearColor(QColor(23, 32, 46));

    addBox({300, 200, -3}, {600, 400, 6}, QColor(48, 62, 75), m_root);
    for (int x = 0; x <= 600; x += 50)
        addLine({double(x), 0, 0.5}, {double(x), 400, 0.5}, QColor(80, 95, 110), m_root, 0.7f);
    for (int y = 0; y <= 400; y += 50)
        addLine({0, double(y), 0.5}, {600, double(y), 0.5}, QColor(80, 95, 110), m_root, 0.7f);
    addLine({0, 0, 2}, {600, 0, 2}, QColor(235, 90, 90), m_root, 2);
    addLine({0, 0, 2}, {0, 400, 2}, QColor(90, 220, 145), m_root, 2);
    addLine({0, 0, 2}, {0, 0, 600}, QColor(80, 160, 255), m_root, 2);

    m_droneVisual = new DroneVisual(m_view->camera(), m_root);
    addBox({0, 0, 3}, {12, 12, 6}, QColor(70, 220, 120), m_root);
    auto *target = addBox({10, 20, 50}, {10, 10, 10}, QColor(235, 65, 65), m_root);
    m_targetTransform = target->componentsOfType<Qt3DCore::QTransform>().first();
    m_targetTransform->setObjectName("targetTransform");

    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(m_root);
    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(lightEntity);
    light->setWorldDirection({-0.5f, -1.0f, -0.4f});
    light->setIntensity(1.1f);
    lightEntity->addComponent(light);

    Qt3DExtras::QOrbitCameraController *controller = new Qt3DExtras::QOrbitCameraController(m_root);
    controller->setCamera(m_view->camera());
    controller->setLinearSpeed(400);
    controller->setLookSpeed(120);
    connect(m_view, &QWindow::widthChanged, this, [this] { updateAspectRatio(); });
    connect(m_view, &QWindow::heightChanged, this, [this] { updateAspectRatio(); });
    resetCamera();
    m_view->setRootEntity(m_root);
}

Scene3D::~Scene3D()
{
    m_view->setRootEntity(nullptr);
    delete m_container;
    delete m_root;
}

Qt3DCore::QEntity *Scene3D::addBox(const Point3D &center, const Point3D &size,
                                 const QColor &color, Qt3DCore::QEntity *parent)
{
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(parent);
    Qt3DExtras::QCuboidMesh *mesh = new Qt3DExtras::QCuboidMesh(entity);
    mesh->setXExtent(size.x);
    mesh->setYExtent(size.z);
    mesh->setZExtent(size.y);
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
    transform->setTranslation(toScene(center));
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
    material->setDiffuse(color);
    material->setAmbient(color.darker(180));
    entity->addComponent(mesh);
    entity->addComponent(transform);
    entity->addComponent(material);
    return entity;
}

void Scene3D::addLine(const Point3D &from, const Point3D &to, const QColor &color,
                      Qt3DCore::QEntity *parent, float width)
{
    const QVector3D direction = toScene(to) - toScene(from);
    if (direction.lengthSquared() < 0.0001f)
        return;
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(parent);
    Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh(entity);
    mesh->setRadius(width);
    mesh->setLength(direction.length());
    mesh->setRings(2);
    mesh->setSlices(8);
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
    const Point3D center{(from.x + to.x) / 2, (from.y + to.y) / 2, (from.z + to.z) / 2};
    transform->setTranslation(toScene(center));
    transform->setRotation(QQuaternion::rotationTo({0, 1, 0}, direction.normalized()));
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
    material->setDiffuse(color);
    material->setAmbient(color.darker(130));
    entity->addComponent(mesh);
    entity->addComponent(transform);
    entity->addComponent(material);
}

void Scene3D::updateDrone(const Drone &drone)
{
    m_dronePosition = toScene({drone.get_x(), drone.get_y(), drone.get_altitude()});
    // Картинка поднята над точкой координат, чтобы быть видимой на земле.
    m_droneVisual->setPosition(m_dronePosition + QVector3D(0, 15, 0));
}

void Scene3D::setTarget(double x, double y, double altitude)
{
    m_targetTransform->setTranslation(toScene({x, y, altitude}));
}

void Scene3D::focusDrone()
{
    auto *camera = m_view->camera();
    camera->setViewCenter(m_dronePosition);
    camera->setPosition(m_dronePosition + QVector3D(160, 140, 200));
    camera->setUpVector({0, 1, 0});
}

void Scene3D::updateAspectRatio()
{
    m_view->camera()->lens()->setPerspectiveProjection(
        45, float(std::max(1, m_view->width())) / std::max(1, m_view->height()), 1, 6000);
}

void Scene3D::resetCamera()
{
    m_view->camera()->setViewCenter({0, 180, 0});
    m_view->camera()->setPosition({700, 700, 900});
    m_view->camera()->setUpVector({0, 1, 0});
    updateAspectRatio();
}

void Scene3D::zoom(float factor)
{
    Qt3DRender::QCamera *camera = m_view->camera();
    const QVector3D offset = camera->position() - camera->viewCenter();
    const float distance = std::clamp(offset.length() * factor, 150.0f, 3000.0f);
    camera->setPosition(camera->viewCenter() + offset.normalized() * distance);
}
