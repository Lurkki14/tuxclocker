#pragma once

#include <optional>
#include <QVariant>
#include <QWidget>

// TODO: duplicate definition
struct AssignableSetting {
	QString assignablePath;
	QVariant value;
};

struct SettingsData {
	bool autoApplyProfile;
	std::optional<QString> currentProfile;
	QVector<AssignableSetting> assignableSettings;
};

class Settings : public QWidget {
public:
	explicit Settings(QWidget *parent = nullptr);
signals:
	void cancelled();
private:
	Q_OBJECT
};
