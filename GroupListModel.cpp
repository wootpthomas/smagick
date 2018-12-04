#include "GroupListModel.h"

/* Roles
Qt::DisplayRole		0	The key data to be rendered (usually text).
Qt::DecorationRole	1	The data to be rendered as a decoration (usually an icon).
Qt::EditRole		2	The data in a form suitable for editing in an editor.
Qt::ToolTipRole		3	The data displayed in the item's tooltip.
Qt::StatusTipRole	4	The data displayed in the status bar.
Qt::WhatsThisRole	5	The data displayed for the item in "What's This?" mode.
Qt::SizeHintRole	13	The size hint for the item that will be supplied to views.
*/

GroupListModel::GroupListModel(QObject *parent)
: QAbstractListModel(parent)
{}

GroupListModel::~GroupListModel()
{}

int GroupListModel::rowCount(const QModelIndex &index) const
{
	if ( ! index.isValid())
		return groupList.count();	

	return 0;
}

Qt::ItemFlags GroupListModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return (Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant GroupListModel::data(const QModelIndex &index, int role) const
{
	QString temp;
	if ( ! index.isValid())
		return QVariant();

	int row = index.row();
	if ( row < 0 || row >= groupList.count() ) //Is the row invalid?
		return QVariant();
	
	if (role == Qt::DisplayRole)
	{
		if( !groupList[row].groupName.isEmpty() ) {
			return groupList[row].groupName;
		}
		return "";
	}
	else if (role == Qt::EditRole)
	{
		return qVariantFromValue<GroupDetails>(groupList[row]);
	}
	else
		return QVariant();
}



bool GroupListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
	{
		int row = index.row();
		//Lets cast our QVariant so we can get the data out
		
		GroupDetails newData = value.value<GroupDetails>();

		//get a pointer to the list item so we can change its contents
		groupList[row] = newData;

		emit dataChanged( index, index);
		return true;
	}
	return false;
}


bool GroupListModel::insertRows(int row, int count, const QModelIndex &index)
{
	beginInsertRows(QModelIndex(), row, row +count-1);

	//Let's add a new connectionData item to our list
	GroupDetails newItem;
	groupList.insert(row, newItem);

	endInsertRows();
#ifdef __DEBUG
	printf("There are now %d items in the list\n", groupList.count() );
#endif
	return true;
}

bool GroupListModel::removeRows(int row, int count, const QModelIndex &index)
{
	if( count == 0 )
		return false;

	beginRemoveRows(QModelIndex(), row, row+count-1);

	groupList.removeAt(row);
	
	endRemoveRows();
#ifdef __DEBUG
	printf("There are now %d items in the list\n", groupList.count() );
#endif
	return true;
}

bool GroupListModel::removeRow(const QModelIndex &index)
{
	if ( ! index.isValid())
		return false;

	beginRemoveRows(QModelIndex(), index.row(), index.row());

	groupList.removeAt(index.row());
	
	endRemoveRows();
	printf("There are now %d items in the list\n", groupList.count() );
	return true;	
}



