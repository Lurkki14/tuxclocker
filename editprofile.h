#ifndef EDITPROFILE_H
#define EDITPROFILE_H

#include <QDialog>
#include <QVector>
#include "qcustomplot.h"
#include <QFile>
#include <QDomNode>
#include <QString>
#include <QTextStream>
#include <QPainter>
#include <QLine>

namespace Ui {
class editProfile;
}

class editProfile : public QDialog
{
    Q_OBJECT

public:
    explicit editProfile(QWidget *parent = nullptr);
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
    void on_pushButton_clicked();
    void clickedPoint(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    //void getDataPoints();
    void on_saveButton_clicked();
    void drawCoordtext();

    double getPixelLength(QMouseEvent *event);
    bool initializeDragging(QMouseEvent *event);
    bool detectMove(QMouseEvent *event);
    bool detectPress(QMouseEvent *event);
    bool detectRelease(QMouseEvent *event);
    bool checkForDuplicatePoint(int x, int y);
    int getDataIndex(QCPAbstractPlottable *plottable, int dataIndex);
    int getYcoordValue(QMouseEvent *event);
    int getXcoordValue(int xcoord);
    int getXPointIndex(int xcoord, int ycoord);
    int getYPointIndex(int index_y);
    bool resetMouseMove();
    bool resetMouseDragging();
    bool checkForNearbyPoints(QMouseEvent *event);
    bool draggedIndicesSet();
    bool draggedIndicesUnset();
    void dragPoint(int index_x, int index_y, QMouseEvent *event);
    bool draggingPointSet();
    bool draggingPointUnset();
    void drawFillerLines();
    void on_clearButton_clicked();

private:
    Ui::editProfile *ui;
    QDomDocument document;
    //QVector<double> qv_x, qv_y;
    QVector<double> leftLineX;
    QVector<double> leftLineY;
    QVector<double> rightLineX;
    QVector<double> rightLineY;
    QVector<int> curvepoints;
    QPair<int, int> curvepoint;
    //QCPItemText *coordText;
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
    bool indicesSet = false;
    bool draggingPoint = false;
};

#endif // EDITPROFILE_H

