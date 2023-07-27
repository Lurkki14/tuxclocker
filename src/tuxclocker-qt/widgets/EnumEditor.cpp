#include "EnumEditor.hpp"

#include <fplus/fplus.hpp>
#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QStandardItem>
#include <QVariant>
#include <QVector>

using namespace fplus;

constexpr int KeyRole = Qt::UserRole + 1; // Stores the associated key (uint)

EnumEditor::EnumEditor(QWidget *parent) : AbstractAssignableEditor(parent) {
	installEventFilter(this);

	setAutoFillBackground(true);
	auto layout = new QHBoxLayout(this);
	layout->setMargin(0);

	m_comboBox = new QComboBox;
	layout->addWidget(m_comboBox);

	connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
	    [this](int) { emit editingDone(); });
}

EnumEditor::EnumEditor(TuxClocker::Device::EnumerationVec enums, QWidget *parent)
    : EnumEditor(parent) {
	auto qStrings = QVector<QString>::fromStdVector(
	    transform([](auto e) { return QString::fromStdString(e.name); }, enums));

	for (uint i = 0; i < qStrings.length(); i++) {
		auto item = new QStandardItem;
		item->setText(qStrings[i]);
		item->setData(i, KeyRole);
		m_model.appendRow(item);
	}

	m_model.sort(0);
	m_comboBox->setModel(&m_model);
}

QVariant EnumEditor::assignableData() {
	auto r = m_model.index(m_comboBox->currentIndex(), 0).data(KeyRole).toUInt();
	return r;
}
QString EnumEditor::displayData() {
	auto r = m_model.index(m_comboBox->currentIndex(), 0).data(Qt::DisplayRole).toString();
	return r;
}
void EnumEditor::setAssignableData(QVariant data) {
	// TODO: make worst case better than O(n)
	auto u = data.toUInt();
	for (int i = 0; i < m_comboBox->model()->rowCount(); i++) {
		if (m_comboBox->itemData(i, KeyRole).toUInt() == u) {
			m_comboBox->setCurrentIndex(i);
			break;
		}
	}
}

bool EnumEditor::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Show)
		m_comboBox->showPopup();
	return true;
}
