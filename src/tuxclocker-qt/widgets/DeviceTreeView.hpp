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
	// Accessor method for connecting everything in the browser
	//const DeviceModel &deviceModel() {return m_deviceModel;}
	// TODO: make this more generalized
	// Defers the complexity to DeviceBrowser
	boost::signals2::signal<void(QModelIndex&)> functionEditorRequested;
protected:
	/* Workaround for the retarded behavior of waiting for a double click,
	   you can't even disable it! */
    bool edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger,
			  QEvent *event) {
        return QTreeView::edit(index,
			trigger == QAbstractItemView::SelectedClicked ?
			QAbstractItemView::AllEditTriggers : trigger, event);
    }
private:
	//DeviceModel &m_deviceModel;
	DeviceModelDelegate *m_delegate;
};
