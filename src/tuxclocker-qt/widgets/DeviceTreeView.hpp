#pragma once

#include <AssignableItemData.hpp>
#include <AssignableProxy.hpp>
#include <boost/signals2.hpp>
#include <Device.hpp>
#include <DeviceModel.hpp>
#include <DeviceModelDelegate.hpp>
#include <QMenu>
#include <QTreeView>
#include <QWidgetAction>

// Class for handling menus on DeviceModel
class DeviceTreeView : public QTreeView {
public:
	DeviceTreeView(QWidget *parent = nullptr);
	void setModel(QAbstractItemModel *) override;
	// Accessor method for connecting everything in the browser
	// const DeviceModel &deviceModel() {return m_deviceModel;}
	// TODO: make this more generalized
	// Defers the complexity to DeviceBrowser
	// TODO: this can be handled in the delegate with QAbstractItemDelegate::editorEvent
	boost::signals2::signal<void(QModelIndex &)> functionEditorRequested;
protected:
	bool edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger,
	    QEvent *event) override {
		// The editor value is applied to all applicable indices from these
		m_editSelection = selectedIndexes();
		// Used as the baseline
		m_editedIndex = index;

		// Don't wait for double click
		return QTreeView::edit(index,
		    trigger == QAbstractItemView::SelectedClicked
			? QAbstractItemView::AllEditTriggers
			: trigger,
		    event);
	}
	// TODO: allow to start editing with the keyboard
	EditTriggers editTriggers() { return QAbstractItemView::AllEditTriggers; }
	void dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &) override;
private:
	void saveCollapsed(QAbstractItemModel *model);
	void restoreCollapsed(QAbstractItemModel *model);
	// Suspend/resume readable updates
	void suspendChildren(const QModelIndex &);
	void resumeChildren(const QModelIndex &);

	// DeviceModel &m_deviceModel;
	QModelIndexList m_editSelection;
	QModelIndex m_editedIndex;
	DeviceModelDelegate *m_delegate;
};
