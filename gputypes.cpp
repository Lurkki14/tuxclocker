#include "gputypes.h"

gputypes::gputypes()
{}
gputypes::~gputypes()
{}

int gputypes::generateFanPoint(int temp, QVector <int> xPoints, QVector <int> yPoints) {
    // Calculate the value for fan speed based on temperature
    // First check if the fan speed should be y[0] or y[final]
    qDebug("temp: %d", temp);
    int index = 0;
    if (temp <= xPoints[0]) {
        qDebug("ret first point");
        return yPoints[0];
    } else if (temp >= xPoints.last()) {
        qDebug("ret last point %d", xPoints.last());
        return yPoints.last();
    } else {
        // Get the index of the leftmost point of the interpolated interval by comparing it to temperature
        for (int i=0; i<xPoints.size(); i++) {
            if (temp >= xPoints[i] && temp <= xPoints[i+1]) {
                index = i;
                break;
            }
        }
        // Check if the change in x is zero to avoid dividing by it
        if (xPoints[index] - xPoints[index + 1] == 0) {
            return yPoints[index + 1];
        } else return (((yPoints[index + 1] - yPoints[index]) * (temp  - xPoints[index])) / (xPoints[index + 1] - xPoints[index])) + yPoints[index];
    }
}
