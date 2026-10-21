#ifndef BATTERYGRAPH_H
#define BATTERYGRAPH_H

#include <QPointF>
#include <QVector>
#include <QWidget>

class BatteryGraph : public QWidget
{
public:
    explicit BatteryGraph(QWidget *parent = nullptr);
    void add_sample(double seconds, double battery);
    const QVector<QPointF>& history() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPointF> samples;
    double current_time = 0.0;
};

#endif // BATTERYGRAPH_H
