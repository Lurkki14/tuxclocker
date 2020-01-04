#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>
#include <QDebug>
#include <tc_module.h>

#include "AssignableAdaptorFactory.h"
#include "EnumData.h"
#include "Result.h"
#include "ReadableAdaptorFactory.h"

void registerMetatypes() {
	//qRegisterMetaType<Result<QDBusVariant>>();
	qDBusRegisterMetaType<Result<QDBusVariant>>();
	
	//qRegisterMetaType<EnumData>();
	qDBusRegisterMetaType<EnumData>();
	
	qDBusRegisterMetaType<QList<EnumData>>();
}

void registerReadables(QObject *parent, QDBusConnection conn) {
	uint16_t count;
	auto modules = tc_module_find_all_from_category(TC_CATEGORY_READABLE, &count);
	
	QList <tc_readable_module_data_t> dataList;
	
	for (uint16_t i = 0; i < count; i++) {
		if (modules[i]->init_callback  && modules[i]->init_callback() == TC_SUCCESS) {
			auto &module = modules[i];
			// Module was initialized successfully
			// TODO : make a function in the lib to get a list of category specific data
			for (auto i = 0; i < module->category_info.num_categories; i++) {
				if (module->category_info.category_data_list[i].category == TC_READABLE) {
					dataList.append(module->category_info.category_data_list[i].readable_data());
					break;
				}
			}
		}
	}
	
	std::function<void (tc_readable_node_t*, tc_readable_module_data_t, QString, int)> traverse;
	traverse = [=, &traverse, &conn](tc_readable_node_t *node, tc_readable_module_data_t data, QString path, int n) {
		if (node->name) {
			qDebug() << node->name;
		}
		
		path = path + "/" + QString::number(n);
		qDebug() << path;
		
		auto obj = new QObject(parent);
		//new ReadableDynamicVariantAdaptor(obj);
		ReadableAdaptorFactory::readableAdaptor(obj, node);
		conn.registerObject(path, obj);
		
		for (uint16_t i = 0; i < node->children_count; i++) {
			traverse(node->children_nodes[i], data, path, n + i);
		}
	};
	
	QString p = "/Readable";
	for (auto data : dataList) {
		for (uint16_t i = 0; i < data.root_node->children_count; i++) {
			traverse(data.root_node->children_nodes[i], data, p, 0);
		}
	}
	
	qDebug() << dataList.length();
}

void registerAssignables(QObject *parent, QDBusConnection conn) {
	uint16_t count;
	auto modules = tc_module_find_all_from_category(TC_CATEGORY_ASSIGNABLE, &count);
	
	QList <tc_assignable_module_data_t> dataList;
	
	for (uint16_t i = 0; i < count; i++) {
		if (modules[i]->init_callback && modules[i]->init_callback() == TC_SUCCESS) {
			auto &module = modules[i];
			for (uint16_t i = 0; i < module->category_info.num_categories; i++) {
				if (module->category_info.category_data_list[i].category == TC_ASSIGNABLE) {
					dataList.append(module->category_info.category_data_list[i].assignable_data());
					break;
				}
			}
		}
	}
	
	std::function <void (tc_assignable_node_t*, tc_assignable_module_data_t, QString, int)> traverse;
	traverse = [=, &traverse, &conn](tc_assignable_node_t *node, tc_assignable_module_data_t data, QString path, int n) {
		path = path + "/" + QString::number(n);
		
		auto obj = new QObject(parent);
		AssignableAdaptorFactory::assignableAdaptor(obj, node);
		
		if (!conn.registerObject(path, obj)) {
			qDebug() << "unable to register object";
		}
		
		for (uint16_t i = 0; i < node->children_count; i++) {
			traverse(node->children_nodes[i], data, path, n + i);
		}
	};
	
	QString path ="/Assignable";
	for (auto data : dataList) {
		for (uint16_t i = 0; i < data.root_node->children_count; i++) {
			traverse(data.root_node->children_nodes[i], data, path, 0);
		}
	}
}

int main(int argc, char **argv) {
	QCoreApplication a(argc, argv);
	
	registerMetatypes();
	
	QObject root;
	
	auto connection = QDBusConnection::systemBus();
	registerReadables(&root, connection);
	registerAssignables(&root, connection);
	
	connection.registerObject("/", &root);
	
	if (!connection.registerService("org.tuxclocker")) {
		qDebug() << "unable to register:" << connection.lastError().message();
	}
	
	return a.exec();
}
