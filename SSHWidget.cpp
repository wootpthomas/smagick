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
***********************/
#include <QVBoxLayout>
#include <QThread>
#include <QKeyEvent>
#include <QString>
#include <QScrollBar>
#include <QMimeData>
#include <QFileInfo>
#include <QTimer>
#include <QClipboard>


/***********************
 Other Includes
***********************/
#include "SSHWidget.h"
#include "DataTypes.h"
#include "SSHWidgetThread.h"
#include "FileTransferDlg.h"
#include "RetryLoginDlg.h"
#include "QPainterRenderWidget.h"

/***********************
 * #Defines
 **********************/
#define RENDER_INTERVAL			33	//33msec is about 30fps, 20 is 40 fps
#define VERT_SCROLLBAR_WIDTH	20
#define FUDGE_WIDTH				30 //pixels we subtract when guessing console width
#define FUDGE_HEIGHT				75 //Pixels we subtract when guessing console height 



////////////////////////////////////////////////////////////////////
SSHWidget::SSHWidget(
							ConnectionDetails *pConnectionDetails,
							UserPreferences * const pUserPrefs,
							MainWindowDlg * const pMainWin,
							Tabbinator * const pTabbinator):
QWidget(),
mp_connectionDetails(pConnectionDetails),
mp_userPrefs(pUserPrefs),
mp_mainWin(pMainWin),
mp_tabbinator( pTabbinator),
m_lastWidthUpdate(0),
m_lastHeightUpdate(0),
mp_renderWidget(NULL),
m_allowResize(false)
{
	/* Create all needed objects */
	mp_mainLayout	= new QVBoxLayout(this);

	//Setup our render timer
	mp_renderTimer	= new QTimer();
	mp_renderTimer->setSingleShot(false);		//Reoccurring
	mp_renderTimer->setInterval( RENDER_INTERVAL );

	//Connect up our render signal
	connect(mp_renderTimer, SIGNAL(timeout()),
		this, SLOT(slotRender()));

	//Accept drag-n-drop drops
	setAcceptDrops(true);

	m_cwd = "";

	//Register our type (if not registered) so we can send our custom object across in a signal
	int typeNum = QMetaType::type("StatusBarInfo");
	if ( ! typeNum)
		qRegisterMetaType<StatusBarInfo>("StatusBarInfo");
}

////////////////////////////////////////////////////////////////////
SSHWidget::~SSHWidget(void)
{
	if ( mp_resizeTimer)
	{
		delete mp_resizeTimer;
		mp_resizeTimer = NULL;
	}

	if ( mp_renderTimer)
	{
		mp_renderTimer->stop();
		delete mp_renderTimer;
	}

	//Shutdown our worker thread
	if (mp_workerThread && mp_workerThread->isRunning() )
	{
		printf("Worker thread is running, telling it to quit\n");
		mp_workerThread->quit();
		mp_workerThread->wait();	//Wait on it to die before continuing on
	}

	if (mp_workerThread)
	{
		if (! mp_workerThread->isFinished())
			printf("Tried to kill a thread before it was finished\n");

		delete mp_workerThread;
		mp_workerThread = NULL;
	}

	//Now no one should be using the connectionDetails object. Since we own
	// it, lets get rid of it now
	if ( mp_connectionDetails)
	{
		delete mp_connectionDetails;
		mp_connectionDetails = NULL;
	}

	/* mp_mainLayout, mp_textConsole, mp_graphicalConsole
	 * will all be deleted automagically by Qt since this widget is
	 * their parent */
}

////////////////////////////////////////////////////////////////////
void
SSHWidget::init()
{
	mp_renderWidget = new QPainterRenderWidget(this, mp_userPrefs);

	mp_mainLayout->addWidget(mp_renderWidget);
	mp_mainLayout->setSpacing( 0);
	mp_mainLayout->setMargin( 0);
	this->setLayout( mp_mainLayout);

	//Set the widget which will actually get focus when SSHWidget gets focus
	mp_renderWidget->setFocusProxy(this);
	setFocusPolicy(Qt::StrongFocus);		//Allow our widget to be "tabbed to"

	//Setup our resize timer
	mp_resizeTimer = new QTimer(this);
	mp_resizeTimer->setSingleShot(true);
	connect(mp_resizeTimer, SIGNAL(timeout()),
		this, SLOT(slotResizeConsole()));

	//For the initial console size, we just make a rough guess at the
	//size of the console. We'll call a resize event when everything's connected
	QFontMetrics fm( mp_userPrefs->font() );
	int consoleW = (mp_mainWin->geometry().width() - FUDGE_WIDTH) / fm.averageCharWidth();
	int consoleH = (mp_mainWin->geometry().height() - FUDGE_HEIGHT) / fm.height();

	mp_workerThread	= new SSHWidgetThread(
		mp_connectionDetails, this, consoleW, consoleH);
	
	/* Connect up our consoles so they can send data easily */
	connect(this, SIGNAL(textToSend(QString)), 
		mp_workerThread, SLOT(slotSendConsoleData(QString)));

	connect(this, SIGNAL(signalResizeConsole(int, int, int, int)),
		mp_workerThread, SLOT(slotResizeConsole(int, int, int, int)));
	
	connect(mp_workerThread, SIGNAL(loginFailedNeedNewPWD()),
		this, SLOT(slotGetCorrectPassword()));

	connect(mp_workerThread, SIGNAL(readyToWork()),
		this, SLOT(slotFinishStartup()));

	//Startup the thread and change its affinity
   mp_workerThread->start();
	mp_workerThread->moveToThread(mp_workerThread);

	mp_renderTimer->start();

	show();		//Finally, show the widget
}

////////////////////////////////////////////////////////////////////
void
SSHWidget::isOnActiveTab(bool isVisible)
{
	m_isOnActiveTab = isVisible;
}

//////////////////////////////////////////////////////////////////////
QString 
SSHWidget::getCurrentSelection()
{
	return ((QPainterRenderWidget*)mp_renderWidget)->getSelection();
}
//////////////////////////////////////////////////////////////////////
void
SSHWidget::getConsoleSizes(int &cols, int &rows, int &widthPx, int &heightPx)
{
	QFontMetrics fm( mp_userPrefs->font() );

	int 
		singleCharWidthPx		= fm.averageCharWidth(), //don't use maxwidth, looks bad on Vista
		singleCharHeightPx	= fm.height(),
		consoleWidthPx			= mp_renderWidget->width() - VERT_SCROLLBAR_WIDTH,
		consoleHeightPx		= mp_renderWidget->height(),
		horCharsCanDisplay	= consoleWidthPx / singleCharWidthPx,
		vertCharsCanDisplay	= consoleHeightPx / singleCharHeightPx,
	//now lets see how many pixels are left over
		horLeftovers			= consoleWidthPx % singleCharWidthPx,
		vertLeftovers			= consoleHeightPx % singleCharHeightPx;

	/* If we have any leftovers, it means that with the current dimensions, we will
	 * clip off parts of characters. So let's make a slight adjustment */
	//if ( horLeftovers )
	//	horCharsCanDisplay--;

	//if ( vertLeftovers )
	//	vertCharsCanDisplay--;

	cols		= horCharsCanDisplay;
	rows		= vertCharsCanDisplay;
	widthPx	= consoleWidthPx;
	heightPx	= consoleHeightPx;
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotGetCorrectPassword()
{
	RetryLoginDlg dlg(this);
	dlg.m_host->setText( mp_connectionDetails->hostAddress);
	dlg.m_port->setText( QString("%1").arg(mp_connectionDetails->hostPort) );
	dlg.m_username->setText( mp_connectionDetails->username);

	/* Make the proper tab active for this relogin attempt */
	emit makeActive( (QWidget*)this );

	//Show and run the relogin dialog
	dlg.show();
	if (dlg.exec() == QDialog::Accepted)
		emit retryLoginWithPassword( dlg.m_password->text());
	else
	{
		//The user doesn't want to login or retry. Close this tab
		emit closeMe( (QWidget*)this );
	}
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotConnectionStatusChanged( StatusChangeEvent statusEvent)
{
	m_statusBarInfo.statusEvent = statusEvent;
	m_statusBarInfo.isEventUserVisible = true;
	
	switch(statusEvent)
	{
		case CS_creatingSocket:
			m_statusBarInfo.statusMsg = SSHWIDGET_10;
			m_statusBarInfo.isEventUserVisible = false;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_creatingSocketFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_11;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_lookingUpHostsIPAddress:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_12).arg(mp_connectionDetails->hostAddress);
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_lookingUpHostsIPAddressFailed:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_13).arg( mp_connectionDetails->hostAddress);
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_IPAddressInvalid:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_14).arg( mp_connectionDetails->hostAddress);
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_connectingToRemoteHost:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_15).arg( mp_connectionDetails->hostAddress);
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_connectingToRemoteHostFailed:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_16).arg( mp_connectionDetails->hostAddress);
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_initLIBSSH2:
			m_statusBarInfo.statusMsg = SSHWIDGET_17;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_initLIBSSH2Failed:
			m_statusBarInfo.statusMsg = SSHWIDGET_18;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_gettingHostFingerPrint:
			m_statusBarInfo.statusMsg = SSHWIDGET_19;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_gettingHostFingerPrintFailed:
			m_statusBarInfo.statusMsg = QString(SSHWIDGET_20).arg( mp_connectionDetails->hostAddress);;
			m_statusBarInfo.iconPath = ":/icons/link_warning.png";
			break;
		case CS_gettingAllowedAuthTypes:
			m_statusBarInfo.statusMsg = SSHWIDGET_21;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_gettingAllowedAuthTypesFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_22;
			m_statusBarInfo.iconPath = ":/icons/link_warning.png";
			break;
		case CS_openChannelRequest:
			m_statusBarInfo.statusMsg = SSHWIDGET_23;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_openChannelRequestFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_24;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_loggingInByKeyboardAuth:
			m_statusBarInfo.statusMsg = SSHWIDGET_25;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_loggingInByKeyboardAuthFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_26;
			m_statusBarInfo.iconPath = ":/icons/link_warning.png";
			break;
		case CS_loggingInByRSA:
			m_statusBarInfo.statusMsg = SSHWIDGET_27;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_loggingInByRSAFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_28;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_requestingPTY:
			m_statusBarInfo.statusMsg = SSHWIDGET_29;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_requestingPTYFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_30;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_requestingShell:
			m_statusBarInfo.statusMsg = SSHWIDGET_31;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_requestingShellFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_32;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";
			break;
		case CS_requestingTunnel:
			m_statusBarInfo.statusMsg = SSHWIDGET_33;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			m_statusBarInfo.isEventUserVisible = false;
			break;
		case CS_requestingTunnelFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_34;
			m_statusBarInfo.iconPath = ":/icons/link_warning.png";
			break;
		case CS_setEnvironmentVariables:
			m_statusBarInfo.statusMsg = SSHWIDGET_35;
			m_statusBarInfo.iconPath = ":/icons/link_go.png";
			break;
		case CS_setEnvironmentVariablesFailed:
			m_statusBarInfo.statusMsg = SSHWIDGET_36;
			m_statusBarInfo.iconPath = ":/icons/link_warning.png";
			break;
		case CS_connected:
			m_statusBarInfo.statusMsg = SSHWIDGET_37;
			m_statusBarInfo.iconPath = ":/icons/link_connected.png";
			break;
		case CS_disconnected:
			m_statusBarInfo.statusMsg = SSHWIDGET_38;
			m_statusBarInfo.iconPath = ":/icons/link_error.png";

			mp_renderWidget->setEnabled(false);

			break;
		case CS_shutdownNormal:
			m_statusBarInfo.statusMsg = SSHWIDGET_39;
			m_statusBarInfo.iconPath = ":/icons/link_disconnected.png";
			break;
		default:
			m_statusBarInfo.statusMsg = SSHWIDGET_40;
	}

	emit updateStatusBarInfo( this, m_statusBarInfo);
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotWindowTitleChanged(QString windowTitle)
{
	m_statusBarInfo.cwd = windowTitle;

	emit updateStatusBarInfo( this, m_statusBarInfo);

	//In the case that our regex matches, we update our current working directory
	QRegExp regex("[a-zA-Z0-9]+\\@[a-zA-Z0-9]+:(.*)");
	if ( regex.indexIn( m_statusBarInfo.cwd ) != -1)
	{
		m_cwd = regex.cap(1);
	}
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotRender()
{
	if ( m_isOnActiveTab && mp_renderWidget)
	{
		RenderInfo *pRenderInfo = mp_workerThread->getRenderInfo();
		if (pRenderInfo)
		{
			mp_renderWidget->renderScreen( pRenderInfo );
			pRenderInfo = NULL;
		}

		/* We only get a NULL pointer if
		 * 1) the terminal object is not ready (still starting up)
		 * 2) The last RenderInfo is still up to date */
	}
}

/////////////////////////////////////////////////////////////////////
void
SSHWidget::slotResizeConsole(bool forceRepaint)
{
	//Make sure we are connected
	if ( m_statusBarInfo.statusEvent != CS_connected)
		return;

	int 
		horCharsCanDisplay,
		vertCharsCanDisplay,
		widthPx,
		heightPx;

	getConsoleSizes(horCharsCanDisplay, vertCharsCanDisplay, widthPx, heightPx);

	/* Check and see what dimensions we last sent to our remote SSH server, if they
	 * are the same as what we recieved, don't send a resize update */
	if (m_lastWidthUpdate == horCharsCanDisplay && m_lastHeightUpdate == vertCharsCanDisplay)
		return;

	m_lastWidthUpdate = horCharsCanDisplay;
	m_lastHeightUpdate = vertCharsCanDisplay;
	emit signalResizeConsole(horCharsCanDisplay, vertCharsCanDisplay, widthPx, heightPx);

	if ( forceRepaint)		//Only true when the user changed font/color settings
		mp_renderWidget->update();
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotToggleDebugPrinting(bool enable)
{
	mp_workerThread->toggleDebugPrinting( enable);
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotFinishStartup()
{
	m_allowResize = true;
	slotResizeConsole();
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::slotPasteTextFromClipboard()
{
	QString subType = "plain";
	QString text = QApplication::clipboard()->text( subType );
	if ( ! text.isEmpty())
		emit textToSend( text);
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::keyPressEvent(QKeyEvent *pEvent)
{
	if ( ! pEvent->modifiers().testFlag(Qt::ShiftModifier) )
	{		//Shift key is NOT being pressed

		QString sendMe( QChar(CHAR_ESC));
		bool
			bSendText = true,
			appM = areCursorKeysInAppMode();
		switch( pEvent->key() )
		{
		case Qt::Key_Insert:		sendMe += "[2~";																break;
		case Qt::Key_Home:		sendMe += "[7~";																break;
		case Qt::Key_PageUp:		sendMe += "[5~";																break;
		case Qt::Key_PageDown:	sendMe += "[6~";																break;
		case Qt::Key_End:			sendMe += "[8~";																break;
		case Qt::Key_Delete:		sendMe += "[3~";																break;
		case Qt::Key_Up:			if ( appM )	sendMe += "OA";		else		sendMe += "[A";		break;
		case Qt::Key_Right:		if ( appM )	sendMe += "OC";		else		sendMe += "[C";		break;
		case Qt::Key_Left:		if ( appM )	sendMe += "OD";		else		sendMe += "[D";		break;
		case Qt::Key_Down:		if ( appM )	sendMe += "OB";		else		sendMe += "[B";		break;
		case Qt::Key_F1:			if ( appM )	sendMe += "[11~";												break;
		case Qt::Key_F2:			if ( appM )	sendMe += "[12~";												break;
		case Qt::Key_F3:			if ( appM )	sendMe += "[13~";												break;
		case Qt::Key_F4:			if ( appM )	sendMe += "[14~";												break;
		case Qt::Key_F5:			if ( appM )	sendMe += "[15~";												break;
		case Qt::Key_F6:			if ( appM )	sendMe += "[16~";												break;
		case Qt::Key_F7:			if ( appM )	sendMe += "[17~";												break;
		case Qt::Key_F8:			if ( appM )	sendMe += "[18~";												break;
		case Qt::Key_F9:			if ( appM )	sendMe += "[19~";												break;
		case Qt::Key_F10:			if ( appM )	sendMe += "[20~";												break;
		case Qt::Key_F11:			if ( appM )	sendMe += "[21~";												break;
		case Qt::Key_F12:			if ( appM )	sendMe += "[22~";												break;
		default:
			bSendText = false;
		}

		if (bSendText)
		{
			emit textToSend( sendMe);
			pEvent->accept();
			return;
		}
	}
	else		//shift key is being pressed
	{
		if ( pEvent->key() == Qt::Key_Insert)	//SHIFT + INSERT pressed, paste text from clipboard
		{
			slotPasteTextFromClipboard();
			pEvent->accept();
			return;
		}
		else if ( pEvent->modifiers().testFlag(Qt::ControlModifier) ) //SHIFT + CTRL pressed
		{
			if ( pEvent->key() == Qt::Key_F )
			{
				if ( mp_mainWin->isFullScreen() )
				{
					mp_mainWin->statusBar()->show();
					mp_mainWin->menuBar()->show();
					mp_tabbinator->toggleTabBarVisibility(true);
					mp_mainWin->showNormal();
				}
				else
				{
					//Hide the statusbar and menu bars
					mp_mainWin->statusBar()->hide();
					mp_mainWin->menuBar()->hide();
					mp_tabbinator->toggleTabBarVisibility(false);
					mp_mainWin->showFullScreen();
				}

				pEvent->accept();
				return;
			}
		}
	}

	//Normally when we get here we have text to send over, like user typed in stuff
	if ( ! pEvent->text().isEmpty() )
	{
		emit textToSend( pEvent->text() );
		pEvent->accept();		//We just ate the keypress event
		return;	
	}

	pEvent->ignore();
}

//////////////////////////////////////////////////////////////////////
bool
SSHWidget::event(QEvent *pEvent)
{
	if (pEvent->type() == QEvent::KeyPress)
	{
		QKeyEvent *pQKeyEvent = static_cast<QKeyEvent *>(pEvent);
		if (pQKeyEvent->key() == Qt::Key_Tab)
		{
			emit textToSend( QString(CHAR_HORIZONTAL_TAB) );
			return true;
		}
	}
	return QWidget::event(pEvent);
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::dragEnterEvent(QDragEnterEvent *pEvent)
{
	this->setBackgroundRole(QPalette::Highlight);
	pEvent->acceptProposedAction();

}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::dropEvent(QDropEvent *pEvent)
{
	const QMimeData *pMimeData = pEvent->mimeData();

	if (pMimeData->hasUrls())
	{
		QList<FileFolderTransferInfo> fileAndFolderList;
		QList<QUrl> urlList = pMimeData->urls();
		for (int i = 0; i < urlList.size(); i++)
		{
			FileFolderTransferInfo FFTInfo;

			FFTInfo.localPath = urlList[i].path().mid(1);
			QFileInfo fInfo(FFTInfo.localPath);
			if ( fInfo.isFile() )
				FFTInfo.isFolder = false;
			else
				FFTInfo.isFolder = true;

			fileAndFolderList.append( FFTInfo);
		}

		FileTransferDlg FTD( &fileAndFolderList, m_cwd, mp_connectionDetails->username );
		if ( FTD.exec() == QDialog::Rejected)
			return;

		FTD.hide();

		//Create our transfer request
		TransferRequest request;

		if ( ! FTD.m_recursivelyCopyFolders->isChecked() )
			request.isRecursive = false;		//default is true

		if ( ! FTD.m_makeDirsExecutable->isChecked() )
			request.makeDIRsExecutable = false;		//default is true;

		request.hostAddress		= mp_connectionDetails->hostAddress;
		request.hostPort			= mp_connectionDetails->hostPort;
		request.username			= mp_connectionDetails->username;
		request.password			= mp_connectionDetails->password;
		request.fileFolderList	= fileAndFolderList;
		request.filePermissions	= FTD.getPermissions();
		request.remoteBasePath	= FTD.m_destination->text();
		if (request.remoteBasePath[ request.remoteBasePath.size()-1] != '/')
			request.remoteBasePath.append( '/');

		//Find the location that marks the start of our "base path"
		QString temp = request.fileFolderList[0].localPath;
		int pos;
		for (pos = temp.size()-1; pos >= 0; pos--)
		{
			if ( temp[pos] == '/')
				break;
		}

		//Now that we know the size of the local base path, rip it off and replace
		//it with its remote counter-part
		for (int i = 0; i < request.fileFolderList.size(); i++)
		{
			temp = FTD.m_destination->text();
			temp.append( request.fileFolderList[i].localPath.mid(pos+1) );
			request.fileFolderList[i].remotePath = temp;
		}

		//Request built, send it off!
		emit fileTransfer(request);
	}
	
	pEvent->acceptProposedAction();
}

//////////////////////////////////////////////////////////////////////
void
SSHWidget::resizeEvent ( QResizeEvent * pEvent )
{
	if ( ! m_allowResize)
		return;			//We aren't yet connected to the SSH server, don't try resizing

	if (mp_resizeTimer->isActive())
		mp_resizeTimer->stop();
		
	mp_resizeTimer->start( 100);
}

//////////////////////////////////////////////////////////////////////
