#pragma once

#include <QMenu>
#include <QStyledItemDelegate>

class AssignableItemData;

// TODO: align checkbox to the right
class DeviceModelDelegate : public QStyledItemDelegate {
public:
	DeviceModelDelegate(QObject *parent = nullptr);
	QWidget *createEditor(
	    QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void updateEditorGeometry(
	    QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(
	    QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	bool editorEvent(QEvent *, QAbstractItemModel *, const QStyleOptionViewItem &,
	    const QModelIndex &) override;
protected:
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
	    const QModelIndex &index) const override;
private:
	void commitAndClose();
	template <typename T>
	void setAssignableData(QAbstractItemModel *, const QModelIndex &, QString text, T data);

	QAction *m_parametrize;
	QMenu m_menu;
};
