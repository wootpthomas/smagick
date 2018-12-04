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
***********************/
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QDropEvent>
#include <QUrl>


/***********************
 Other Includes
***********************/
#include "DataTypes.h"
#include "SSHWidgetThread.h"
#include "QPainterRenderWidget.h"
#include "UserPreferences.h"
#include "MainWindowDlg.h"

/***********************
 Forward Class declarations
***********************/
class QVBoxLayout;
class QTimer;



/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/***********************
	Defines
***********************/
#define SSHWIDGET_1 "Login Error"
#define SSHWIDGET_2 "No login method was specified."
#define SSHWIDGET_3 "Could not initialze the SSH Connection. Verify that you have internet connectivity."
#define SSHWIDGET_4 "Could not login, check username and password."
#define SSHWIDGET_5 "Library error: Could not create a channel for the console."

/* Error message translations */
#define SSHWIDGET_10 "Creating socket"
#define SSHWIDGET_11 "Could not create a socket"
#define SSHWIDGET_12 "Looking up IP address of %1" // hostname
#define SSHWIDGET_13 "Could not find the IP address of %1" //ip address
#define SSHWIDGET_14 "The IP address is invalid: %1" //ip address
#define SSHWIDGET_15 "Trying to connect to %1"	//ipaddress or host
#define SSHWIDGET_16 "Unable to connect to %1"	//ip address or host
#define SSHWIDGET_17 "Initializing the library"
#define SSHWIDGET_18 "Library initialization failed!"
#define SSHWIDGET_19 "Retrieving host's fingerprint"
#define SSHWIDGET_20 "Could not get the fingerprint for %1" //host
#define SSHWIDGET_21 "Retrieving host's authentication methods"
#define SSHWIDGET_22 "Unable to get authentication methods"
#define SSHWIDGET_23 "Requesting a new SSH channel"
#define SSHWIDGET_24 "Unable to get a SSH channel"
#define SSHWIDGET_25 "Attempting to login by keyboard authentication"
#define SSHWIDGET_26 "Failed to login by keyboard authentication"
#define SSHWIDGET_27 "Attempting to login by RSA authentication"
#define SSHWIDGET_28 "Failed to login by RSA authentication"
#define SSHWIDGET_29 "Requesting a PTY"
#define SSHWIDGET_30 "Failed to get a PTY"
#define SSHWIDGET_31 "Requesting a shell"
#define SSHWIDGET_32 "Failed to get a shell"
#define SSHWIDGET_33 "Requesting a tunnel setup"
#define SSHWIDGET_34 "Failed to setup tunnel"
#define SSHWIDGET_35 "Setting up enviornmental variables"
#define SSHWIDGET_36 "Failed to setup enviornmental variables"
#define SSHWIDGET_37 "Connected"
#define SSHWIDGET_38 "Connection timed out or was lost"
#define SSHWIDGET_39 "Normal shutdown"
#define SSHWIDGET_40 "Unknown status event"


/***********************
 Class definition
***********************/
/* This class is the object that will live inside of a QTabWidget and will
 * essentially be the contents of a tab. When creating a new QTabWiget, this
 * object will be given to it. It holds the SSH console view the user sees, 
 * the remote file browser and its internal stuff will provide the signal/slot
 * connections to make this tab be alive like Number 5! */
class SSHWidget :
	public QWidget
{
	Q_OBJECT

public:
	SSHWidget(
		ConnectionDetails *pConnectionDetails,
		UserPreferences * const pUserPrefs,
		MainWindowDlg * const pMainWin,
		Tabbinator * const pTabbinator);
	~SSHWidget(void);

	/* Initializes our connection and gets stuff starting to connect*/
	void init();

	/* Returns a pointer to the connectionDetail object that this widget
	 * is using to initialize its connection. Useful for querying this tab's
	 * connection information, or for updating this connection object on the 
	 * connectionManager if, for instance, the user changed the password*/
	ConnectionDetails * const getConnectionDetails() const { return mp_connectionDetails; }

	/* The Tabbinator is expected to update this value on page changes. If the
	 * tab this page lives in is active, this is set to true. This will keep the
	 * qApp->ProcessEvents() from being called from tabs that are not visible */
	void isOnActiveTab(bool isVisible);

	// Allows the Tabbinator to query this page's believed-to-be active status
	bool isOnActiveTab() const { return m_isOnActiveTab; }

	/* Called when we need to make certain that the console we are using
	 * has keyboard focus */
	void setConsoleActive() { setFocus(); }

	/* Sets the maximum number of lines that we display */
	void maxLinesToDisplay( int maxLines) { m_maxLinesToDisplay = maxLines; }

	/* returns the max number of lines that we display */
	int maxLinesToDisplay() const { return m_maxLinesToDisplay; }

	/* returns a reference to our statusbar info object */
	const StatusBarInfo& getStatusInfo() const { return m_statusBarInfo; }

	/* returns the QString representation of the current selection */
	QString getCurrentSelection();

signals:
	/* Sent out when the information that corresponds to the main window's statusbar
	 * needs to be updated to reflect this widget's changed event. */
	void updateStatusBarInfo(SSHWidget *pSSHWidget, StatusBarInfo);

	/* Sent out when the console is resized. This is connected to our SSHConnection
	 * object in the SSHWidgetWorker thread. The connection is made there. */
	void signalResizeConsole(int currentWidth, int currentHeight, int widthPx, int heightPx);

	/* Sent out when a user drops a file/folder or list of the files on our widget. 
	 * We assume that they want to transfer these files to the remote system. This signal
	 * alerts our transferManager of the request. */
	void fileTransfer(TransferRequest tr);

	/* Sent after our user gives us a new password to try to login with */
	void retryLoginWithPassword( QString);

	/* Normally called when we try to do a re-login after a failed login attempt and
	 * the user clicks cancel */
	void closeMe(QWidget*);

	/* Sent so that when prompting for a password, the TabWidget can make the proper
	 * Tab active */
	void makeActive(QWidget*);

	/* Sent when we recieve a keypress event or some text to send on the user's behaf */
	void textToSend(QString text);

protected:

	/* Filters our keypress events and makes sure the correct SSH friendly text is sent
	 * to our SSHConnection object. Ex: user presses left arrow, we send two different
	 * things depending on graphical or text mode */
	void keyPressEvent(QKeyEvent *pEvent);

	/* Filter out special events... such as the Tab key being pressed */
	bool event(QEvent *pEvent);
	
	/* Needed to accept drag-n-drop events */
	void dragEnterEvent(QDragEnterEvent *pEvent);
	void dragMoveEvent(QDragMoveEvent *pEvent)		{ pEvent->acceptProposedAction(); }
	void dragLeaveEvent(QDragLeaveEvent *pEvent)		{ pEvent->accept(); }
	void dropEvent(QDropEvent *pEvent);

	/* Over ride the resize event so that we can send our SSHObject's console channel
	 * appropriate resize notifications */
	void resizeEvent ( QResizeEvent * event );

public slots:

	/* Prompts the user for the correct password. Only called when auth fails */
	void slotGetCorrectPassword();

	/* Called when the decoder tells us that it recieved a bell character */
	void slotPlayBell() { };

	/* Called when the connection status of the SSH object changes.
	 * This is the main method of providing the user with connection
	 * feedback. */
	void slotConnectionStatusChanged( StatusChangeEvent statusEvent);

	/* Called when the current working directory has been changed. Also is
	 * called when the window title is changed. */
	void slotWindowTitleChanged(QString windowTitle);

	/* Called when we want to tell one of our consoles to update its display. Note that this
	 * does not force a display. It just segguests that the console should update if it has
	 * any changes to display */
	void slotRender();

	/* Called a few msec after a resize has ended. Responsible for telling our SSHWidgets
	 * to inform their SSH connected servers about a console size change */
	void slotResizeConsole(bool forceRepaint = false);

	/* Pastes any text from the clipboard into our app if text is available */
	void slotPasteTextFromClipboard();

	/**********************
	 Console/Decoder Interaction slots
	***********************/
	/* Tells the Decoder to enable printing the SSH data to the console.
	 * Meant to be used for debugging. */
	void slotToggleDebugPrinting(bool enable);

private slots:
	/* Called by SSHWidgetThread when its successfully logged in. We then set a
	 * flag that allows resize events to tell the remote server our console size
	 * and also force a resize */
	void slotFinishStartup();

private:
	/*********************
	Private Functions
	*********************/

	/* Called when we need to know how to translate cursor keys based on
	 * the Decoder's current state. */
	bool areCursorKeysInAppMode() { return mp_workerThread->areCursorKeysInAppMode(); }

	/* Called when we need to know how to translate cursor keys based on
	 * the Decoder's current state. */
	bool areKeypadKeysInAppMode() { return mp_workerThread->areKeypadKeysInAppMode(); }

	/* Called to get the console's size in pixels and characters it can display with
	 * the current UserPreference's font settings */
	void getConsoleSizes(int &cols, int &rows, int &widthPx, int &heightPx);

	/*********************
	Private Variables
	*********************/

	/* Now the resize events will come in quite quickly. To keep from flooding
	 * our sshconnection with resize events, we'll keep tabs on the console size
	 * and only submit an adujustConsoleSize request when the size changes */
	int
		m_consoleColumns,		
		m_consoleRows,
		m_maxLinesToDisplay,		//The max number of lines to display.
		m_lastWidthUpdate,
		m_lastHeightUpdate;

	bool
		m_isInGraphicalMode,		//Flag that tells us what mode we are operating in
		m_isOnActiveTab,			//Lets us know if this tab is visible by the user
		m_allowResize;				//Used to skip resizing stuff if not yet initialized

   RenderWidget
		*mp_renderWidget;			//the user visible console

	QVBoxLayout
		*mp_mainLayout;				//Main layout for this tab object

	SSHWidgetThread
		*mp_workerThread;			//Thread to do all of our bi0tch work

	ConnectionDetails
		*mp_connectionDetails;		//Holds a pointer to our connectionDetails obj

	QColor
		m_currentTextColor,			//Color describing the text color to draw with
		m_currentTextBGColor;		//Color describing the background text color

	QString
		m_cwd;						//Holds the last thing that we think is a valid CWD

	StatusBarInfo
		m_statusBarInfo;			//Holds the info that we will send for statusbar updates

	QTimer
		*mp_renderTimer,			//Helps tell when we can do a rendering
		*mp_resizeTimer;			/*Used to help redneck our resizing so that we hopefully only
										 * send resize events when the user has finished sizing. 
										 * Unfortunately there isn't a cross-platfrom way in Qt to find
										 * out when a window has finished being resized */

	UserPreferences
		* const mp_userPrefs;	//Thread-safe user preferences object. We do Not own it

	MainWindowDlg
		* const mp_mainWin;		/* Used ONLY for making an educated guess at the console's initial size
										 * when we first start things up */
	Tabbinator
		* const mp_tabbinator;	/* Used to help show/hide stuff when going into fullscreen mode */
};

