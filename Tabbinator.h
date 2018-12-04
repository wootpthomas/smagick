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
#include <QTabWidget>
#include <QKeyEvent>

/***********************
 Includes
*************************/
#include "DataTypes.h"
#include "UserPreferences.h"

/************************
Forward declarations
************************/
class ConnectionManager;
class MainWindowDlg;
class QPushButton;
class SSHWidget;
struct ConnectionDetails;


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/* Our tab widget keeps a list of the statusbar info for each SSHWidget. Since the 
 * individual SSHWidgets will update their status bar info as events unfold and only
 * one tab is visible at a time, the Tabbinator can now correctly show up-to-date
 * statusbar info on a tab change */
struct StatusBarInfoItem{
	const SSHWidget
		*pSSHWidget;

	StatusBarInfo
		statusBarInfo;

	StatusBarInfoItem(){
		pSSHWidget = NULL;
	}
};


class Tabbinator :
	public QTabWidget
{
	Q_OBJECT

public:
	Tabbinator(MainWindowDlg * const pMainWin, ConnectionManager * const pConnectionMgr, UserPreferences * const pUserPrefs);
	~Tabbinator(void);

	int newTab(ConnectionDetails *pConnectionDetails, bool makeActive = true);

	/* returns a pointer to the SSHWidget object in the current tab */
	SSHWidget *getCurrentSSHWidget() const;

	/* Allows us to show or hide the tabBar. Useful for when we go into or out of
	 * fullscreen mode */
	void toggleTabBarVisibility(bool visible);

protected:
	/* We override the keypress event so that we can grab the keypresses
	 * and filter them */
	void keyPressEvent(QKeyEvent *e);

	//PT: Commented out since we probably want people to use tabs since its
	//	our main feature.
	///* Over ride them so we can control when the tabBar is shown or hidden */
	//virtual void tabInserted(int index)	{ toggleTabBarVisibility(); }
	//virtual void tabRemoved(int index)	{ toggleTabBarVisibility(); }

public slots:
	/* This slot is meant to be called from one of the TabWidget's
	 * pages. The page is closing and it sends a signal to the tabWidget. 
	 * Its safe to close a tab during a QEvent loop from this function. */
	void slotCloseTab(QWidget *pPageWidget );

	/* Convience slot */
	void slotCloseCurrentTab() { slotCloseTab( currentWidget()); }

	//* Enable/Disable the printing of debugging data to the console */
	void slotToggleDebugPrinting(bool enable);

private slots:
	/* Meant to be called from one of the TabWidget's pages. The page's
	 * SSHConnection has been disconnected. Change the tabindex to this
	 * page and pop up a dialog message to the user. */
	void slotConnectionClosed(QWidget *pPageWidget);

	/* In order to keep the focus on the Console when a tab position
	 * is changed or a tab is removed, we define this slot to be alerted
	 * when position changes, then we take care of the focus as needed. */
	void slotCurrentChanged(int newPageIndex);

	/* Called when one of our SSHWidgets wants to change the statusBarInfo
	 * displayed for it. This slot updates the info and will also update
	 * what the main window's statusbar shows if that widget is the current
	 * widget in the tabWidget. */
	void slotStatusBarInfo(SSHWidget *pSSHWidget, StatusBarInfo SBI);

private:
	/**********************
	 Private Functions
	**********************/
	/* Looks at the number of tabs and decides if we need to show the
	 * TabBar or not */
	void toggleTabBarVisibility();

	/**********************
	 Private Data
	**********************/
	ConnectionManager
		* const mp_connectionMgr;	//Pointer to let use easily access our connectionMgr

	UserPreferences
		* const mp_userPrefs;

	MainWindowDlg
		* const mp_mainWin;			//Pointer to let us easily access our mainwindow

	QPushButton
		*mp_btnCloseTab,	//Closes the current tab
		*mp_btnNewTab;	//Opens a new tab via the quickConnect dialog

	QList<StatusBarInfoItem>
		m_statusBarInfoList;	//Holds the status bar stuff for each tab widget
};
