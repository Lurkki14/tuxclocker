#ifndef EDITPROFILE_H
#define EDITPROFILE_H

#include <QDialog>
#include <QVector>
#include "qcustomplot.h"
#include "gputypes.h"
#include <QString>

namespace Ui {
class editProfile;
}

class editProfile : public QDialog
{
    Q_OBJECT

public:
    explicit editProfile(QWidget *parent = nullptr, int GPUIndex = 0, gputypes *types = nullptr);
    ~editProfile();
    QVector<double> qv_x, qv_y;
    QVector<int> cpoints_x, cpoints_y;

signals:
    void on_clickedPoint(QMouseEvent *event);
    void on_dragPoint(bool);
    void on_clickedGraph(bool);

private slots:
    void clickedGraph(QMouseEvent *event);
    void rePlot();
    void addPoint(double x, double y);
    void clickedPoint(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void on_saveButton_clicked();
    bool initializeDragging(QMouseEvent *event);
    bool detectMove(QMouseEvent *event);
    bool detectPress(QMouseEvent *event);
    void detectRelease(QMouseEvent *event);
    bool checkForDuplicatePoint(int x, int y);
    int getDataIndex(QCPAbstractPlottable *plottable, int dataIndex);
    void getClosestCoords(QMouseEvent *event);
    void getPointIndices();
    bool checkForNearbyPoints(QMouseEvent *event);
    void dragPoint(int index_x, int index_y, QMouseEvent *event);
    void drawFillerLines();
    void on_clearButton_clicked();
    void drawPointHoveredText(QMouseEvent *event);

    void on_cancelButton_pressed();

private:
    Ui::editProfile *ui;
    //QVector<double> qv_x, qv_y;
    QVector<double> leftLineX;
    QVector<double> leftLineY;
    QVector<double> rightLineX;
    QVector<double> rightLineY;
    QVector<int> curvepoints;
    QPair<int, int> curvepoint;
    QCPItemText *coordText;
    int x_lower = 0;
    int x_upper = 100;
    int y_lower = 0;
    int y_upper = 100;
    double pixelLength;
    double selectionPrecision = 2;
    bool mouseMoving = false;
    bool mousePressed = false;
    bool mouseDragging = false;
    bool duplicatePoint = false;
    bool isNearbyPoint = false;
    bool coordTextCreated = false;
    int pointIndex;
    int y_length;
    int x_length;
    int xcoord;
    int ycoord;
    int index_x = 0;
    int index_y = 0;
    int GPUId = 0;
    gputypes *type;
    bool indicesSet = false;
    bool draggingPoint = false;
    QTimer *pressTimer = new QTimer(this);
    int timerTime = 1000;
};

#endif // EDITPROFILE_H

