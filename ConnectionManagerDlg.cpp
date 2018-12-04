

#include "MainWindowDlg.h"
#include "ConnectionManager.h"
#include "ConnectionManagerDlg.h"
#include "DataTypes.h"


ConnectionManagerDlg::ConnectionManagerDlg(MainWindowDlg *parent, ConnectionManager * const pConnectionMgr) :
	QDialog(parent),
	m_pConnManager(pConnectionMgr),
	m_pConnectionModel(NULL)
{
	setupUi(this);
	
	init();
	show();
}

ConnectionManagerDlg::~ConnectionManagerDlg() 
{
	if(m_pConnectionModel)
		delete m_pConnectionModel;
	if(m_pConnectionSelectionModel)
		delete m_pConnectionSelectionModel;
	if(m_pConnectionsInGroupModel)
		delete m_pConnectionsInGroupModel;
	if(m_pConnectionsInGroupSelectionModel)
		delete m_pConnectionsInGroupSelectionModel;
	if(m_pConnectionsAvailableToGroupSelectionModel)
		delete m_pConnectionsAvailableToGroupSelectionModel;
	//we don't delete the connection manager, we don't own it....
}

void ConnectionManagerDlg::init() 
{
	m_pConnectionModel = new ConnectionListModel();
	m_pConnectionsInGroupModel = new ConnectionListModel();

	//setup model/view for the list of individual connections
	m_LstConnections->setModel(m_pConnectionModel);
	m_pConnectionSelectionModel = m_LstConnections->selectionModel();

	//setup model/view for the available groups
	m_LstAvailableConnections->setModel( m_pConnectionModel );
	m_pConnectionsAvailableToGroupSelectionModel = m_LstAvailableConnections->selectionModel();

	//setup model/view for connections in group
	m_LstConsInGroup->setModel( m_pConnectionsInGroupModel );
	m_pConnectionsInGroupSelectionModel = m_LstConsInGroup->selectionModel();

	m_HostPort->setValidator(new QIntValidator(1,65535, this));

	//throw the connectionDetails into the model
	populateConnectionList(m_pConnectionModel, m_pConnectionSelectionModel);

	//throw the groupDetails into the model
	populateGroupName();
	
	//setup signals and slots
	connect( m_OK, SIGNAL( clicked() ),
		this, SLOT( slotOKClicked() ));

	//Signals and slots for the individual connection stuff
	connect( m_pConnectionSelectionModel, SIGNAL( selectionChanged(const QItemSelection &, const QItemSelection &) ),
		this, SLOT( slotSelectConnection(const QItemSelection &, const QItemSelection &) ));
	connect( m_NewConnection, SIGNAL( clicked() ),
		this, SLOT( slotAddNewConnection() ));
	connect( m_DeleteConnection, SIGNAL( clicked() ),
		this, SLOT( slotRemoveConnection() ));
	connect( m_HostAddress, SIGNAL( textEdited(const QString &) ),
		this, SLOT( slotConnectionInformationChanged(const QString &) ));
	connect( m_UserName, SIGNAL( textEdited(const QString &) ),
		this, SLOT( slotConnectionInformationChanged(const QString &) ));
	connect( m_Password, SIGNAL( textEdited(const QString &) ),
		this, SLOT( slotConnectionInformationChanged(const QString &) ));
	connect( m_PrivateKey, SIGNAL( textEdited(const QString &) ),
		this, SLOT( slotConnectionInformationChanged(const QString &) ));
	connect( m_HostPort, SIGNAL( textEdited(const QString &) ),
		this, SLOT( slotConnectionInformationChanged(const QString &) ));

	//Signals and slots for the group stuff
	connect (m_GroupName, SIGNAL( editTextChanged(const QString &) ),
		this, SLOT( slotSelectGroup(const QString &) ));
	connect (m_NewGroup, SIGNAL( clicked() ),
		this, SLOT( slotAddNewGroup() ));
	connect (m_DeleteGroup, SIGNAL( clicked() ),
		this, SLOT( slotRemoveGroup() ));
	connect (m_AddToGroup, SIGNAL( clicked() ),
		this, SLOT( slotAddToGroup() ));
	connect (m_LstAvailableConnections, SIGNAL( doubleClicked(const QModelIndex &) ),
		this, SLOT( slotAddToGroup() ));
	connect (m_RemoveFromGroup, SIGNAL( clicked() ),
		this, SLOT( slotRemoveFromGroup() ));
	connect (m_LstConsInGroup, SIGNAL( doubleClicked(const QModelIndex &) ),
		this, SLOT( slotRemoveFromGroup() ));
}

void ConnectionManagerDlg::slotOKClicked() 
{
	m_pConnectionSelectionModel->clear();
	this->close();
}

void ConnectionManagerDlg::slotAddNewConnection()
{
	QVariant tempVariant;
	ConnectionDetails temp = ConnectionDetails::ConnectionDetails();
	temp.hostAddress = "Host";
	temp.username = "Username";
	m_pConnectionModel->insertRows(0, 1);
	tempVariant = qVariantFromValue<ConnectionDetails>(temp);
	m_pConnectionModel->setData(m_pConnectionModel->index(0), tempVariant);
	m_pConnectionSelectionModel->clear();
	m_pConnectionSelectionModel->select( m_pConnectionModel->index(0), QItemSelectionModel::SelectCurrent);
	m_HostAddress->setFocus();
	m_HostAddress->selectAll();
}

void ConnectionManagerDlg::populateConnectionList(ConnectionListModel *model, QItemSelectionModel *selectionModel) 
{
	ConnectionDetails temp;
	QVariant tempVariant;
	QList<ConnectionDetails> myList = m_pConnManager->getConnectionDetails();
	foreach(temp, myList) {
		model->insertRows(0, 1);
		tempVariant = qVariantFromValue<ConnectionDetails>(temp);
		model->setData(model->index(0), tempVariant);
		selectionModel->clear();
	}
}

void ConnectionManagerDlg::populateGroupName()
{
	m_GroupName->clear();
	GroupDetails temp;
	QVariant tempVariant;
	QList<GroupDetails> myGroupList = m_pConnManager->getGroupDetails();
	foreach(temp, myGroupList) 
	{
		tempVariant = qVariantFromValue<GroupDetails>(temp);
		m_GroupName->insertItem(0, temp.groupName, tempVariant);
		m_GroupName->setCurrentIndex(-1);
	}
}


void ConnectionManagerDlg::slotSelectGroup(const QString &text) 
{
	//setup variables to get the connections that are in the selected group
	QList<ConnectionDetails> connectionList;
	ConnectionDetails tempConnection;
	GroupDetails tempGroup;
	QVariant tempVariant;
	int indexOfComboBox;
	bool isSelectionValid = false;

	indexOfComboBox = m_GroupName->findText(text);
	if( indexOfComboBox == -1 )
	{
		//the text wasn't found the user is typing something unique.
		if( m_GroupName->currentIndex() != -1 )
		{
			//we're renaming an existing one.
			tempGroup = m_GroupName->itemData( m_GroupName->currentIndex() ).value<GroupDetails>();
			tempGroup.groupName = text;
			tempVariant = qVariantFromValue<GroupDetails>(tempGroup);
			if( m_pConnManager->updateGroupDetails( tempGroup.groupID, tempGroup ) )
			{
				m_GroupName->setItemData( m_GroupName->currentIndex(), tempVariant );
				m_GroupName->setItemText( m_GroupName->currentIndex(), tempGroup.groupName );
			}
			isSelectionValid = true;
		}
	} else {
		//the data is valid populate the other information.

		//get the details of the selected group
		GroupDetails selectedGroupDetails = m_GroupName->itemData( indexOfComboBox).value<GroupDetails>();

		//get the connections in the selected group
		connectionList = m_pConnManager->getConnectionDetails( QString::number(selectedGroupDetails.groupID) );
		
		//loop through the connections in the group
		m_pConnectionsInGroupModel->removeRows(0, m_pConnectionsInGroupModel->rowCount());
		foreach(tempConnection, connectionList)
		{
			m_pConnectionsInGroupModel->insertRows(0, 1);
			tempVariant = qVariantFromValue<ConnectionDetails>(tempConnection);
			m_pConnectionsInGroupModel->setData( m_pConnectionsInGroupModel->index(0), tempVariant );
			m_pConnectionsInGroupSelectionModel->clear();
		}
		isSelectionValid = true;
	}
	//something happend to the group selection now set the enabled states of everything depending on isSelectionValid
	m_AddToGroup->setEnabled(isSelectionValid);
	m_RemoveFromGroup->setEnabled(isSelectionValid);
	m_DeleteGroup->setEnabled(isSelectionValid);
	m_LstAvailableConnections->setEnabled(isSelectionValid);
	m_LstConsInGroup->setEnabled(isSelectionValid);
}

void ConnectionManagerDlg::slotSelectConnection(const QItemSelection &current, const QItemSelection &previous)
{
	QModelIndexList indexes = current.indexes();
	QModelIndexList oldIndexes = previous.indexes();
	int countOfSelection = m_pConnectionSelectionModel->selectedIndexes().count();
	if( current == previous )
		return;

	ConnectionDetails temp;
	QModelIndex selectedIndex;
	if( !previous.isEmpty() )
	{
		foreach(selectedIndex, previous.indexes() ) {
			temp = m_pConnectionModel->data(selectedIndex, Qt::EditRole).value<ConnectionDetails>();
			if( temp.isDirty )
			{
				if( m_pConnManager->updateConnectionDetails(temp.connectionID, &temp) )
				{
					m_pConnectionModel->setData(selectedIndex, qVariantFromValue(temp));
				}
			}
		}
	}
	
	if( countOfSelection == 1 )
	{
		//only one selected this is the ideal state
		this->m_HostAddress->setEnabled(true);
		this->m_HostPort->setEnabled(true);
		this->m_UserName->setEnabled(true);
		this->m_Password->setEnabled(true);
		this->m_PrivateKey->setEnabled(true);
		this->m_Advanced->setEnabled(true);

		this->m_DeleteConnection->setEnabled(true);
		
		//Set the values of the gui fields
		temp = m_pConnectionModel->data( m_pConnectionSelectionModel->selectedIndexes().at(0), Qt::EditRole).value<ConnectionDetails>();
		this->m_HostAddress->setText(temp.hostAddress);
		this->m_HostPort->setText(QString::number(temp.hostPort));
		this->m_UserName->setText(temp.username);
		this->m_Password->setText(temp.password);
		this->m_PrivateKey->setText(temp.RSA_privateKey);
	} else {
		//current selection is undisplayable set the edit fields accordingly
		m_HostAddress->setEnabled(false);
		m_HostPort->setEnabled(false);
		m_UserName->setEnabled(false);
		m_Password->setEnabled(false);
		m_PrivateKey->setEnabled(false);
		m_Advanced->setEnabled(false);

		m_HostAddress->setText("");
		m_HostPort->setText("");
		m_UserName->setText("");
		m_Password->setText("");
		m_PrivateKey->setText("");

		if( countOfSelection > 1 ) 
		{
			//do specific things for the case multiple are selected
			this->m_DeleteConnection->setEnabled(true);
		} else {
			// do specific things for the case none are selected.
			this->m_DeleteConnection->setEnabled(false);
		}
	}
}

void ConnectionManagerDlg::slotConnectionInformationChanged(const QString & text)
{
	QModelIndexList selected = this->m_pConnectionSelectionModel->selectedIndexes();
	ConnectionDetails temp;
	QVariant tempVariant;

	if( selected.count() == 1 )
	{
		//ok lets put the changed stuff into the ConnectionDetails object of the model
		QString hostName = this->m_HostAddress->text();
		int hostPort = this->m_HostPort->text().toInt();
		QString userName = this->m_UserName->text();
		QString password = this->m_Password->text();
		QString privateKey = this->m_PrivateKey->text();

		temp = m_pConnectionModel->data( selected.at(0), Qt::EditRole ).value<ConnectionDetails>();
		
		int connectionID = temp.connectionID;
		temp.hostAddress = hostName;
		temp.hostPort = hostPort;
		temp.username = userName;
		temp.password = password;
		temp.RSA_privateKey = privateKey;
		temp.isDirty = true;

		tempVariant = qVariantFromValue<ConnectionDetails>(temp);
		m_pConnectionModel->setData( selected.at(0), tempVariant);
	}
}

void ConnectionManagerDlg::slotRemoveFromGroup()
{
	QModelIndexList selected = m_pConnectionsInGroupSelectionModel->selectedIndexes();
	int groupID = m_GroupName->itemData(m_GroupName->currentIndex()).value<GroupDetails>().groupID;
	int selectedCount = selected.count();
	QModelIndex index;
	QVariant tempVariant;

	ConnectionDetails temp;

	foreach(index, selected)
	{
		temp = m_pConnectionsInGroupModel->data(index, Qt::EditRole).value<ConnectionDetails>();
		if( m_pConnManager->removeConnectionFromGroup(groupID, temp.connectionID ) )
		{
			m_pConnectionsInGroupModel->removeRow(index);
		}
	}
	m_pConnectionsInGroupSelectionModel->clear();
}

void ConnectionManagerDlg::slotAddToGroup()
{
	QModelIndexList selected = m_pConnectionsAvailableToGroupSelectionModel->selectedIndexes();
	int selectedCount = selected.count();
	int currentGroupIndex = m_GroupName->currentIndex();
	if( currentGroupIndex < 0 )
		return;   //no group was selected.

	int groupID = m_GroupName->itemData(m_GroupName->currentIndex()).value<GroupDetails>().groupID;
	QModelIndex index;
	QVariant tempVariant;

	ConnectionDetails temp;
	
	foreach(index, selected)
	{
		temp = m_pConnectionModel->data(index, Qt::EditRole).value<ConnectionDetails>();
		if( m_pConnManager->addConnectionToGroup(groupID, temp.connectionID) )
		{
			m_pConnectionsInGroupModel->insertRows(0, 1);
			tempVariant = qVariantFromValue<ConnectionDetails>(temp);
			m_pConnectionsInGroupModel->setData(m_pConnectionsInGroupModel->index(0), tempVariant);
			m_pConnectionsInGroupSelectionModel->clear();
		}
	}
}

void ConnectionManagerDlg::slotAddNewGroup()
{
	//first we call connection manager and get a new group
	GroupDetails newGroup;
	if(	m_pConnManager->getNewGroup(newGroup) )
	{
		//then we repopulate the groupName dropDown
		populateGroupName();
		//then we select this new group
		m_GroupName->setCurrentIndex( m_GroupName->findText(newGroup.groupName) );
		m_GroupName->setFocus();
	}
}

void ConnectionManagerDlg::slotRemoveConnection()
{
	QModelIndexList selectionList = m_pConnectionSelectionModel->selectedIndexes();
	QModelIndex currentSelection;
	ConnectionDetails connectionToRemove;

	if( selectionList.count() == 1 )
	{
		currentSelection = selectionList.at(0);
		connectionToRemove = m_pConnectionModel->data(currentSelection, Qt::EditRole).value<ConnectionDetails>();
		if( m_pConnManager->removeConnection(connectionToRemove.connectionID) )
		{
			m_pConnectionSelectionModel->clear();
			m_pConnectionModel->removeRow(currentSelection);
		}
	}
}

void ConnectionManagerDlg::slotRemoveGroup()
{
	//TODO: finish this
	GroupDetails temp = m_GroupName->itemData( m_GroupName->currentIndex() ).value<GroupDetails>();
	if( m_pConnManager->removeGroup(temp) )
	{
		m_GroupName->removeItem( m_GroupName->currentIndex() );
	}
}

//TODO: add the functionality that it prepopulates the hostname and username fields when you make a new connection
