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

// Delet this
namespace p = mpark::patterns;

Q_DECLARE_METATYPE(DynamicReadableProxy *)

// TODO: make constructor of the type data Editor a = Maybe (Range a)
class FunctionEditor : public QWidget {
public:
	FunctionEditor(DeviceModel &model, QWidget *parent = nullptr)
	    : QWidget(parent), m_model(model), m_proxyModel(model) {
		m_latestNodeIndex = std::nullopt;

		m_proxyModel.setDisableFiltered(true);
		m_proxyModel.setFlags(DeviceModel::DynamicReadable);
		m_proxyModel.setShowIcons(false);

		m_layout = new QGridLayout(this);
		m_dependableReadableComboBox = new NodeSelector;

		// Tree view for device tree
		auto treeView = new QTreeView;
		m_dependableReadableComboBox->setModel(&m_proxyModel);
		m_dependableReadableComboBox->setView(treeView);
		treeView->expandAll();
		// Try not to cut off node names
		treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

		m_dependableLabel = new QLabel;
		m_layout->addWidget(m_dependableLabel, 0, 0, 1, 2);
		m_layout->addWidget(m_dependableReadableComboBox, 1, 0, 1, 2);
		m_dragView = new DragChartView;

		// m_dragView->setRange(0, 100, 0, 100);
		m_layout->addWidget(m_dragView, 2, 0, 1, 2);
		m_applyButton = new QPushButton("Apply");
		// No connection to apply at first
		m_applyButton->setEnabled(false);
		m_cancelButton = new QPushButton("Cancel");
		m_layout->addWidget(m_cancelButton, 3, 0, 1, 1);
		m_layout->addWidget(m_applyButton, 3, 1, 1, 1);

		connect(m_dragView, &DragChartView::pointsChanged, [=](auto points) {
			if (points.length() > 1 && m_latestNodeIndex.has_value())
				emit canApplyChanged(true);
			else {
				emit canApplyChanged(false);
				m_applyButton->setToolTip(disabledReason());
			}
		});

		connect(this, &FunctionEditor::canApplyChanged, m_applyButton,
		    &QPushButton::setEnabled);

		connect(m_applyButton, &QPushButton::clicked, [this] {
			auto index = m_latestNodeIndex.value();
			auto proxy = index.data(DeviceModel::DynamicReadableProxyRole)
					 .value<DynamicReadableProxy *>();
			auto points = m_dragView->vector();
			if (points.length() < 2)
				return;

			auto data = DynamicReadableConnectionData{
			    .points = points,
			    .dynamicReadablePath = proxy->dbusPath(),
			};
			emit connectionDataChanged(data);
			this->close();
		});

		m_dependableReadableComboBox->indexChanged.connect([this](auto &index) {
			m_latestNodeIndex = index;
			auto nodeName = index.data(Qt::DisplayRole).toString();
			m_dragView->xAxis().setTitleText(nodeName);

			if (m_dragView->vector().length() > 1)
				emit canApplyChanged(true);
			else {
				emit canApplyChanged(false);
				m_applyButton->setToolTip(disabledReason());
			}
		});

		setLayout(m_layout);
	}
	// Somehow existing points disappear somewhere in these two?
	void setRangeInfo(TuxClocker::Device::RangeInfo rangeInfo) {
		p::match(rangeInfo)(
		    p::pattern(p::as<TuxClocker::Device::Range<double>>(p::arg)) =
			[this](auto dr) { m_dragView->setRange(0, 100, dr.min, dr.max); },
		    p::pattern(p::as<TuxClocker::Device::Range<int>>(p::arg)) =
			[this](auto ir) { m_dragView->setRange(0, 100, ir.min, ir.max); });
	}
	void setAssignableName(QString name) {
		m_dependableLabel->setText(QString{"Connecting %1 with:"}.arg(name));
		m_dragView->yAxis().setTitleText(name);
	}

	boost::signals2::signal<void(std::shared_ptr<AssignableConnection>)>
	    assignableConnectionChanged;
signals:
	void connectionDataChanged(DynamicReadableConnectionData);
private:
	Q_OBJECT

	DeviceModel &m_model;
	DeviceProxyModel m_proxyModel;
	DragChartView *m_dragView;
	QComboBox *m_functionComboBox;
	NodeSelector *m_dependableReadableComboBox;
	QGridLayout *m_layout;
	QLabel *m_dependableLabel;
	std::optional<QModelIndex> m_latestNodeIndex;
	QPushButton *m_applyButton, *m_cancelButton;

	QString disabledReason() {
		QString reason;

		if (!m_latestNodeIndex.has_value())
			reason.append("A node to connect with needs to be selected");
		if (m_dragView->vector().length() < 2) {
			if (!reason.isEmpty())
				reason.append('\n');
			reason.append("At least two points need to be placed");
		}
		return reason;
	}
signals:
	void canApplyChanged(bool);
};
