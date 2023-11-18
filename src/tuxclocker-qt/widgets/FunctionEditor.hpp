#pragma once

/* Widget for editing the function for an assignable.
   Possible options, where y = value of assignable, t = time,
   x = value of readable, n = static value set in model editor:
   - y = n
   - y = f(x)
   - y = f(t)
*/

#include "DynamicReadableProxy.hpp"
#include <AssignableConnection.hpp>
#include <AssignableProxy.hpp>
#include <boost/signals2.hpp>
#include <DeviceProxyModel.hpp>
#include <DragChartView.hpp>
#include <DynamicReadableConnection.hpp>
#include <libintl.h>
#include <NodeSelector.hpp>
#include <patterns.hpp>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QGridLayout>
#include <QLabel>
#include <QModelIndex>
#include <QPushButton>
#include <QTimer>
#include <QTreeView>
#include <QWidget>

#define _(String) gettext(String)

// Delet this
namespace p = mpark::patterns;

Q_DECLARE_METATYPE(DynamicReadableProxy *)

// TODO: make constructor of the type data Editor a = Maybe (Range a)
class FunctionEditor : public QWidget {
public:
	FunctionEditor(DeviceModel &model, QWidget *parent = nullptr);
	// Somehow existing points disappear somewhere in these two?
	void setRangeInfo(TuxClocker::Device::RangeInfo rangeInfo) {
		// HACK: points get deleted somwhere when requesting editor
		// for another node, but DragChartView::pointsChanged doesn't fire -_-
		emit canApplyChanged(false);

		p::match(rangeInfo)(
		    p::pattern(p::as<TuxClocker::Device::Range<double>>(p::arg)) =
			[this](auto dr) { m_dragView->setRange(0, 100, dr.min, dr.max); },
		    p::pattern(p::as<TuxClocker::Device::Range<int>>(p::arg)) =
			[this](auto ir) { m_dragView->setRange(0, 100, ir.min, ir.max); });

		m_applyButton->setToolTip(disabledReason());
		m_rangeInfo = rangeInfo;
	}
	void setAssignableText(QString name, std::optional<QString> unit) {
		m_dependableLabel->setText(QString{_("Connecting %1 with:")}.arg(name));
		m_dragView->yAxis().setTitleText(name);

		if (!unit.has_value())
			m_dragView->yAxis().setTitleText(name);
		else {
			auto text = QString{"%1 (%2)"}.arg(name, *unit);
			m_dragView->yAxis().setTitleText(text);
		}
	}

	boost::signals2::signal<void(std::shared_ptr<AssignableConnection>)>
	    assignableConnectionChanged;
signals:
	void cancelled();
	void connectionDataChanged(DynamicReadableConnectionData);
private:
	Q_OBJECT

	RangeInfo m_rangeInfo;
	DeviceModel &m_model;
	DeviceProxyModel m_proxyModel;
	DragChartView *m_dragView;
	QComboBox *m_functionComboBox;
	NodeSelector *m_dependableReadableComboBox;
	QGridLayout *m_layout;
	QLabel *m_dependableLabel;
	std::optional<QModelIndex> m_latestNodeIndex;
	QPushButton *m_applyButton, *m_cancelButton;
	// Used to determine if DeviceTreeView selection should be cleared
	bool m_active;

	QString disabledReason() {
		QString reason;

		if (!m_latestNodeIndex.has_value())
			reason.append(_("A node to connect with needs to be selected"));
		if (m_dragView->vector().length() < 2) {
			if (!reason.isEmpty())
				reason.append('\n');
			reason.append(_("At least two points need to be placed"));
		}
		return reason;
	}
signals:
	void canApplyChanged(bool);
};
