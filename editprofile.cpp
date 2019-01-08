#include "editprofile.h"
#include "ui_editprofile.h"
#include "mainwindow.h"

editProfile::editProfile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::editProfile)
{
    ui->setupUi(this);

    // Get the main widget backgorund palette and use the colors for the plots
    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Active);
    QColor color = palette.color(QPalette::Background);
    QColor textColor = palette.color(QPalette::Text);
    QColor graphColor = palette.color(QPalette::Highlight);
    QPen graphPen;
    graphPen.setWidth(2);
    graphPen.setColor(graphColor);
    QPen tickPen;
    tickPen.setWidthF(0.5);
    tickPen.setColor(textColor);

    // Define the filler line vectors and graphs so they don't need to be recreated every update
    leftLineX.append(x_lower);
    leftLineX.append(0);
    leftLineY.append(0);
    leftLineY.append(0);

    rightLineX.append(0);
    rightLineX.append(x_upper);
    rightLineY.append(0);
    rightLineY.append(0);

    ui->curvePlot->addGraph();
    ui->curvePlot->addGraph();

    ui->curvePlot->addGraph();
    ui->curvePlot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
    ui->curvePlot->graph(0)->setLineStyle(QCPGraph::lsLine);

    ui->curvePlot->setBackground(color);
    ui->curvePlot->xAxis->setLabelColor(textColor);
    ui->curvePlot->yAxis->setLabelColor(textColor);
    ui->curvePlot->xAxis->setTickLabelColor(textColor);
    ui->curvePlot->yAxis->setTickLabelColor(textColor);
    ui->curvePlot->graph(0)->setPen(graphPen);
    ui->curvePlot->graph(1)->setPen(graphPen);
    ui->curvePlot->graph(2)->setPen(graphPen);
    ui->curvePlot->xAxis->setTickPen(tickPen);
    ui->curvePlot->yAxis->setTickPen(tickPen);
    ui->curvePlot->xAxis->setSubTickPen(tickPen);
    ui->curvePlot->yAxis->setSubTickPen(tickPen);
    ui->curvePlot->xAxis->setBasePen(tickPen);
    ui->curvePlot->yAxis->setBasePen(tickPen);

    ui->curvePlot->xAxis->setLabel("Temperature");
    ui->curvePlot->yAxis->setLabel("Fan speed (%)");

    ui->curvePlot->xAxis->setRange(x_lower, (x_upper+5));
    ui->curvePlot->yAxis->setRange(y_lower, (y_upper+5));

    //connect(ui->curvePlot, SIGNAL(on_clickedGraph(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent*)), SLOT(getDataIndex(QCPAbstractPlottable *plottable, int dataIndex)));
    connect(ui->curvePlot, SIGNAL(mouseDoubleClick(QMouseEvent*)), SLOT(clickedGraph(QMouseEvent*)));
    //connect(ui->curvePlot, SIGNAL(mouseMove(QMouseEvent*)), SLOT(getPixelLength(QMouseEvent*)));
    connect(ui->curvePlot, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), SLOT(clickedPoint(QCPAbstractPlottable*,int,QMouseEvent*)));
    connect(ui->curvePlot, SIGNAL(mousePress(QMouseEvent*)), SLOT(detectPress(QMouseEvent*)));
    connect(ui->curvePlot, SIGNAL(mouseMove(QMouseEvent*)), SLOT(detectMove(QMouseEvent*)));
    connect(ui->curvePlot, SIGNAL(mouseRelease(QMouseEvent*)), SLOT(detectRelease(QMouseEvent*)));
    connect(ui->curvePlot, SIGNAL(mouseMove(QMouseEvent*)), SLOT(getYcoordValue(QMouseEvent*)));

    // Load the existing points to the graph
    MainWindow mw;
    for (int i=0; i<mw.xCurvePoints.length(); i++) {
        qv_x.append(mw.xCurvePoints[i]);
        qv_y.append(mw.yCurvePoints[i]);
    }
    ui->curvePlot->graph(0)->setData(qv_x, qv_y);
    drawFillerLines();
}

editProfile::~editProfile()
{
    delete ui;
}

void editProfile::addPoint(double x, double y)
{
    y = round(y);
    x = round(x);
    if (qv_x.length() != 0) {
    checkForDuplicatePoint(x, y);
    }
    if ((x_lower<=x) && (x<=x_upper) && (y_lower<=y) && (y<=y_upper) && !duplicatePoint) {
        qv_x.append(x);
        qv_y.append(y);
    }
}

void editProfile::rePlot()
{
    ui->curvePlot->replot();
    ui->curvePlot->update();
}
void editProfile::clickedGraph(QMouseEvent *event)
{
    QPoint point = event->pos();
    addPoint(ui->curvePlot->xAxis->pixelToCoord(point.x()), ui->curvePlot->yAxis->pixelToCoord(point.y()));
    ui->curvePlot->graph(0)->setData(qv_x, qv_y);
    drawFillerLines();
}

void editProfile::clickedPoint(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event)
{
    checkForNearbyPoints(event);
    ycoord = round(ycoord);
    xcoord = round(xcoord);
    if (isNearbyPoint && qv_x.length() != 0) {
            for (int i=0; i<qv_y.length(); i++ ) {
                qv_y[i] = round(qv_y[i]);
                qv_x[i] = round(qv_x[i]);
                if ((qv_x[i] == xcoord) && (qv_y[i] == ycoord)) {
                    qv_x.remove(i);
                    qv_y.remove(i);
                    break;
                }
            }
      ui->curvePlot->graph(0)->setData(qv_x, qv_y);
      rePlot();
      drawFillerLines();
    }

}

bool editProfile::checkForNearbyPoints(QMouseEvent *event)
{
    if (qv_x.length() != 0) {
    QPoint point = event->pos();
    double pointerxcoord = ui->curvePlot->xAxis->pixelToCoord(point.x());
    double pointerycoord = ui->curvePlot->yAxis->pixelToCoord(point.y());
    for (int i=0; i<qv_x.length(); i++) {
        if ((abs(pointerxcoord - qv_x[i]) < selectionPrecision) && abs(pointerycoord - qv_y[i]) < selectionPrecision) {
            isNearbyPoint = true;
            break;
        }
        else {
            isNearbyPoint = false;
        }
    }
    return isNearbyPoint;
    }
}

int editProfile::getDataIndex(QCPAbstractPlottable *plottable, int dataIndex)
{
    dataIndex = pointIndex;
    qDebug() << pointIndex;
    return pointIndex;
}

bool editProfile::checkForDuplicatePoint(int x, int y)
// Return true if there is a duplicate point
{
    for (int i=0; i<qv_x.length(); i++) {
        qv_x[i] = round(qv_x[i]);
        qv_y[i] = round(qv_y[i]);
        if ((x == qv_x[i]) && (y == qv_y[i])) {
            duplicatePoint = true;
        }
    }
    return duplicatePoint;
}

int editProfile::getXcoordValue(int xcoord)
{
    getXPointIndex(xcoord, ycoord);
    return xcoord;
}

int editProfile::getYcoordValue(QMouseEvent *event)
{
    if (qv_x.length() != 0) {
    //Gets the indices of points that match the current cursor location
    QPoint point = event->pos();
    double pointerycoord = ui->curvePlot->yAxis->pixelToCoord(point.y());
    double pointerxcoord = ui->curvePlot->xAxis->pixelToCoord(point.x());
    for (int i=0; i<qv_y.length(); i++) {
        if ((abs(pointerxcoord - qv_x[i]) < selectionPrecision) && abs(pointerycoord - qv_y[i]) < selectionPrecision) {
            xcoord = qv_x[i];
            ycoord = qv_y[i];
            break;
        }
    }
    }
    getXcoordValue(xcoord);
    return ycoord;
}

bool editProfile::detectMove(QMouseEvent *event)
{
    mouseMoving = true;
    if (mouseMoving && mousePressed) {
        initializeDragging(event);
    }
    return mouseMoving;
}

bool editProfile::detectPress(QMouseEvent *event)
{
    mousePressed = true;
    if (mouseMoving && mousePressed) {
        initializeDragging(event);
    }
    return mousePressed;
}

bool editProfile::initializeDragging(QMouseEvent *event)
{
    // Decides whether to start dragging the point
    mouseDragging = true;
    checkForNearbyPoints(event);
    if (isNearbyPoint || draggingPoint) {
        dragPoint(index_x, index_y, event);
        draggingPointSet();
    }
    return mouseDragging;
}

int editProfile::getXPointIndex(int xcoord, int ycoord)
{
    // Gets the indices of the point that the mouse has hovered over
    if (qv_x.length() > 0) {
        for (int i=0; i<qv_x.length(); i++) {
            if ((qv_x[i] == xcoord) && (qv_y[i] == ycoord)) {
                index_x = i;
                index_y = i;
                break;
            }
        }
        getYPointIndex(index_y);
        return index_x;
    }
}

int editProfile::getYPointIndex(int index_y)
{
    qDebug() << index_x << index_y;
    return index_y;
}

void editProfile::dragPoint(int index_x, int index_y, QMouseEvent* event)
{
    // Moves the selected point by index
    // Sleep here so we don't take up so much CPU time
    QThread::msleep(10);
    ui->curvePlot->clearItems();
    drawCoordtext();
    QPoint point = event->pos();
    qv_y[index_y] = round(ui->curvePlot->yAxis->pixelToCoord(point.y()));
    qv_x[index_x] = round(ui->curvePlot->xAxis->pixelToCoord(point.x()));

    if (qv_x[index_x] > x_upper) {
        qv_x[index_x] = x_upper;
    }
    if (qv_x[index_x] < x_lower) {
        qv_x[index_x] = x_lower;
    }
    if (qv_y[index_y] > y_upper) {
        qv_y[index_y] = y_upper;
    }
    if (qv_y[index_y] < y_lower) {
        qv_y[index_y] = y_lower;
    }

    ui->curvePlot->graph(0)->setData(qv_x, qv_y);
    drawFillerLines();
}

void editProfile::drawCoordtext()
{
    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Active);
    QColor textColor = palette.color(QPalette::Text);

    QCPItemText *coordText = new QCPItemText(ui->curvePlot);
    if (draggingPoint) {
    if (qv_x[index_x] > x_upper) {
        qv_x[index_x] = x_upper;
    }
    if (qv_x[index_x] < x_lower) {
        qv_x[index_x] = x_lower;
    }
    if (qv_y[index_y] > y_upper) {
        qv_y[index_y] = y_upper;
    }
    if (qv_y[index_y] < y_lower) {
        qv_y[index_y] = y_lower;
    }
    coordText->position->setType(QCPItemPosition::ptPlotCoords);
    coordText->position->setCoords(qv_x[index_x], qv_y[index_y] + 4);
    QString xString = QString::number(qv_x[index_x]);
    QString yString = QString::number(qv_y[index_y]);
    coordText->setText(xString + ", " + yString);
    coordText->setColor(textColor);

    } else {
        ui->curvePlot->removeItem(coordText);
    }
}

void editProfile::drawFillerLines()
{
    // Draw the filler lines separately so they don't interfere with the main data. graph(1) = leftward line, graph(2) = rightward line

    leftLineX[1] = ui->curvePlot->graph(0)->dataSortKey(0);

    leftLineY[0] = ui->curvePlot->graph(0)->dataMainValue(0);
    leftLineY[1] = ui->curvePlot->graph(0)->dataMainValue(0);

    ui->curvePlot->graph(1)->setData(leftLineX, leftLineY);

    rightLineX[0] = ui->curvePlot->graph(0)->dataSortKey(qv_x.length() -1);

    rightLineY[0] = ui->curvePlot->graph(0)->dataMainValue(qv_x.length() -1);
    rightLineY[1] = ui->curvePlot->graph(0)->dataMainValue(qv_x.length() -1);

    ui->curvePlot->graph(2)->setData(rightLineX, rightLineY);
    rePlot();

}
bool editProfile::detectRelease(QMouseEvent *event)
{
    mousePressed = false;
    resetMouseMove();
    resetMouseDragging();
    draggedIndicesUnset();
    draggingPointUnset();
    drawCoordtext();
    return mousePressed;
}

bool editProfile::draggingPointUnset()
{
    draggingPoint = false;
    return draggingPoint;
}

bool editProfile::draggingPointSet()
{
    draggingPoint = true;
    return draggingPoint;
}
bool editProfile::draggedIndicesSet()
{
    indicesSet = true;
    return indicesSet;
}

bool editProfile::draggedIndicesUnset()
{
    indicesSet = false;
    return indicesSet;
}
bool editProfile::resetMouseMove()
{
    mouseMoving = false;
    return mouseMoving;
}

bool editProfile::resetMouseDragging()
{
    mouseDragging = false;
    return mouseDragging;
}

double editProfile::getPixelLength(QMouseEvent *event)
{
    QPoint point = event->pos();
    qDebug() << ui->curvePlot->graph(0)->selectTest(point, 2);
    qDebug() << pixelLength;
    return pixelLength;
}

void editProfile::on_pushButton_clicked()
{
    qDebug() << draggingPoint;
    drawCoordtext();
}

void editProfile::on_saveButton_clicked()
{
    QString xString;
    QString yString;
    for (int i=0; i<qv_x.length(); i++) {
        QString x = QString::number(ui->curvePlot->graph(0)->dataSortKey(i));
        QString y = QString::number(ui->curvePlot->graph(0)->dataMainValue(i));
        xString.append(x + ", ");
        yString.append(y + ", ");

    }
    MainWindow mw;
    QVariant xarray = xString;
    QVariant yarray = yString;
    qDebug() << xarray.toString() << yarray.toString();
    QSettings settings("nvfancurve");
    QString xsetting = mw.currentProfile;
    QString ysetting = mw.currentProfile;
    ysetting.append("/ypoints");
    xsetting.append("/xpoints");
    settings.setValue(xsetting, xarray);
    settings.setValue(ysetting, yarray);

}

void editProfile::on_clearButton_clicked()
{
    MainWindow mw;
    qv_x.clear();
    qv_y.clear();
    qDebug() << mw.currentProfile;
    ui->curvePlot->graph(0)->setData(qv_x, qv_y);
    drawFillerLines();
}
