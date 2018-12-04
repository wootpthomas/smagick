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
#include "ConnectionListModel.h"


/* Roles
Qt::DisplayRole		0	The key data to be rendered (usually text).
Qt::DecorationRole	1	The data to be rendered as a decoration (usually an icon).
Qt::EditRole		2	The data in a form suitable for editing in an editor.
Qt::ToolTipRole		3	The data displayed in the item's tooltip.
Qt::StatusTipRole	4	The data displayed in the status bar.
Qt::WhatsThisRole	5	The data displayed for the item in "What's This?" mode.
Qt::SizeHintRole	13	The size hint for the item that will be supplied to views.
*/



ConnectionListModel::ConnectionListModel(QObject *parent)
 : QAbstractListModel(parent)
{
}


ConnectionListModel::~ConnectionListModel()
{
}

int ConnectionListModel::rowCount(const QModelIndex &index) const
{
	if ( ! index.isValid())
		return connectionList.count();
	
	return 0;
}

Qt::ItemFlags ConnectionListModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return ( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

QVariant ConnectionListModel::data(const QModelIndex &index, int role) const
{
	QString temp;
	if ( ! index.isValid() )
		return QVariant();

	int row = index.row();
	if ( row < 0 || row >= connectionList.count() ) //Is the row invalid?
		return QVariant();
	
	if (role == Qt::DisplayRole)
	{
		temp = connectionList[row].username + "@" + connectionList[row].hostAddress;
		if( connectionList[row].hostPort != 22) {
			temp += ":" + QString::number(connectionList[row].hostPort);
		}
		switch( index.column() ){
			case 0: return temp;
			case 1: return connectionList[row].hostAddress;
			case 2: return connectionList[row].username;
			case 3: return connectionList[row].password;
			case 4: return connectionList[row].hostPort;
			default: return temp;
		}
	}
	else if (role == Qt::EditRole)
	{
		return qVariantFromValue<ConnectionDetails>(connectionList[row]);
	}
	else
		return QVariant();
}



bool ConnectionListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
	{
		int row = index.row();
		//Lets cast our QVariant so we can get the data out
		
		ConnectionDetails newData = value.value<ConnectionDetails>();

		//get a pointer to the list item so we can change its contents
		connectionList[row] = newData;

		emit dataChanged( index, index);
		return true;
	}
	return false;
}


bool ConnectionListModel::insertRows(int row, int count, const QModelIndex &index)
{
	beginInsertRows(QModelIndex(), row, row +count-1);

	//Let's add a new connectionData item to our list

	for( int i = row; i < count ; i++ )
	{
		ConnectionDetails newItem;
		connectionList.insert(i, newItem);
	}

	endInsertRows();
	printf("There are now %d items in the list\n", connectionList.count() );
	return true;
}

bool ConnectionListModel::removeRows(int row, int count, const QModelIndex &index)
{
	if( count == 0 ) 
		return false;

	beginRemoveRows(QModelIndex(), row, row+count-1);

	for( int i = row ; i < count ; i++ )
	{
		connectionList.removeAt(row);
	}
	
	endRemoveRows();
	printf("There are now %d items in the list\n", connectionList.count() );
	return true;
}

bool ConnectionListModel::removeRow(const QModelIndex &index)
{
	if ( ! index.isValid())
		return false;

	beginRemoveRows(QModelIndex(), index.row(), index.row());

	connectionList.removeAt(index.row());
	
	endRemoveRows();
	printf("There are now %d items in the list\n", connectionList.count() );
	return true;	
}



