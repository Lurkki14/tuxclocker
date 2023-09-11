#include "Settings.hpp"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

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

	auto autoLoadCheck = new QCheckBox{"Apply profile settings automatically", this};

	auto profileCheck = new QCheckBox{"Use profile", this};

	// TODO: add delegate to make deleting a little nicer
	auto profileView = new QListWidget{this};
	auto triggers = QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed;
	profileView->setEditTriggers(triggers);
	profileView->setEnabled(false);

	auto addButton = new QPushButton{"Add profile"};
	addButton->setEnabled(false);

	auto removeButton = new QPushButton{"Remove selected"};

	connect(addButton, &QPushButton::released, [=] {
		auto item = new QListWidgetItem{"Unnamed"};
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		profileView->addItem(item);
	});

	connect(removeButton, &QPushButton::released,
	    [=] { profileView->model()->removeRow(profileView->currentRow()); });

	connect(profileCheck, &QCheckBox::stateChanged, [=](auto state) {
		bool enable = (state == Qt::Unchecked) ? false : true;

		addButton->setEnabled(enable);
		profileView->setEnabled(enable);
	});

	auto cancelButton = new QPushButton{"Cancel", this};

	connect(cancelButton, &QPushButton::released, this, &Settings::cancelled);

	auto saveButton = new QPushButton{"Save", this};

	layout->addWidget(label, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop);
	layout->addWidget(autoLoadCheck, 1, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(profileCheck, 2, 0, 1, 1, Qt::AlignLeft);
	layout->addWidget(profileView, 2, 1, 1, 2);
	layout->addWidget(addButton, 3, 1);
	layout->addWidget(removeButton, 3, 2);
	layout->addWidget(cancelButton, 4, 0, 1, 1, Qt::AlignBottom);
	layout->addWidget(saveButton, 4, 1, 1, 2, Qt::AlignBottom);

	this->setLayout(layout);
}
