#include "Settings.hpp"

#include <DynamicReadableConnectionData.hpp>
#include <Globals.hpp>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <Utils.hpp>

Settings::Settings(QWidget *parent) : QWidget(parent) {
	qRegisterMetaTypeStreamOperators<QVector<QString>>("QVector<QString>>");

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

	m_useProfile = new QCheckBox{"Use profile", this};

	// TODO: add delegate to make deleting a little nicer
	m_profileView = new QListWidget{this};
	m_profileView->setEnabled(false);
	m_profileView->setSelectionMode(QAbstractItemView::SingleSelection);

	auto addButton = new QPushButton{"Add profile"};
	addButton->setEnabled(false);

	auto removeButton = new QPushButton{"Remove selected"};

	connect(addButton, &QPushButton::released, [=] {
		auto item = new QListWidgetItem{"Unnamed"};
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		m_profileView->addItem(item);
		m_profileView->editItem(item);
		// Disable editing since renaming wouldn't function as expected
		// (doesn't rename profile's assignable settings)
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	});

	// TODO: this doesn't remove the profile's assignableSettings from the file
	connect(removeButton, &QPushButton::released,
	    [=] { m_profileView->model()->removeRow(m_profileView->currentRow()); });

	connect(m_useProfile, &QCheckBox::stateChanged, [=](auto state) {
		bool enable = (state == Qt::Unchecked) ? false : true;

		addButton->setEnabled(enable);
		m_profileView->setEnabled(enable);
	});

	auto cancelButton = new QPushButton{"Cancel", this};

	connect(cancelButton, &QPushButton::released, this, &Settings::cancelled);

	auto saveButton = new QPushButton{"Save", this};

	connect(saveButton, &QPushButton::released, this, [=] {
		auto settingsData = fromUIState();
		writeSettings(fromUIState());

		Globals::g_settingsData = settingsData;

		emit settingsSaved(settingsData);
	});

	setUIState(readSettings());

	layout->addWidget(label, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop);
	layout->addWidget(m_autoLoad, 1, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(m_useProfile, 2, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(m_profileView, 2, 1, 1, 2);
	layout->addWidget(addButton, 3, 1);
	layout->addWidget(removeButton, 3, 2);
	layout->addWidget(cancelButton, 4, 0, 1, 1, Qt::AlignBottom);
	layout->addWidget(saveButton, 4, 1, 1, 2, Qt::AlignBottom);

	this->setLayout(layout);
}

void Settings::setUIState(SettingsData data) {
	m_useProfile->setChecked(data.currentProfile.has_value());

	m_autoLoad->setChecked(data.autoApplyProfile);

	for (auto &profile : data.profiles) {
		auto item = new QListWidgetItem{profile};
		m_profileView->addItem(item);
		if (data.currentProfile.has_value() && data.currentProfile.value() == profile)
			m_profileView->setCurrentItem(item);
	}
}

SettingsData Settings::fromUIState() {
	std::optional<QString> currentProfile = std::nullopt;

	if (m_profileView->currentItem() && m_useProfile->isChecked()) {
		auto data = m_profileView->currentItem()->data(Qt::DisplayRole);
		if (data.isValid())
			currentProfile = data.toString();
	}

	QVector<QString> profiles;
	auto cb = [&](auto model, auto index, int row) {
		auto next = model->index(row, 0, index);

		if (next.data(Qt::DisplayRole).isValid())
			profiles.append(next.data(Qt::DisplayRole).toString());
		return next;
	};
	Utils::traverseModel(cb, m_profileView->model());

	// Read assignableSettings for wanted profile
	QVector<AssignableSetting> assSettings;
	if (currentProfile.has_value())
		assSettings = readAssignableSettings(currentProfile.value());

	return SettingsData{
	    .autoApplyProfile = m_autoLoad->isChecked(),
	    .currentProfile = currentProfile,
	    .assignableSettings = assSettings,
	    .profiles = profiles,
	};
}

void Settings::writeSettings(SettingsData data) {
	bool usingProfile = data.currentProfile.has_value() ? true : false;

	QSettings settings{"tuxclocker"};

	settings.setValue("autoApplyProfile", data.autoApplyProfile);
	settings.setValue("usingProfile", usingProfile);
	// We need to save this in case nothing is changed in current run
	settings.setValue("profiles", QVariant::fromValue(data.profiles));

	if (usingProfile)
		settings.setValue("currentProfile", data.currentProfile.value());
}

SettingsData Settings::setAssignableSetting(SettingsData data, AssignableSetting setting) {
	// Check if there is exising setting for path
	for (auto &assSetting : data.assignableSettings) {
		if (assSetting.assignablePath == setting.assignablePath) {
			assSetting.value = setting.value;
			return data;
		}
	}
	data.assignableSettings.append(setting);
	return data;
}

QVector<AssignableSetting> Settings::readAssignableSettings(QString profile) {
	QVector<AssignableSetting> retval;
	QSettings s{"tuxclocker"};
	s.beginGroup(QString{"profiles/%1"}.arg(profile));

	auto keys = s.allKeys();
	for (auto &key : keys) {
		AssignableSetting setting{
		    .assignablePath = Utils::fromSettingsPath(key),
		    .value = s.value(key),
		};
		retval.append(setting);
	}
	s.endGroup();
	return retval;
}

SettingsData Settings::readSettings() {
	qRegisterMetaTypeStreamOperators<QVector<QString>>("QVector<QString>>");
	qRegisterMetaTypeStreamOperators<QVector<QPointF>>("QVector<QPointF>");
	qRegisterMetaTypeStreamOperators<DynamicReadableConnectionData>(
	    "DynamicReadableConnectionData");
	QSettings s{"tuxclocker"};

	std::optional<QString> profile;
	QVector<AssignableSetting> assignableSettings;
	auto currentProfile = s.value("currentProfile");
	if (s.value("usingProfile").toBool() && currentProfile.isValid()) {
		auto profileStr = s.value("currentProfile").toString();
		profile = profileStr;
		// Read possible assignable settings
		assignableSettings = readAssignableSettings(profileStr);
	} else
		profile = std::nullopt;

	auto profiles = qvariant_cast<QVector<QString>>(s.value("profiles"));

	return SettingsData{
	    .autoApplyProfile = s.value("autoApplyProfile").toBool(),
	    .currentProfile = profile,
	    .assignableSettings = assignableSettings,
	    .profiles = profiles,
	};
}
