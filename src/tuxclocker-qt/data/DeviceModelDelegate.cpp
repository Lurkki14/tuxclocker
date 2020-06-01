#include "DeviceModelDelegate.hpp"

#include "DeviceModel.hpp"
#include <DoubleRangeEditor.hpp>
#include <IntRangeEditor.hpp>
#include <patterns.hpp>

using namespace TuxClocker::Device;
using namespace mpark::patterns;

Q_DECLARE_METATYPE(AssignableItemData)

DeviceModelDelegate::DeviceModelDelegate(QObject* parent) : 
		QStyledItemDelegate(parent) {}
		
QWidget *DeviceModelDelegate::createEditor(QWidget *parent,
		const QStyleOptionViewItem&, const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	QWidget *editor = nullptr;
	if (v.canConvert<AssignableItemData>()) {
		match(v.value<AssignableItemData>().assignableInfo())
			(pattern(as<RangeInfo>(arg)) = [&](auto r_info) {
				match(r_info)
					(pattern(as<Range<int>>(arg)) = [&](auto ir) {
						editor = new IntRangeEditor(ir, parent);
					},
					pattern(as<Range<double>>(arg)) = [&](auto dr) {	
						editor = new DoubleRangeEditor(dr, parent);
					});
		});
	}
	return editor;
}

void DeviceModelDelegate::setEditorData(QWidget *editor,
		const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	if (v.canConvert<AssignableItemData>()) {
		auto data = v.value<AssignableItemData>().value();
		auto a_editor = static_cast<AbstractAssignableEditor*>(editor);
		a_editor->setAssignableData(data);
	}
}

void DeviceModelDelegate::setModelData(QWidget *editor,
		QAbstractItemModel *model, const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	if (v.canConvert<AssignableItemData>()) {
		auto a_editor = static_cast<AbstractAssignableEditor*>(editor);
		auto data = index.data(DeviceModel::AssignableRole)
			.value<AssignableItemData>();
		data.setValue(a_editor->assignableData());
		QVariant v;
		v.setValue(data);
		model->setData(index, a_editor->displayData(), Qt::DisplayRole);
		model->setData(index, v, DeviceModel::AssignableRole);
	}
}

void DeviceModelDelegate::updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex&) const {
	// Why do I need to override this to perform such a basic task?
	editor->setGeometry(option.rect);
}

/*void DeviceModelDelegate::drawCheck(QPainter *painter,
		const QStyleOptionViewItem &option, const QRect &rect,
		Qt::CheckState state) const {
	QStyledItemDelegate::drawCheck(painter, option, rect, state)
}*/
