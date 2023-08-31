#pragma once

#include <QMenu>
#include <QStyledItemDelegate>

class AssignableItemData;
class FunctionEditor;
class QStandardItemModel;

// TODO: align checkbox to the right
class DeviceModelDelegate : public QStyledItemDelegate {
public:
	DeviceModelDelegate(QObject *parent = nullptr);
	~DeviceModelDelegate();
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
	static void setAssignableData(
	    QAbstractItemModel *, const QModelIndex &, QString text, T data);
	// Whether to show reset action for a node
	bool subtreeHasAssignableDefaults(QAbstractItemModel *, const QModelIndex &);

	FunctionEditor *m_functionEditor;
	QAction *m_parametrize;
	QAction *m_resetAssignable;
	QMenu m_menu;
};
