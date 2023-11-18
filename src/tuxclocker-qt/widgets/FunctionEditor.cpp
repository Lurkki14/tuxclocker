#include "FunctionEditor.hpp"

#include <DeviceProxyModel.hpp>
#include <Globals.hpp>

QWidget *Globals::g_functionEditor;

FunctionEditor::FunctionEditor(DeviceModel &model, QWidget *parent)
    : QWidget(parent), m_model(model), m_proxyModel(model) {
	m_latestNodeIndex = std::nullopt;

	m_proxyModel.setDisableFiltered(true);
	m_proxyModel.setFlags(DeviceModel::DynamicReadable);
	m_proxyModel.setShowIcons(false);

	m_layout = new QGridLayout(this);
	m_dependableReadableComboBox = new NodeSelector{this};

	// Tree view for device tree
	auto treeView = new QTreeView{this};
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
	m_applyButton = new QPushButton(_("Apply"));
	// No connection to apply at first
	m_applyButton->setEnabled(false);
	m_cancelButton = new QPushButton(_("Cancel"));
	m_layout->addWidget(m_cancelButton, 3, 0, 1, 1);
	m_layout->addWidget(m_applyButton, 3, 1, 1, 1);

	m_applyButton->setToolTip(disabledReason());

	connect(m_dragView, &DragChartView::pointsChanged, [=](auto points) {
		if (points.length() > 1 && m_latestNodeIndex.has_value())
			emit canApplyChanged(true);
		else {
			emit canApplyChanged(false);
			m_applyButton->setToolTip(disabledReason());
		}
	});

	connect(m_cancelButton, &QPushButton::clicked, this, &FunctionEditor::cancelled);

	connect(this, &FunctionEditor::canApplyChanged, [=](bool canApply) {
		if (canApply)
			m_applyButton->setToolTip("");
		m_applyButton->setEnabled(canApply);
	});

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
		    .rangeInfo = m_rangeInfo,
		};
		emit connectionDataChanged(data);
		this->close();
	});

	m_dependableReadableComboBox->indexChanged.connect([this](QModelIndex &index) {
		m_latestNodeIndex = index;
		auto nodeName = index.data(Qt::DisplayRole).toString();
		m_dragView->xAxis().setTitleText(nodeName);

		// DynamicReadableProxy from adjacent column
		auto dynProxyV =
		    index.model()
			->index(index.row(), DeviceModel::InterfaceColumn, index.parent())
			.data(DeviceModel::DynamicReadableProxyRole);
		if (dynProxyV.isValid()) {
			auto unit = dynProxyV.value<DynamicReadableProxy *>()->unit();
			if (unit.has_value()) {
				auto text = QString{"%1 (%2)"}.arg(nodeName, *unit);
				m_dragView->xAxis().setTitleText(text);
			} else
				m_dragView->xAxis().setTitleText(nodeName);
		} else
			m_dragView->xAxis().setTitleText(nodeName);

		if (m_dragView->vector().length() > 1)
			emit canApplyChanged(true);
		else {
			emit canApplyChanged(false);
			m_applyButton->setToolTip(disabledReason());
		}
	});

	setLayout(m_layout);

	Globals::g_functionEditor = this;
}
