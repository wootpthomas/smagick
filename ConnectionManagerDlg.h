#pragma once


/***********************
 Qt Includes
***********************/
#include <QDialog>
#include <QString>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QList>
#include <QListView>
#include <QAbstractListModel>
#include <QVariant>
#include <QModelIndexList>


/***********************
 Includes
***********************/
#include "ui_ConnectionManager.h"
#include "ConnectionManager.h"
#include "ConnectionListModel.h"
#include "MainWindowDlg.h"
#include "GroupListModel.h"


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


class ConnectionManagerDlg : public QDialog, public Ui_ConnectionManager 
{
	Q_OBJECT
public:
	ConnectionManagerDlg(MainWindowDlg *parent, ConnectionManager * const pConnectionMgr);
	~ConnectionManagerDlg(void);
public slots:
	void slotOKClicked();

	//slots for the actions of selectiong groups or connections
	void slotSelectGroup(const QString &text);
	void slotSelectConnection(const QItemSelection &current, const QItemSelection &previous);

	void slotConnectionInformationChanged(const QString & text);

	//slots for adding/removing connections to a group
	void slotRemoveFromGroup();
	void slotAddToGroup();

	//slots to add new connections and remove old connections
	void slotAddNewConnection();
	void slotRemoveConnection();

	//slots to add new groups and remove old groups
	void slotAddNewGroup();
	void slotRemoveGroup();
private:
	void init();
	void queryConnectionsAvailableToGroup();
	void queryConnectoinsInGroup();

	void populateConnectionList(ConnectionListModel *model, QItemSelectionModel *selectionModel);
	void populateGroupName();

	ConnectionManager * const m_pConnManager;

	//model and selection model for the individual connecitons
	ConnectionListModel *m_pConnectionModel;
	QItemSelectionModel *m_pConnectionSelectionModel;

	//model and selection model for the connection in a selected group
	ConnectionListModel *m_pConnectionsInGroupModel;
	QItemSelectionModel *m_pConnectionsInGroupSelectionModel;

	//model and selection model for the connections available to a group
	QItemSelectionModel *m_pConnectionsAvailableToGroupSelectionModel;
};
