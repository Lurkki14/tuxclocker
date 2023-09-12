#include "Settings.hpp"

#include <Globals.hpp>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

SettingsData Globals::g_settingsData;

Settings::Settings(QWidget *parent) : QWidget(parent) {
	auto layout = new QGridLayout{this};

	auto label = new QLabel{this};
	// Bigger and bolder text
	QFont biggerPoint{};
	biggerPoint.setBold(true);
	biggerPoint.setPointSize(biggerPoint.pointSize() + 4);
	label->setTextFormat(Qt::RichText);
	label->setFont(biggerPoint);
	label->setText("Settings");

	m_autoLoad = new QCheckBox{"Apply profile settings automatically", this};

	auto profileCheck = new QCheckBox{"Use profile", this};

	// TODO: add delegate to make deleting a little nicer
	m_profileView = new QListWidget{this};
	auto triggers = QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed;
	m_profileView->setEditTriggers(triggers);
	m_profileView->setEnabled(false);

	auto addButton = new QPushButton{"Add profile"};
	addButton->setEnabled(false);

	auto removeButton = new QPushButton{"Remove selected"};

	connect(addButton, &QPushButton::released, [=] {
		auto item = new QListWidgetItem{"Unnamed"};
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		m_profileView->addItem(item);
	});

	connect(removeButton, &QPushButton::released,
	    [=] { m_profileView->model()->removeRow(m_profileView->currentRow()); });

	connect(profileCheck, &QCheckBox::stateChanged, [=](auto state) {
		bool enable = (state == Qt::Unchecked) ? false : true;

		addButton->setEnabled(enable);
		m_profileView->setEnabled(enable);
	});

	auto cancelButton = new QPushButton{"Cancel", this};

	connect(cancelButton, &QPushButton::released, this, &Settings::cancelled);

	auto saveButton = new QPushButton{"Save", this};

	connect(saveButton, &QPushButton::released, this, [=] {
		// TODO: read assignableSettings from disk
		auto settingsData = fromUIState();
		writeSettings(fromUIState());

		Globals::g_settingsData = settingsData;

		emit settingsSaved(settingsData);
	});

	layout->addWidget(label, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop);
	layout->addWidget(m_autoLoad, 1, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(profileCheck, 2, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(m_profileView, 2, 1, 1, 2);
	layout->addWidget(addButton, 3, 1);
	layout->addWidget(removeButton, 3, 2);
	layout->addWidget(cancelButton, 4, 0, 1, 1, Qt::AlignBottom);
	layout->addWidget(saveButton, 4, 1, 1, 2, Qt::AlignBottom);

	this->setLayout(layout);
}

SettingsData Settings::fromUIState() {
	std::optional<QString> currentProfile = std::nullopt;

	auto data = m_profileView->currentItem()->data(Qt::DisplayRole);
	if (data.isValid())
		currentProfile = data.toString();

	return SettingsData{
	    .autoApplyProfile = m_autoLoad->isChecked(),
	    .currentProfile = currentProfile,
	};
}

void Settings::writeSettings(SettingsData data) {
	bool usingProfile = data.currentProfile.has_value() ? true : false;

	QSettings settings{"tuxclocker"};
	settings.beginGroup("general");

	settings.setValue("autoApplyProfile", data.autoApplyProfile);
	settings.setValue("usingProfile", usingProfile);

	if (usingProfile)
		settings.setValue("currentProfile", data.currentProfile.value());
}
