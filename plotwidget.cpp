#include "plotwidget.h"

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{

}
void PlotWidget::leaveEvent(QEvent *event)
{
    emit leftPlot();
}
