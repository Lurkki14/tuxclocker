#pragma once

#include <boost/signals2.hpp>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>

#include "DeviceModel.hpp"

// TODO: If a disabled item is clicked, the item that is highlighted get selected instead
class NodeSelector : public QComboBox {
public:
	NodeSelector(QWidget *parent = nullptr) : QComboBox(parent), m_skipNextHide(false) {
		view()->viewport()->installEventFilter(this);
	}
	bool eventFilter(QObject *obj, QEvent *ev) {
		if (ev->type() == QEvent::MouseButtonPress && obj == view()->viewport()) {
			auto mouse_ev = static_cast<QMouseEvent *>(ev);
			auto index = view()->indexAt(mouse_ev->pos());
			// Hide popup when an enabled item is clicked
			if (!view()->visualRect(index).contains(mouse_ev->pos()) ||
			    !(model()->flags(index) & Qt::ItemIsEnabled))
				m_skipNextHide = true;
			else if (model()->flags(index) & Qt::ItemIsSelectable) {
				qDebug() << index.data(DeviceModel::DynamicReadableProxyRole);
				indexChanged(index);
				m_skipNextHide = false;
			}
		}
		return QComboBox::eventFilter(obj, ev);
	}
	virtual void hidePopup() override {
		if (m_skipNextHide)
			m_skipNextHide = false;
		else
			QComboBox::hidePopup();
	}
	/*void mouseReleaseEvent(QMouseEvent *event) override {
		auto index = view()->indexAt(event->pos());
		if (!view()->visualRect(index).contains(event->pos()) ||
				!(model()->flags(index) & Qt::ItemIsEnabled))
			qDebug() << index.data();

		QComboBox::mouseReleaseEvent(event);
	}*/
	void setView(QAbstractItemView *view) {
		// Why no signal, Qt?
		view->viewport()->installEventFilter(this);

		QComboBox::setView(view);
	}
	virtual void showPopup() override {
		// TODO: don't account for scrollbar width when it isn't visible
		// This is quite a weird way to do it but it works
		auto vBarWidth = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		view()->setMinimumWidth(view()->sizeHintForColumn(0) + vBarWidth);
		QComboBox::showPopup();
	}
	boost::signals2::signal<void(QModelIndex &)> indexChanged;
private:
	bool m_skipNextHide;
};
