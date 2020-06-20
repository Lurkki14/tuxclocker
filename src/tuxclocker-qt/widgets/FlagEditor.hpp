#pragma once

// Combobox editor for QFlags types

// Qt can't use its own signals in templated classes
#include <boost/signals2.hpp>
#include <fplus/fplus.hpp>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QEvent>
#include <QFlags>
#include <QHash>
#include <QIcon>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QStylePainter>

template <typename T>
class FlagEditor : public QComboBox {
public:
	// Need to do the templated stuff in the header because C++ is retarded
	FlagEditor(QVector<std::tuple<QString, QIcon, T>> flagData,
			QWidget *parent = nullptr) : QComboBox(parent), m_skipNextHide(false) {
		view()->viewport()->installEventFilter(this);
		auto model = new QStandardItemModel(this);
		
		for (const auto &flagItem : flagData) {
			auto item = new QStandardItem;
			
			auto [text, icon, flag] = flagItem;
			//auto icon = style()->standardIcon(QStyle::SP_ComputerIcon);
			//item->setData(icon, Qt::DecorationRole);
			
			item->setCheckable(true);
			item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			item->setData(Qt::Checked, Qt::CheckStateRole);
			item->setData(icon ,Qt::DecorationRole);
			item->setText(text);
			// Cast to uint so we don't need to register the type
			auto uFlag = static_cast<uint>(flag);
			item->setData(uFlag, FlagRole);
			m_flagHash.insert(uFlag, item);
			model->appendRow(item);
		}
		setModel(model);
		
		connect(model, &QStandardItemModel::itemChanged,
				[this](QStandardItem *item) {
			auto flag = static_cast<QFlags<T>>(item->data(FlagRole).toUInt());
			auto checkState = item->data(Qt::CheckStateRole)
				.value<Qt::CheckState>();
			// Shouldn't ever be partially checked
			if (checkState == Qt::Unchecked)
				// Remove flag
				m_flags &= ~flag;
			else
				m_flags |= flag;
			flagsChanged(m_flags);
		});
		
		// Cause a repaint on changed flags
		// Boost won't let me discard the argument :(
		flagsChanged.connect([this](auto) {update();});
	}
	void setFlags(QFlags<T> flags) {
		m_flags = flags;
		flagsChanged(flags);
		auto handledFlags = m_flagHash.keys();
		auto uFlags = static_cast<uint>(flags);
		
		for (const auto &flag : handledFlags) {
			if (uFlags & flag)
				m_flagHash.value(flag)->setData(Qt::Checked, Qt::CheckStateRole);
		}
	}
	//QFlags<T> flags() {}
	
	bool eventFilter(QObject *obj, QEvent *ev) {
		if (ev->type() == QEvent::MouseButtonPress && obj == view()->viewport())
			m_skipNextHide = true;
		return QComboBox::eventFilter(obj, ev);
	}
	virtual void hidePopup() {
		if (m_skipNextHide)
			m_skipNextHide = false;
		else
			QComboBox::hidePopup();
	}
	boost::signals2::signal<void(QFlags<T>)> flagsChanged;
protected:
	void paintEvent(QPaintEvent*) {
		// Override paintEvent to draw custom text in the box
		QStylePainter painter(this);
		QStyleOptionComboBox opt;
		initStyleOption(&opt);
		
		// Show which flags are selected
		auto items = m_flagHash.values();
		// Not sure why using 'auto' breaks something mysteriously here
		QList<QStandardItem*> checked = fplus::keep_if([](QStandardItem *item) {
			return item->data(Qt::CheckStateRole)
				.value<Qt::CheckState>() == Qt::Checked;
		}, items);

		// Where the first icon is drawn
		auto startY = opt.rect.top() + iconSize().height() / 2;
		auto startX = opt.rect.left() + iconSize().width() / 2;
		auto deltaX = iconSize().width();

		painter.drawComplexControl(QStyle::CC_ComboBox, opt);

		// TODO: this probably breaks when drawRect goes over the combobox's right edge
		// Draw icons of selected items
		QRect drawRect(QPoint(startX, startY), iconSize());
		for (const auto &item : checked) {
			auto pixmap = item->data(Qt::DecorationRole)
				.value<QIcon>().pixmap(iconSize());
			painter.drawItemPixmap(drawRect,
				Qt::AlignCenter, pixmap);
			drawRect.moveRight(drawRect.right() + deltaX);
		}
	}
private:
	bool m_skipNextHide;
	const int FlagRole = Qt::UserRole + 1;
	QFlags<T> m_flags;
	QHash<uint, QStandardItem*> m_flagHash; 
};
