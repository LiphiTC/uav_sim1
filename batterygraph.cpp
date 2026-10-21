#include "batterygraph.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BatteryGraph::BatteryGraph(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(260, 180);
    setObjectName("batteryGraph");
    setAccessibleName("График заряда аккумулятора");
}

void BatteryGraph::add_sample(double seconds, double battery)
{
    if (!std::isfinite(seconds) || !std::isfinite(battery) || seconds < current_time
        || battery < 0.0 || battery > 100.0)
        return;
    current_time = seconds;
    // На постоянном участке достаточно одной точки в секунду.
    if (samples.isEmpty() || samples.last().y() != battery
        || seconds - samples.last().x() >= 1.0) {
        samples.append(QPointF(seconds, battery));
        // Сохраняем одну точку перед границей, чтобы линия не обрывалась.
        while (samples.size() > 2 && samples.at(1).x() < seconds - 120.0)
            samples.removeFirst();
        update();
    }
}

const QVector<QPointF>& BatteryGraph::history() const
{
    return samples;
}

void BatteryGraph::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());
    const QColor text_color = palette().text().color();
    painter.setPen(text_color);
    painter.drawText(QRect(0, 4, width(), 22), Qt::AlignCenter, "Заряд аккумулятора, %");

    const QRectF plot(40, 32, width() - 70, height() - 66);
    const double end = std::max(10.0, current_time);
    const double start = std::max(0.0, end - 120.0);
    auto map_point = [plot, start, end](const QPointF& sample) {
        return QPointF(plot.left() + (sample.x() - start) / (end - start) * plot.width(),
                       plot.bottom() - sample.y() / 100.0 * plot.height());
    };
    for (int value = 0; value <= 100; value += 25) {
        const double y = map_point(QPointF(start, value)).y();
        painter.setPen(palette().mid().color());
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(text_color);
        painter.drawText(QRectF(0, y - 9, 34, 18), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value));
    }
    for (int i = 0; i <= 4; ++i) {
        const double time = start + (end - start) * i / 4.0;
        const double x = map_point(QPointF(time, 0)).x();
        painter.drawText(QRectF(x - 22, plot.bottom() + 2, 44, 18), Qt::AlignCenter,
                         QString::number(time, 'f', 0));
    }
    painter.drawText(QRectF(0, height() - 18, width(), 18), Qt::AlignCenter, "Время, с");
    if (samples.isEmpty())
        return;

    QPainterPath path(map_point(samples.first()));
    for (int i = 1; i < samples.size(); ++i) {
        // Заряд изменяется при команде, между командами остаётся постоянным.
        path.lineTo(map_point(QPointF(samples.at(i).x(), samples.at(i - 1).y())));
        path.lineTo(map_point(samples.at(i)));
    }
    path.lineTo(map_point(QPointF(current_time, samples.last().y())));
    const double battery = samples.last().y();
    const QColor color(battery >= 50.0 ? "#2e7d32" : battery >= 20.0 ? "#f9a825" : "#c62828");
    painter.save();
    painter.setClipRect(plot.adjusted(-2, -2, 2, 2));
    painter.setPen(QPen(color, 2.5));
    painter.drawPath(path);
    painter.setBrush(color);
    painter.drawEllipse(map_point(QPointF(current_time, battery)), 3, 3);
    painter.restore();
}
