#pragma once

#include <optional>
#include <QVariant>
#include <QWidget>

class QCheckBox;
class QListWidget;

// TODO: duplicate definition
struct AssignableSetting {
	QString assignablePath;
	QVariant value;
};

struct SettingsData {
	bool autoApplyProfile;
	std::optional<QString> currentProfile;
	QVector<AssignableSetting> assignableSettings;
	// Only needed for interaction with settings file
	QVector<QString> profiles;
};

class Settings : public QWidget {
public:
	explicit Settings(QWidget *parent = nullptr);

	static SettingsData readSettings();
	static SettingsData setAssignableSetting(SettingsData, AssignableSetting);
signals:
	void cancelled();
	void settingsSaved(SettingsData);
private:
	static QVector<AssignableSetting> readAssignableSettings(QString profile);
	SettingsData fromUIState();
	void writeSettings(SettingsData);
	void setUIState(SettingsData);

	QCheckBox *m_autoLoad;
	QCheckBox *m_useProfile;
	QListWidget *m_profileView;

	Q_OBJECT
};
