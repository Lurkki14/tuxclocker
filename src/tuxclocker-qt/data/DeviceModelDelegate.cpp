#include "DeviceModelDelegate.hpp"

#include "DeviceModel.hpp"
#include <DoubleRangeEditor.hpp>
#include <EnumEditor.hpp>
#include <IntRangeEditor.hpp>
#include <patterns.hpp>
#include <QDebug>
#include <QPainter>

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
					}
				);
			},
			pattern(as<EnumerationVec>(arg)) = [&](auto ev) {
				editor = new EnumEditor(ev, parent);
			}
		);
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

QColor alphaBlend(QColor top, QColor background) {
	auto alpha = top.alphaF();
	auto factor = 1 - alpha;
	return QColor(
		(top.red() * alpha) + (factor * background.red()),
		(top.green() * alpha) + (factor * background.green()),
		(top.blue() * alpha) + (factor * background.blue())
	);
}

QColor filter(QColor filter, QColor color) {	
	/* Dominant color lets 95% (in case of a theme using pure r/g/b) of that 
	   channel through, the rest 70% */
	auto factorR = filter.redF();
	auto factorG = filter.greenF();
	auto factorB = filter.blueF();
	
	Qt::GlobalColor dominant;
	
	if (factorR > qMax(factorG, factorB))
		dominant = Qt::red;
	if (factorG > qMax(factorR, factorB))
		dominant = Qt::green;
	if (factorB > qMax(factorR, factorG))
		dominant = Qt::blue;
	
	factorR = (dominant == Qt::red) ? 0.95 : 0.7;
	factorG = (dominant == Qt::green) ? 0.95 : 0.7;
	factorB = (dominant == Qt::blue) ? 0.95 : 0.7;
	
	return QColor(
		color.red() * factorR,
		color.green() * factorG,
		color.blue() * factorB
	);
}

void DeviceModelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
		const QModelIndex &index) const  {
	auto optCpy = option;
	auto baseColor = index.data(Qt::BackgroundRole).value<QColor>();
	if (baseColor.isValid()) {
		// Filter background color through the highlight color
		auto topColor = option.palette.color(QPalette::Highlight);
		auto color = filter(topColor, baseColor);
		QPalette p = option.palette;
		p.setColor(QPalette::Highlight, color);
		optCpy.palette = p;
	}
	QStyledItemDelegate::paint(painter, optCpy, index);
}
