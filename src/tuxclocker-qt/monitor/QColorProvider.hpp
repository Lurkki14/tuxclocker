#include <QColor>
#include <QVector>

class QColorProvider {
public:
  int current = 0;

  QVector<QColor> colors = {
      QColor(230, 25, 75, 170),    // Red
      QColor(245, 130, 49, 170),   // Orange
      QColor(255, 255, 25, 170),   // Yellow
      QColor(191, 239, 69, 170),   // Lime
      QColor(60, 180, 75, 170),    // Green
      QColor(66, 212, 244, 170),   // Cyan
      QColor(67, 99, 216, 170),    // Navy
      QColor(240, 50, 230, 170),   // Dark Green
      QColor(145, 30, 180, 170),   // Dark Blue
      QColor(250, 190, 190, 170),  // Peach
      QColor(255, 216, 177, 170),  // Tan
      QColor(255, 250, 200, 170),  // Light Blue
      QColor(128, 0, 255, 170),   // Purple
      QColor(255, 128, 128, 170),  // Pink
      QColor(128, 255, 128, 170),  // Light Green
      QColor(128, 128, 255, 170),  // Sky Blue
      QColor(255, 255, 128, 170),  // Light Yellow
      QColor(255, 128, 255, 170),  // Light Pink
      QColor(128, 255, 255, 170),  // Light Turquoise
      QColor(192, 192, 192, 170)   // Gray
  };


  QColorProvider();

  QColor pick();
};