/***************************************************************************
 *   Copyright (C) 2008 by Paul Thomas                                     *
 *   thomaspu@gmail.com                                                    *
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

//TODO: http://doc.trolltech.com/4.3/appicon.html


/**********************
 Qt includes
**********************/
#include <QMessageBox>
#include <QLabel>
#include <QAction>
#include <QSettings>
#include <QSize>
#include <QPoint>
#include <QApplication>
#include <QMouseEvent>
#include <QTimer>

/**********************
 Other includes
**********************/
#include <stdlib.h>

#include "MainWindowDlg.h"
#include "ConnectionManager.h"
#include "CreateChangeMasterPasswordDlg.h"
#include "ConnectionManagerDlg.h"
#include "ConnectionAction.h"
#include "Tabbinator.h"
#include "QuickConnectDlg.h"
#include "SSHWidget.h"
#include "UserPreferences.h"
#include "UserPreferencesDlg.h"
#include "DataTypes.h"



/**********************
 Defines
**********************/
#define MAINWINDOWDLG_1 "Current working directory"
#define MAINWINDOWDLG_2 100	//msec timeout for SSH resize update after the window has been resized

#define MAINWINDOWDLG_4 "About Shell Magic"
#define MAINWINDOWDLG_5 "Shell Magic\nCreated by Paul Thomas and Jeremy Combs"

////////////////////////////////////////////////////////////////////
MainWindowDlg::MainWindowDlg(ConnectionManager * const pConnectionManager, int argc, char *argv[]):
mp_connectionMgr(pConnectionManager),
mp_tabbinator(NULL),
mp_SB_currentWorkingDir(NULL),
mp_SB_statusIndicator(NULL),
mp_userPreferencesDlg(NULL),
m_argc(argc),
mp_argv(argv)
{
	setupUi(this);

	createActions();
	createMenus();
	createStatusBar();

	init();
	show();

	reopenPreviousTabs();
}

////////////////////////////////////////////////////////////////////
MainWindowDlg::~MainWindowDlg(void)
{
	if ( mp_userPrefs)
	{
		delete mp_userPrefs;
		mp_userPrefs = NULL;
	}

	//Tell our transfer manager that its time to put its toys away and come inside
	if ( mp_transferManager)
	{
		if ( mp_transferManager->isRunning())
		{
			mp_transferManager->quit();
			printf("Waiting on the transfer manager to stop\n");
			mp_transferManager->wait();
			printf("Transfer manager stoped\n");
		}

		delete mp_transferManager;
		mp_transferManager = NULL;
	}

	if ( mp_fileTransferProgressDlg)
	{
		delete mp_fileTransferProgressDlg;
		mp_fileTransferProgressDlg = NULL;
	}

	if ( mp_userPreferencesDlg)
	{
		delete mp_userPreferencesDlg;
		mp_userPreferencesDlg = NULL;
	}
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::init()
{
	//Let's pull in our user preferences
	mp_userPrefs = new UserPreferences();

	//Setup our main tabWidget
	mp_tabbinator = new Tabbinator(this, mp_connectionMgr, mp_userPrefs);
	setCentralWidget(mp_tabbinator);

	//Setup our file Progress dialog so the transferManager can make use of it
	mp_fileTransferProgressDlg = new FileTransferProgressDlg();

	//Create our transfer manager
	mp_transferManager = new TransferManager(mp_fileTransferProgressDlg);
	mp_transferManager->start();

	//IMPORTANT: Let's change the thread affinity to events are handled by its event loop
	mp_transferManager->moveToThread(mp_transferManager);

	//Restore window position and userPreferences
	restoreSettings();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::createActions()
{
	//Connect up the actions in the File menu
	connect( action_Connection_Manager, SIGNAL(triggered()), 
		this, SLOT(slotConnectionManager()));
	connect( action_Exit, SIGNAL(triggered()),
		this, SLOT(slotClose()));
	connect( actionChange_Master_Password, SIGNAL(triggered()),
		this, SLOT( slotChangeMasterPassword()));
	connect( action_Quick_Connect, SIGNAL(triggered()),
		this, SLOT( slotQuickConnect()));
	connect( this->menuEdit, SIGNAL( aboutToShow() ),
		this, SLOT( slotUpdateCopyPasteAvailable()));
	connect( action_Preferences, SIGNAL(triggered()),
		this, SLOT(slotUserPreferences()));
	connect( actionCopy, SIGNAL(triggered()),
		this, SLOT(slotCopy()));
	connect ( action_Paste, SIGNAL(triggered()),
		this, SLOT(slotPaste()));

	//Connect up the actions in the Help Menu
	connect( action_About, SIGNAL(triggered()), this, SLOT(slotAbout()));
	connect( actionAbout_Qt, SIGNAL(triggered()), this, SLOT(slotAboutQt()));

	createConnectionActions();

#ifdef _DEBUG
	mp_actionEnableDebugPrinting = new QAction(this);
    mp_actionEnableDebugPrinting->setObjectName(QString::fromUtf8("action_EnableDebugPrinting"));
	mp_actionEnableDebugPrinting->setText( "&Enable Debug Printing");
	mp_actionEnableDebugPrinting->setCheckable(true);
	mp_actionEnableDebugPrinting->setChecked(true);

	connect( mp_actionEnableDebugPrinting, SIGNAL(toggled(bool)),
		this->mp_tabbinator, SLOT(slotToggleDebugPrinting(bool)));

	mp_actionEnableDebugTooltip = new QAction(this);
	mp_actionEnableDebugTooltip->setObjectName(QString::fromUtf8("action_EnableDebugTooltip"));
	mp_actionEnableDebugTooltip->setText( "Enable &Debug Tooltip" );
	mp_actionEnableDebugTooltip->setCheckable(true);
	mp_actionEnableDebugTooltip->setChecked(true);

	connect( mp_actionEnableDebugTooltip, SIGNAL(toggled(bool)),
		this, SLOT(slotToggleDebugTooltip(bool)));

#endif

}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::createConnectionActions()
{

	QList<QAction *> connectionActions;

	QMenu *tempMenu;
	
	ConnectionAction *tempConnectionAction;
	GroupAction *ConnectToAll;
	QAction *seperator;

	//get the entire list of connections, this variable will be reset eventually
	//this is just to check that connections actually exist in the database
	QList<ConnectionDetails> ConnectionList = mp_connectionMgr->getConnectionDetails();
	
	//now lets make the actions for the stored groups.
	QList<GroupDetails> GroupList = mp_connectionMgr->getGroupDetails();;

	ConnectionDetails tempConnectionDetails;
	GroupDetails tempGroupDetails;

	//lets start by clearing the menu, we are going to compleatly rebuild it...
	menuStored_Connections->clear();
	if( !ConnectionList.isEmpty() )
	{

		//ok now loop through the groups and make menus for each, then put connectionActions into each menu
		foreach( tempGroupDetails, GroupList )
		{
			//make the menu for the group
			tempMenu = new QMenu(tempGroupDetails.groupName, this);

			//get the connections in the group
			ConnectionList = mp_connectionMgr->getConnectionDetails( QString::number(tempGroupDetails.groupID) );
			
			//clear out the connectionActions array and then fill it with the connections
			//in the tempGroupDetails group
			connectionActions.clear();
			
			//put the connect to all link at the top
			ConnectToAll = new GroupAction(tempGroupDetails, this);
			connect( ConnectToAll, SIGNAL(triggeredDetails(GroupDetails *)),
				this, SLOT(openGroup(GroupDetails *)));
			
			connectionActions.append(ConnectToAll);

			//put a seperator beneith that
			seperator = new QAction(this);
			seperator->setSeparator(true);
			connectionActions.append(seperator);

			//loop through the connecitons in a group and put them in connectionActions
			foreach( tempConnectionDetails, ConnectionList )
			{
				tempConnectionAction = new ConnectionAction(tempConnectionDetails, this);
				connect( tempConnectionAction, SIGNAL(triggeredDetails(ConnectionDetails *)),
					this, SLOT(openConnection(ConnectionDetails *)) ); 
				connectionActions.append(tempConnectionAction);
			}
			tempMenu->addActions(connectionActions);
			menuStored_Connections->addMenu(tempMenu);
			connectionActions.clear();
		}
		
		//put a separator between groups and individual connecitons
		seperator = new QAction(this);
		seperator->setSeparator(true);
		connectionActions.append(seperator);

		//now lets make the actions for the stored connections;
		ConnectionList = mp_connectionMgr->getConnectionDetails();
		foreach( tempConnectionDetails, ConnectionList )
		{
			tempConnectionAction = new ConnectionAction(tempConnectionDetails, this);
			
			connect( tempConnectionAction, SIGNAL(triggeredDetails(ConnectionDetails *)),
				this, SLOT(openConnection(ConnectionDetails *)) ); 
			connectionActions.append(tempConnectionAction);
		}

		//connectionActions now holds all the stored connections and stuff
		//now put it onto the menuStored_Connections menu and enable it
		menuStored_Connections->addActions(connectionActions);
		menuStored_Connections->setEnabled(true);
	} else {
		//Their are no stored connections currently so lets disable the menu
		menuStored_Connections->setEnabled(false);
	}
}

////////////////////////////////////////////////////////////////////
void MainWindowDlg::createMenus()
{
#ifdef _DEBUG
	//Add in the menu item for enabling hex printing
	menuEdit->addAction( mp_actionEnableDebugPrinting);
	menuEdit->addAction( mp_actionEnableDebugTooltip);
#endif
}

////////////////////////////////////////////////////////////////////
void MainWindowDlg::slotUpdateCopyPasteAvailable()
{
	SSHWidget *currSSH = mp_tabbinator->getCurrentSSHWidget();
	QClipboard *clipboard = QApplication::clipboard();
	if( currSSH && clipboard)
	{
		QString currentSelection = currSSH->getCurrentSelection();

			//Set availability of copy
			if( currentSelection.isEmpty() )
				actionCopy->setEnabled(false);
			else
				actionCopy->setEnabled(true);

			//Set availability of paste
			QString subType = "plain";
			if( clipboard->text( subType  ).isEmpty() )
				action_Paste->setEnabled(false);
			else 
				action_Paste->setEnabled(true);
			
			//set sucessfully return now
	}
	else
	{
		//if some of the pointers didn't work out assume that no tab is open or simething is wrong
		//disable the copy paste to prevent crashs
		action_Paste->setEnabled(false);
		actionCopy->setEnabled(false);
	}
}


////////////////////////////////////////////////////////////////////
/* do when the user asks for the contect menu */
void MainWindowDlg::contextMenuEvent(QContextMenuEvent *event){
	menuEdit->exec(event->globalPos());
}
////////////////////////////////////////////////////////////////////
void
MainWindowDlg::createStatusBar()
{
	/* Our status bar is going to consist of a label that holds the CWD
	 * and a 2nd label that holds a status indicator showing the status
	 * of the connection on the tab */
	mp_SB_currentWorkingDir = new QLabel(this);
	mp_SB_statusIndicator = new QLabel(this);
	statusBar()->insertPermanentWidget(0, mp_SB_currentWorkingDir, 1 );
	statusBar()->insertPermanentWidget(1, mp_SB_statusIndicator );
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::saveSettings()
{
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);

	//Lets save our window position and size for the next time we open
	settings.setValue("windowPosition", pos());
	settings.setValue("windowSize", size());

	//Remove any previously saved connection history
	settings.beginGroup("history");
	settings.remove("");

	//Add in new connections
	for (int i = 0; i < mp_tabbinator->count(); i++)
	{
		SSHWidget *pSSH = dynamic_cast<SSHWidget*>( mp_tabbinator->widget( i));
		if ( pSSH)
		{
			ConnectionDetails * const pCD = pSSH->getConnectionDetails();
			if (pCD)
			{
				int connID = pCD->connectionID;
				QString key = "history%1";
				key = key.arg(i);
				settings.setValue( key, connID);
			}
		}
	}
	settings.endGroup();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::restoreSettings()
{
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);

	/***********************
	 * Restore saved window position and size
	 **********************/
	//Retrieve the saved window position, or use default if there isn't a saved one and move window
	QPoint windowPosition = settings.value("windowPosition", QPoint(0,0)).toPoint();
	move( windowPosition);

	//Retrieve the saved window size, or use the default size if there isn't one saved and resize window
	QSize windowSize = settings.value("windowSize", QSize(640, 480)).toSize();
	resize( windowSize);
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::reopenPreviousTabs()
{
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);
	/* The basic idea here: If the user specified connections via the command line, open them
	 * and do not open any connections from previous launch. Otherwise, if the user has allowed
	 * us to open previously left open connections, re-open them */

	/*TODO: implement a better command line parameter handler
	 * right now I just use this to quickly connect to the same session
	 * for quicker debugging */
	if (m_argc >= 4)
	{
		ConnectionDetails *pCD = new ConnectionDetails();
		pCD->hostAddress =  mp_argv[1];
		pCD->username = mp_argv[2];
		pCD->password = mp_argv[3];
		mp_tabbinator->newTab( pCD);
	}
	else if ( mp_userPrefs->reOpenPreviousConnections() )
	{
		settings.beginGroup("history");
		bool bKeyExists = true;
		int ctr = 0;
		while (bKeyExists)
		{
			QString key = "history%1";
			key = key.arg( ctr);
			int connID = settings.value( key, QVariant(-1) ).toInt();
			if ( connID == -1)
			{
				bKeyExists = false;
			}
			else
			{
				ConnectionDetails *pCD = mp_connectionMgr->getConnectionDetails(connID );
				if ( pCD )
					mp_tabbinator->newTab( pCD, false);

			}
			ctr++;
		}

		settings.endGroup();
	}
}

////////////////////////////////////////////////////////////////////
bool
MainWindowDlg::slotClose()
{
	saveSettings();		//Save window position and userPreferences
	
	return QMainWindow::close();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotConnectionManager()
{
	//Show the connection manager
	ConnectionManagerDlg connManagerDlg(this, mp_connectionMgr);
	connManagerDlg.exec();
	createConnectionActions();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotChangeMasterPassword()
{
	CreateChangeMasterPasswordDlg changePasswordDlg(CreateChangeMasterPasswordDlg::CHANGE_PASSWORD, mp_connectionMgr);
	changePasswordDlg.exec();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotUserPreferences()
{
	if ( ! mp_userPreferencesDlg )
	{
		mp_userPreferencesDlg = new UserPreferencesDlg(this, mp_userPrefs);
		connect( mp_userPreferencesDlg, SIGNAL(stuffChanged()),
			this, SLOT(slotUpdateUserPrefChanges()));
	}

	if ( mp_userPreferencesDlg->isHidden() )
		mp_userPreferencesDlg->show();
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotAbout()
{
	QMessageBox::about(this, tr(MAINWINDOWDLG_4),
		tr(MAINWINDOWDLG_5));
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotQuickConnect()
{
	QuickConnectDlg qc(this, this->mp_connectionMgr);
	if ( qc.exec() == QDialog::Accepted)
	{
		int tabNum = mp_tabbinator->newTab( qc.getConnectionDataObj() );
		createConnectionActions();
	}
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotUpdateUserPrefChanges()
{
	for (int i = 0; i < mp_tabbinator->count(); i++)
	{
		SSHWidget *pSSHWidget = dynamic_cast<SSHWidget*>( mp_tabbinator->widget( i) );
		if ( pSSHWidget)
			pSSHWidget->slotResizeConsole(true);
	}
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotToggleDebugTooltip(bool value)
{
#if defined( _DEBUG)
	mp_userPrefs->toggleDebugToolTip(value);
#endif
}

////////////////////////////////////////////////////////////////////
void 
MainWindowDlg::openConnection(ConnectionDetails *connInfo) 
{ 
	mp_tabbinator->newTab(connInfo); 
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::openGroup(GroupDetails *groupInfo) 
{
	ConnectionDetails temp;
	ConnectionDetails *tempPointer;
	QList<ConnectionDetails> ConnectionsInGroup = mp_connectionMgr->getConnectionDetails(QString::number(groupInfo->groupID));
	foreach( temp, ConnectionsInGroup ) {
		tempPointer = new ConnectionDetails();
		*tempPointer = temp;
		openConnection(tempPointer);
	}
	delete groupInfo;
	groupInfo = NULL;
}

////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotUpdateStatusBarInfo( const StatusBarInfo &SBI)
{
	if (SBI.statusEvent == CS_disconnected)
	{
		mp_SB_currentWorkingDir->setText( SBI.statusMsg );
		mp_SB_currentWorkingDir->setToolTip( MAINWINDOWDLG_1  );
		mp_SB_statusIndicator->setToolTip( "" );
		mp_SB_statusIndicator->setPixmap( QPixmap( SBI.iconPath) );
	}
	else
	{
		if ( SBI.cwd.isEmpty() )
			mp_SB_currentWorkingDir->setText( SBI.statusMsg );
		else
			mp_SB_currentWorkingDir->setText( SBI.cwd );

		mp_SB_statusIndicator->setToolTip( SBI.statusMsg );
		mp_SB_statusIndicator->setPixmap( QPixmap( SBI.iconPath) );
	}
}

//////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotCopy()
{
	SSHWidget *currSSH = mp_tabbinator->getCurrentSSHWidget();
	if( currSSH )
	{
		QString currentSelection = currSSH->getCurrentSelection();

#ifdef _DEBUG
		printf("-------------------------------------------------------------\n");
		printf("The current Selection is : \n\"");
		printf(currentSelection.toAscii().data());
		printf("\"\n-------------------------------------------------------------\n");
#endif

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(currentSelection);
	}
}

//////////////////////////////////////////////////////////////////////
void
MainWindowDlg::slotPaste()
{
	SSHWidget *pSSHWidget = mp_tabbinator->getCurrentSSHWidget();
	if ( pSSHWidget)
	{
		pSSHWidget->slotPasteTextFromClipboard();
	}
}

//////////////////////////////////////////////////////////////////////
void
MainWindowDlg::closeEvent(QCloseEvent *pEvent)
{
	saveSettings();		//Save window position and userPreferences
}

//////////////////////////////////////////////////////////////////////
