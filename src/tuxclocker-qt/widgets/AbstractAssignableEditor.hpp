#pragma once

#include <QWidget>

class AbstractAssignableEditor : public QWidget {
public:
	AbstractAssignableEditor(QWidget *parent = nullptr) : QWidget(parent) {}
	virtual ~AbstractAssignableEditor() {}
	virtual QVariant assignableData() = 0;
	virtual QString displayData() = 0;
	virtual void setAssignableData(QVariant data) = 0;
signals:
	void editingDone();
private:
	Q_OBJECT
};
