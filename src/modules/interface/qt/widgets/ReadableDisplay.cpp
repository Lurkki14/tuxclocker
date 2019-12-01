#include "ReadableDisplay.h"
#include <ReadableObservableManager.h>

ReadableDisplay::ReadableDisplay(QWidget *parent) : QWidget(parent) {
    setAcceptDrops(true);
    
    m_displayWidgetLayout = new QVBoxLayout;
    
    m_displayWidgetLayout->addWidget(new ReadableGraphDisplay(new ReadableObservableManager));
    
    setLayout(m_displayWidgetLayout);
}

void ReadableDisplay::dropEvent(QDropEvent *event) {
}

void ReadableDisplay::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasFormat(ReadableData::mimeType())) {
        return;
    }
    event->accept();
}
