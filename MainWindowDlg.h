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

#pragma once

/***********************
 Qt Includes
*************************/
#include <QMainWindow>
#include <QClipboard>

/***********************
 Other Includes
*************************/
#include "ui_MainWindow.h"
#include "ConnectionManager.h"
#include "Tabbinator.h"
#include "TransferManager.h"
#include "FileTransferProgressDlg.h"



/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/*************************
 Forward Class Declarations
**************************/
class QLabel;
class UserPreferences;
class UserPreferencesDlg;
struct ConnectionDetails;
class SSHWidget;
class QTimer;



#ifdef _DEBUG
class QAction;
#endif


class MainWindowDlg :
	public QMainWindow,
	public Ui_MainWindow
{
	Q_OBJECT

public:
	MainWindowDlg(ConnectionManager * const pConnectionManager, int argc, char *argv[]);
	~MainWindowDlg(void);

	/* Returns a pointer to the connection manager */
	//const ConnectionManager * const getConnectionManager() const { return mp_connectionMgr; }

	/* Returns a pointer to the transfer manager. Meant to be used for signal/slot
	 * connections */
	const TransferManager *getTransferMgrPtr() const { return mp_transferManager; }

public slots:
	/* Opens a new tab with the settings specified in the connectionInfo object */
	void openConnection(ConnectionDetails *connInfo);

	/* Opens new tabs for each connection in the group */
	void openGroup(GroupDetails *groupInfo);

	/* Opens up the Quick Connect dialog. Upon successful completion,
	 * this will spawn a new tab with a SSHConnection. The tab will try
	 * and connect with the given details from the dialog */
	void slotQuickConnect();
	
	/* This will close all open SSH connections, free all resources, save
	 * any changed connectionManager data and finally exit the application */
	bool slotClose();

	/* Opens up the user preferences window */
	void slotUserPreferences();
	
	/* Shows the Conneciton manager */
	void slotConnectionManager();

	/* Shows the Shell Magic dialog and info about the authors */
	void slotAbout();

	/* Shows the Qt built-in help box. Provides information about the Qt version
	 * used. */
	void slotAboutQt()	{ qApp->aboutQt(); }

	void slotChangeMasterPassword();

	/*Called whenever the TabWidget changes the active tab. The statusbar info is
	 * then updated to the given statusBarInfo object's contents.*/
	void slotUpdateStatusBarInfo( const StatusBarInfo &);

	/* Called when the user preferences have changed. This normally forces the current
	 * SSHWidget to resize and repaint. All other widgets in the tab are also called */
	void slotUpdateUserPrefChanges();
	
	/* called when the user clicks copy */
	void slotCopy();

	/* called when the user toggles the debuging tool tip option */
	void slotToggleDebugTooltip(bool value);

	/* Called when the user wants to paste text from clipboard. This tells the current
	 * SSHWidget to paste contents from clipboard */
	void slotPaste();

	/* Called when a user shows a menu containing copy and past
	 * This will enable and disable copy past accordingly */
	void slotUpdateCopyPasteAvailable();

protected:
	void closeEvent(QCloseEvent *pEvent);

private:
	/***********************
	 Private Functions
	************************/
	/* Initializes the mainWindowDialog and sets up any needed objects */
	void init();

	/* Sets up the signal/slot linkage between things like the menus to their
	 * respective functions */
	void createActions();

	//This function is very similar to the above but it only creates the menu stuff for the stored connections
	void createConnectionActions();

	void createMenus();

	void createStatusBar();

	/* Saves window position and user preferences stuff */
	void saveSettings();

	/* Restores window position and user preferences stuff if saved*/
	void restoreSettings();

	/* Re-opens any previous tabs that were present when the user last
	 * closed shell magick */
	void reopenPreviousTabs();

	/* do when the user asks for the contect menu */
	void contextMenuEvent(QContextMenuEvent *event);

	/***************************
	Private Variables
	***************************/
	ConnectionManager
		* const mp_connectionMgr;

	TransferManager
		*mp_transferManager;		//Responsible for handling all file/folder transfers

	FileTransferProgressDlg
		*mp_fileTransferProgressDlg;

	Tabbinator
		*mp_tabbinator;			//Central tab widget
	
	/* Status bar widget items */
	QLabel 
		*mp_SB_currentWorkingDir,	//Holds the current working directory of the current console
		*mp_SB_statusIndicator;		//Shows a visual status indicator of the current console

	UserPreferences
		*mp_userPrefs;				//Pointer to our user preferences object

	UserPreferencesDlg
		*mp_userPreferencesDlg;

	int
		m_argc;						//If nonzero this is how many command line arguments we have
	
	char 
		**mp_argv;		//essentiall is argv[] from our main() function

#ifdef _DEBUG
	QAction
		*mp_actionEnableDebugPrinting;	//Enable debug output
	QAction
		*mp_actionEnableDebugTooltip;   //Enable debug Tooltiping...
#endif
};
