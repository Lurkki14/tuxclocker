#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

signals:
    void leftPlot();
protected:
    void leaveEvent(QEvent *event);
public slots:
};

#endif // PLOTWIDGET_H
