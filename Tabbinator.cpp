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


/***********************
 Qt Includes
*************************/
#include <QTabWidget>
#include <QMessageBox>
#include <QTabBar>
#include <QThread>
#include <QPushButton>

/***********************
 Includes
*************************/
#include <exception>
#include "Tabbinator.h"
#include "ConnectionManager.h"
#include "DataTypes.h"
#include "SSHWidget.h"
#include "SSHWidgetThread.h"
#include "MainWindowDlg.h"



/***********************
 Defines
*************************/
#define TABBINATOR_1 "Shell Magic Error"
#define TABBINATOR_2 "The connection timed out."
#define TABBINATOR_3 "Close the current tab"
#define TABBINATOR_4 "Open a new tab"

////////////////////////////////////////////////////////////////////
Tabbinator::Tabbinator(MainWindowDlg * const pMainWin, ConnectionManager * const pConnectionMgr, UserPreferences * const pUserPrefs):
	QTabWidget(pMainWin),
	mp_mainWin(pMainWin),
	mp_connectionMgr( pConnectionMgr),
	mp_userPrefs(pUserPrefs)
{
	//Let's setup some snazzy open/close buttons
	mp_btnCloseTab = new QPushButton(this);
	mp_btnCloseTab->setIcon( QPixmap(":/icons/delete.png"));
	mp_btnCloseTab->setToolTip( TABBINATOR_3 );

	mp_btnNewTab = new QPushButton(this);
	mp_btnNewTab->setIcon( QPixmap(":/icons/add.png"));
	mp_btnNewTab->setToolTip( TABBINATOR_4 );

	//Add our buttons to our Tabbinator
	this->setCornerWidget( mp_btnCloseTab, Qt::TopRightCorner);
	this->setCornerWidget( mp_btnNewTab, Qt::TopLeftCorner);

	//Connect up our buttons to make them functional
	connect(mp_btnNewTab, SIGNAL(clicked()),
		mp_mainWin, SLOT(slotQuickConnect()));
	connect(mp_btnCloseTab, SIGNAL(clicked()),
		this, SLOT(slotCloseCurrentTab()));

	//Connect the tab index change to our slot so we can change focus appropriately
	connect(this, SIGNAL(currentChanged(int)),
		this, SLOT(slotCurrentChanged(int)));
}

////////////////////////////////////////////////////////////////////
Tabbinator::~Tabbinator(void)
{
}

////////////////////////////////////////////////////////////////////
int
Tabbinator::newTab(ConnectionDetails *pConnectionDetails, bool makeActive)
{
	if (pConnectionDetails)
	{
		SSHWidget *pSSHWidget = new SSHWidget(pConnectionDetails, mp_userPrefs, mp_mainWin, this );
		int tabNum = addTab( pSSHWidget, pConnectionDetails->hostAddress );

		//Connect the widget so that it can close its own tab
		connect(pSSHWidget, SIGNAL(closeMe(QWidget*)),
			this, SLOT(slotCloseTab(QWidget*)));

		/* Lets the widget request that it be the current tab. Only sent when putting
		 * in a new password after a login failure */
		connect(pSSHWidget, SIGNAL(makeActive(QWidget*)),
			this, SLOT(setCurrentWidget(QWidget*)));

		// connect up the widget's updateStatusBarInfo signal
		connect(pSSHWidget, SIGNAL(updateStatusBarInfo(SSHWidget *, StatusBarInfo)),
			this, SLOT(slotStatusBarInfo(SSHWidget *, StatusBarInfo)));

		/*Connect up the file transfer signal so when our widget gets a file dropped on it
		 * we send the request to our transfer manager to deal with */
		connect(pSSHWidget, SIGNAL(fileTransfer(TransferRequest)),
			mp_mainWin->getTransferMgrPtr(), SLOT(slotFileTransfer(TransferRequest)));

		//Add a statusbarInfo item to our list for this widget
		StatusBarInfoItem SBInfoItem;
		SBInfoItem.pSSHWidget = pSSHWidget;
		m_statusBarInfoList.append( SBInfoItem);

		// Startup our tab!
		pSSHWidget->init();
		pSSHWidget->setConsoleActive();

		pSSHWidget->isOnActiveTab(true);

		if (makeActive)
			this->setCurrentIndex(tabNum);
	
		return tabNum;
	}
	
	return -1;
}

////////////////////////////////////////////////////////////////////
SSHWidget *
Tabbinator::getCurrentSSHWidget() const
{
	return dynamic_cast<SSHWidget *>( currentWidget() );
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::slotCloseTab(QWidget *pPageWidget)
{
	if (pPageWidget)
	{
		int tabIndex = indexOf(pPageWidget);
		if (tabIndex != -1)
		{
			SSHWidget *pSSHWidget = dynamic_cast<SSHWidget *>(pPageWidget);
			if (pSSHWidget)
			{
				//Remove the statusBarInfo item from our list
				for (int i = m_statusBarInfoList.size()-1; i >= 0 ; i--)
				{
					if ( m_statusBarInfoList[i].pSSHWidget == pSSHWidget)
						m_statusBarInfoList.removeAt(i);
				}

				//Execute the old tab, right in the temple... it'll be quick
				removeTab(tabIndex);		//Removes the tab from the display

				/* Schedule the object for deletion when control returns to the event loop.
				 * Important: Do not call delete on this object, its event loop is still
				 * running code! */
				pSSHWidget->deleteLater();

				//If all tabs are now closed, exit out of fullscreen mode if we are in it
				if ( count() == 0)
				{
					if ( mp_mainWin->isFullScreen())
					{
						mp_mainWin->statusBar()->show();
						mp_mainWin->menuBar()->show();
						toggleTabBarVisibility(true);
						mp_mainWin->showNormal();
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::slotConnectionClosed(QWidget *pPageWidget)
{
	if (pPageWidget)
	{
		int tabIndex = indexOf(pPageWidget);
		if (tabIndex != -1)
		{
			//Change to the current tab index, its been disabled.
			this->setCurrentIndex( tabIndex);

			//Pop up a dialog alerting the user of what has happened
			QMessageBox::warning( this, TABBINATOR_1, TABBINATOR_2, QMessageBox::Ok);
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::slotCurrentChanged(int newPageIndex)
{
	SSHWidget *pSSHWidget = NULL;
	/* Set all the tab objects to non-active */
	for (int i = 0; i < count(); i++)
	{
		pSSHWidget = dynamic_cast<SSHWidget *>( this->widget(i) );
		if (pSSHWidget)
			pSSHWidget->isOnActiveTab( false);
		pSSHWidget = NULL;
	}

	//this->setCurrentWidget( pPageWidget);	//Move the focus to the tab's page (console)
	pSSHWidget = dynamic_cast<SSHWidget *>( widget(newPageIndex) );
	if (pSSHWidget)
	{
		pSSHWidget->isOnActiveTab(true);		//Let SSHWidget know its being displayed
		pSSHWidget->setConsoleActive();			//Force the keyboard focus
	}

	//Now update our mainwindow statusbar to reflect the current widget
	for (int i = 0; i < m_statusBarInfoList.size(); i++)
	{
		if ( m_statusBarInfoList[i].pSSHWidget == pSSHWidget)
		{
			mp_mainWin->slotUpdateStatusBarInfo(
				m_statusBarInfoList[i].statusBarInfo);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::slotToggleDebugPrinting(bool enable)
{
	SSHWidget *pSSHWidget = dynamic_cast<SSHWidget *>( currentWidget() );
	if (pSSHWidget)
	{
		pSSHWidget->slotToggleDebugPrinting(enable);
	}
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::keyPressEvent(QKeyEvent *e)
{
	if (e->modifiers().testFlag(Qt::ShiftModifier) )
	{
		//Shift key is being pressed
		if ( e->key() == Qt::Key_Left || e->key() == Qt::Key_Right)
		{
			int numOfTabs = count();
			if (numOfTabs > 1)
			{
				int currentTab = currentIndex();
				int newTabNum;
				if ( e->key() == Qt::Key_Left )
				{
					if (currentTab == 0)
						newTabNum = numOfTabs-1;
					else
						newTabNum = currentTab -1;
				}
				else //We want to move to the right
				{
					if (currentTab == numOfTabs-1)
						newTabNum = 0;
					else
						newTabNum = currentTab +1;
				}
				setCurrentIndex( newTabNum);
				e->accept();		//We just ate the keypress event
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::slotStatusBarInfo(SSHWidget *pSSHWidget, StatusBarInfo SBI)
{
	for (int i = 0; i < m_statusBarInfoList.size(); i++)
	{
		if ( m_statusBarInfoList[i].pSSHWidget == pSSHWidget)
		{
			if ( SBI.statusEvent == CS_shutdownNormal)
			{
				slotCloseTab(pSSHWidget);			//Close the tab and remove this item from the list
				return;
			}
			else
			{
				m_statusBarInfoList[i].statusBarInfo = SBI;
			}
			
			break;
		}
	}

	//Now if the current widget is the one being updated, update our statusbar
	if ( currentWidget() == pSSHWidget )
		mp_mainWin->slotUpdateStatusBarInfo(SBI);
}

////////////////////////////////////////////////////////////////////
void
Tabbinator::toggleTabBarVisibility(bool visible)
{
	if (visible)
		tabBar()->show();
	else
		tabBar()->hide();
}

////////////////////////////////////////////////////////////////////