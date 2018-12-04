#pragma once

/***********************
 Qt Includes
***********************/
#include <QAction>

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


class ConnectionAction : public QAction {
	Q_OBJECT

public:

	ConnectionAction(const ConnectionDetails &CD, QObject *in_parent)
		: QAction(in_parent)
	{
		actionDetails = new ConnectionDetails();
		this->setObjectName(QString("action_storedConnection") + CD.connectionID);
		this->setText(CD.username + QString("@") + CD.hostAddress);
		*actionDetails = CD;
		connect( this, SIGNAL( triggered() ),
			this, SLOT( slotTriggered() ) );
		
	}

	~ConnectionAction() {
		if(actionDetails)
			delete actionDetails;
	}

signals:
	void triggeredDetails(ConnectionDetails *CD);

private slots:
	void slotTriggered() {
		ConnectionDetails *tempDetails = new ConnectionDetails();  //this object is deleted by the sshtab
		*tempDetails = *actionDetails;
		emit triggeredDetails(tempDetails);
	}
private:
	ConnectionDetails *actionDetails;
};


class GroupAction : public QAction {
	Q_OBJECT
public:
	GroupAction(const GroupDetails &GD, QObject *in_parent)
		: QAction(in_parent)
	{
		actionDetails = new GroupDetails();
		this->setObjectName(QString("action_storedGroup") + QString::number(GD.groupID));
		this->setText("Open all in " + GD.groupName);
		*actionDetails = GD;
		connect( this, SIGNAL( triggered() ),
			this, SLOT( slotTriggered() ) );

	}

	~GroupAction() {
		if(actionDetails)
			delete actionDetails;
	}

signals:
	void triggeredDetails(GroupDetails *GD);

private slots:
	void slotTriggered() {
		GroupDetails *tempDetails = new GroupDetails();
		*tempDetails = *actionDetails;
		emit triggeredDetails(tempDetails);
	}
private:
	GroupDetails *actionDetails;
};