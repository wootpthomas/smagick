#pragma once

/***********************
 Qt Includes
***********************/
#include <QAbstractListModel>

/***********************
 Includes
***********************/
#include "DataTypes.h"


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


class GroupListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	GroupListModel(QObject *parent = 0);
	~GroupListModel();

	//Required item to make this work
	int rowCount(const QModelIndex &index = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	//required items to be able to edit items
	bool setData(const QModelIndex &index, const QVariant &value, int rolw = Qt::DisplayRole);
	Qt::ItemFlags flags(const QModelIndex &index) const;

	//required to be able to resize the model (add & remove)
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
	bool removeRow(const QModelIndex &index);

private:
	QList<GroupDetails> groupList;
};
