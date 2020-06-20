#include "DeviceTreeView.hpp"

#include <DragChartView.hpp>
//#include <FunctionEditor.hpp>
#include <QCheckBox>
#include <QDebug>

Q_DECLARE_METATYPE(AssignableItemData)

DeviceTreeView::DeviceTreeView(QWidget *parent)
		: QTreeView(parent) {
	setSortingEnabled(true);
	auto triggers = editTriggers() ^= DoubleClicked;
	triggers |= SelectedClicked;
	setEditTriggers(SelectedClicked | EditKeyPressed);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTreeView::customContextMenuRequested, [this](QPoint point) {
		auto index = indexAt(point);
		auto data = index.data(DeviceModel::AssignableRole);
		QMenu menu;
		if (data.canConvert<AssignableItemData>()) {
			/*auto a_data = data.value<AssignableItemData>();
			QCheckBox commitCb("Commit");
			auto commitAct = new QWidgetAction(&menu);
			commitAct->setDefaultWidget(&commitCb);
			commitCb.setChecked(a_data.committal());
			connect(&commitCb, &QCheckBox::toggled, [&](bool toggled) {
				a_data.setCommittal(toggled);
			});
			// Write the committal value to the model only on menu close
			connect(&menu, &QMenu::aboutToHide, [&] {
				QVariant v;
				v.setValue(a_data);
				m_deviceModel.setData(index, v, DeviceModel::AssignableRole);
			});
			menu.addAction(commitAct);
			menu.exec(QCursor::pos());*/
			
			//auto dragView = new FunctionEditor;
			//dragView->show();
			
			functionEditorRequested();
		}
	});
	m_delegate = new DeviceModelDelegate(this);
	
	setItemDelegate(m_delegate);
}
