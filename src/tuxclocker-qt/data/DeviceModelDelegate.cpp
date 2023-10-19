#include "DeviceModelDelegate.hpp"

#include "DeviceModel.hpp"
#include <DoubleRangeEditor.hpp>
#include <EnumEditor.hpp>
#include <FunctionEditor.hpp>
#include <Globals.hpp>
#include <IntRangeEditor.hpp>
#include <libintl.h>
#include <patterns.hpp>
#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QSettings>
#include <Utils.hpp>

#define _(String) gettext(String)

using namespace TuxClocker::Device;
using namespace mpark::patterns;

Q_DECLARE_METATYPE(AssignableItemData)

DeviceModelDelegate::DeviceModelDelegate(QObject *parent) : QStyledItemDelegate(parent) {
	m_parametrize = new QAction{_("Parametrize..."), this};
	m_resetAssignable = new QAction{_("Reset to default"), this};

	m_functionEditor = nullptr;
}

void DeviceModelDelegate::commitAndClose() {
	// It's also retarded to get the editor this way when we could just use it in the lambda
	auto editor = qobject_cast<AbstractAssignableEditor *>(sender());
	emit commitData(editor);
	emit closeEditor(editor);
}

QWidget *DeviceModelDelegate::createEditor(
    QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	AbstractAssignableEditor *editor = nullptr;
	if (v.canConvert<AssignableItemData>()) {
		match(v.value<AssignableItemData>().assignableInfo())(
		    pattern(as<RangeInfo>(arg)) =
			[&](auto r_info) {
				match(r_info)(
				    pattern(as<Range<int>>(arg)) =
					[&](auto ir) { editor = new IntRangeEditor(ir, parent); },
				    pattern(as<Range<double>>(arg)) =
					[&](auto dr) {
						editor = new DoubleRangeEditor(dr, parent);
					});
			},
		    pattern(as<EnumerationVec>(arg)) =
			[&](auto ev) { editor = new EnumEditor(ev, parent); });
	}

	// This is really retarded, why can't we just do this in a lambda??? (Some const shit)
	connect(editor, &AbstractAssignableEditor::editingDone, this,
	    &DeviceModelDelegate::commitAndClose);

	return editor;
}

void DeviceModelDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	if (v.canConvert<AssignableItemData>()) {
		auto data = v.value<AssignableItemData>().value();
		auto a_editor = static_cast<AbstractAssignableEditor *>(editor);
		a_editor->setAssignableData(data);
	}
}

void DeviceModelDelegate::setModelData(
    QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	auto v = index.data(DeviceModel::AssignableRole);
	if (v.canConvert<AssignableItemData>()) {
		auto a_editor = static_cast<AbstractAssignableEditor *>(editor);
		auto data = index.data(DeviceModel::AssignableRole).value<AssignableItemData>();

		setAssignableData(model, index, a_editor->assignableData());
	}
}

void DeviceModelDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
	// Why do I need to override this to perform such a basic task?
	editor->setGeometry(option.rect);
}

bool DeviceModelDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
    const QStyleOptionViewItem &item, const QModelIndex &index) {
	// Context menu handling
	if (event->type() == QEvent::MouseButtonRelease) {
		// Disconnect so all the previous context menu actions don't go off
		// Private signals also get disconnected if we don't specify any signal
		if (m_functionEditor)
			disconnect(m_functionEditor, &FunctionEditor::connectionDataChanged,
			    nullptr, nullptr);
		disconnect(m_parametrize, nullptr, nullptr, nullptr);
		disconnect(m_resetAssignable, nullptr, nullptr, nullptr);
		m_menu.clear();

		auto mouse = dynamic_cast<QMouseEvent *>(event);

		// Check if any children have saved default assignable values
		if (mouse->button() == Qt::RightButton &&
		    subtreeHasAssignableDefaults(model, index)) {
			auto defaults = subtreeAssignableDefaults(model, index);

			connect(m_resetAssignable, &QAction::triggered,
			    [=]() { setAssignableDefaults(model, defaults); });

			m_menu.addAction(m_resetAssignable);
		}

		auto data = index.data(DeviceModel::AssignableRole);
		auto assInfo = data.value<AssignableItemData>();
		// TODO: need a different check for future 'Reset assignable' action
		if (mouse->button() == Qt::RightButton && data.canConvert<AssignableItemData>() &&
		    std::holds_alternative<RangeInfo>(assInfo.assignableInfo())) {
			// Initialize FunctionEditor with model once
			if (!m_functionEditor) {
				// This cast should be valid since we can fetch AssignableData
				auto devModel = static_cast<DeviceModel *>(model);
				m_functionEditor = new FunctionEditor{*devModel};

				Globals::g_mainStack->addWidget(m_functionEditor);
			}
			m_functionEditor->setRangeInfo(
			    std::get<RangeInfo>(assInfo.assignableInfo()));
			auto name = index.data(DeviceModel::NodeNameRole).toString();
			auto unit = assInfo.unit();
			m_functionEditor->setAssignableText(name, unit);

			m_menu.addAction(m_parametrize);

			connect(m_parametrize, &QAction::triggered, [=](auto) {
				Globals::g_mainStack->setCurrentWidget(m_functionEditor);
			});

			connect(m_functionEditor, &FunctionEditor::cancelled, []() {
				Globals::g_mainStack->setCurrentWidget(Globals::g_deviceBrowser);
			});

			// TODO: not handled in AssignableProxy
			connect(m_functionEditor, &FunctionEditor::connectionDataChanged,
			    [=](auto data) {
				    setAssignableData(model, index, data);
				    Globals::g_mainStack->setCurrentWidget(
					Globals::g_deviceBrowser);
			    });
		}
		if (!m_menu.actions().empty())
			m_menu.exec(mouse->globalPos());
	}

	return QStyledItemDelegate::editorEvent(event, model, item, index);
}

template <typename T>
void DeviceModelDelegate::setAssignableData(
    QAbstractItemModel *model, const QModelIndex &index, T data) {
	auto assData = index.data(DeviceModel::AssignableRole).value<AssignableItemData>();
	QVariant assV;
	assV.setValue(data);
	assData.setValue(assV);

	QVariant v;
	v.setValue(assData);

	model->setData(index, v, DeviceModel::AssignableRole);
}

void DeviceModelDelegate::setAssignableDefaults(
    QAbstractItemModel *model, QVector<AssignableDefaultData> defaults) {
	for (auto &def : defaults) {
		auto data = def.index.data(DeviceModel::AssignableRole).value<AssignableItemData>();

		setAssignableVariantData(model, def.index, def.defaultValue);
	}
}

void DeviceModelDelegate::setAssignableVariantData(
    QAbstractItemModel *model, const QModelIndex &index, QVariant data) {
	auto assData = index.data(DeviceModel::AssignableRole).value<AssignableItemData>();
	assData.setValue(data);

	QVariant v;
	v.setValue(assData);

	model->setData(index, v, DeviceModel::AssignableRole);
}

bool DeviceModelDelegate::subtreeHasAssignableDefaults(
    QAbstractItemModel *model, const QModelIndex &index) {
	QSettings settings{"tuxclocker"};
	settings.beginGroup("assignableDefaults");
	bool hasDefaults = false;

	auto cb = [&settings, &hasDefaults](QAbstractItemModel *model, const QModelIndex &index,
		      int row) -> std::optional<const QModelIndex> {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto assProxyV = ifaceIndex.data(DeviceModel::AssignableProxyRole);

		auto name = model->index(row, DeviceModel::NameColumn, index).data();
		if (assProxyV.isValid()) {
			auto nodePath = qvariant_cast<AssignableProxy *>(assProxyV)->dbusPath();
			if (settings.contains(Utils::toSettingsPath(nodePath))) {
				// This stops traversing model
				hasDefaults = true;
				return std::nullopt;
			}
		}
		return model->index(row, DeviceModel::NameColumn, index);
	};
	auto nameIndex = model->index(index.row(), DeviceModel::NameColumn, index.parent());
	Utils::traverseModel(cb, model, nameIndex);

	return hasDefaults;
}

QVector<AssignableDefaultData> DeviceModelDelegate::subtreeAssignableDefaults(
    QAbstractItemModel *model, const QModelIndex &index) {
	QVector<AssignableDefaultData> retval;
	QSettings settings{"tuxclocker"};
	settings.beginGroup("assignableDefaults");
	auto paths = settings.childKeys();

	auto cb = [&](auto model, auto index, int row) {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto assProxyV = ifaceIndex.data(DeviceModel::AssignableProxyRole);

		if (assProxyV.isValid()) {
			auto proxy = qvariant_cast<AssignableProxy *>(assProxyV);
			// Check if there is a default for this node
			// TODO: might need to make this faster by providing
			// a hash table in DeviceModel for example
			for (auto &path : paths) {
				if (proxy->dbusPath() == Utils::fromSettingsPath(path)) {
					retval.append(AssignableDefaultData{
					    .index = ifaceIndex,
					    .defaultValue = settings.value(path),
					});
				}
			}
		}
		return model->index(row, DeviceModel::NameColumn, index);
	};
	auto nameIndex = model->index(index.row(), DeviceModel::NameColumn, index.parent());
	Utils::traverseModel(cb, model, nameIndex);

	return retval;
}

QColor alphaBlend(QColor top, QColor background) {
	auto alpha = top.alphaF();
	auto factor = 1 - alpha;
	return QColor((top.red() * alpha) + (factor * background.red()),
	    (top.green() * alpha) + (factor * background.green()),
	    (top.blue() * alpha) + (factor * background.blue()));
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

	return QColor(color.red() * factorR, color.green() * factorG, color.blue() * factorB);
}

void DeviceModelDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
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
