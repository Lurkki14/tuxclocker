#pragma once

#include "DragChartView.h"

#include <QGridLayout>
#include <QPushButton>

class GraphEditor : public QWidget {
public:
    GraphEditor(QWidget *parent = nullptr);
    DragChartView *dragChartView() {return m_dragChartView;}
private:
    Q_OBJECT
    
    QGridLayout *m_layout;
    DragChartView *m_dragChartView;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
signals:
    void cancelled();
};
