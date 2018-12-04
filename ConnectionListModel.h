/***************************************************************************
 *   Copyright (C) 2007 by Paul Thomas   *
 *   thomaspu@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef CONNECTIONLISTMODEL_H
#define CONNECTIONLISTMODEL_H


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


class ConnectionListModel : public QAbstractListModel
{
	Q_OBJECT

public:
		
    ConnectionListModel(QObject *parent = 0);
    ~ConnectionListModel();

	 //Required items to make this model work
	 int rowCount(const QModelIndex &index = QModelIndex()) const;
	 QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	 //required items to be able to edit items
	 bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole);
	 Qt::ItemFlags flags(const QModelIndex &index) const;

	 //Required to be able to resize the model (add & remove shit)
	 bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	 bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
	 bool removeRow(const QModelIndex &index);
	 
	private:
		QList<ConnectionDetails> connectionList;
};


#endif
