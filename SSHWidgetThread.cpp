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
#include <QMutexLocker>

/***********************
 Includes
***********************/
#include "SSHWidgetThread.h"
#include "SSHWidget.h"
#include "SSHConnection.h"
#include "DecoderThread.h"


////////////////////////////////////////////////////////////////////
SSHWidgetThread::SSHWidgetThread(
		ConnectionDetails * const pConnectionDetails,
		SSHWidget * const pSSHWidget,
		int consoleCols,
		int consoleRows):
mp_connectionDetails(pConnectionDetails),
mp_SSHWidget(pSSHWidget),
m_consoleWidth(consoleCols),
m_consoleHeight(consoleRows),
m_alreadySentTimeout(false),
mp_decoderThread(NULL),
mp_SSH(NULL),
m_bQuit(false)
{

	//Register our type (if not registered) so we can send our custom object across in a signal
	int typeNum = QMetaType::type("StatusChangeEvent");
	if ( ! typeNum)
		qRegisterMetaType<StatusChangeEvent>("StatusChangeEvent");
}

////////////////////////////////////////////////////////////////////
SSHWidgetThread::~SSHWidgetThread(void)
{

}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::run()
{
	if (mp_connectionDetails)
	{
		mp_decoderThread = new DecoderThread(m_consoleWidth, m_consoleHeight);
		mp_decoderThread->start();

		//Change the thread's affinity
		mp_decoderThread->moveToThread(mp_decoderThread);

		//Block till our decoder thread is ready
		mp_decoderThread->isReadyToWork();

		//Inspect the connectionDetails for login type
		if (mp_connectionDetails->loginByUsernamePassword &&
			! mp_connectionDetails->hostAddress.isEmpty() &&
			! mp_connectionDetails->username.isEmpty() )
		{
			/*Cool, attempt to login by username&password
			 * NOTE:
			 *	We will allow the password to be blank. IF this is the
			 * case then the user will be expected to supply it when needed */
			
			mp_SSH = new SSHConnection(
						mp_connectionDetails->hostAddress,
						mp_connectionDetails->username,
						mp_connectionDetails->password,
						mp_connectionDetails->hostPort,
						mp_decoderThread);

			if ( m_bQuit )  //User clicked close tab buton while we were still attempting to connect
			{
				cleanup();
				return;
			}

			/* Connect up our SSHConnection object with this object so that we can
			 * keep track of our connection status in a format that can easliy be
			 * sent, when requested, to our statusbar.
			 * The library will now inform us of its progress and failures ;p */
			connect(mp_SSH, SIGNAL(statusChanged(StatusChangeEvent)),
				mp_SSHWidget, SLOT(slotConnectionStatusChanged( StatusChangeEvent)));

			// Triggered when we fail to write data to the remote host
			connect(this, SIGNAL(connectionStatusChanged(StatusChangeEvent)),
				mp_SSHWidget, SLOT(slotConnectionStatusChanged( StatusChangeEvent)));

			// Let the decoder update the window title
			connect(mp_decoderThread->getDecoderPtr(), SIGNAL(windowTitleChanged(QString)),
				mp_SSHWidget, SLOT(slotWindowTitleChanged(QString)));

			//Connect up the requestCorrectPassword() signal so that we can get new passwords
			// If auth failes
			/* This connects us to the SSHWidget's get new password stuff */
			connect( mp_SSH, SIGNAL(requestCorrectPassword()),
				this, SLOT(slotRequestCorrectPassword()));
			/* Connects the SSHWidgets "here's a new password to try" to our retryLogin stuff */
			connect( mp_SSHWidget, SIGNAL(retryLoginWithPassword(QString)),
				this, SLOT(slotLoginAndCreateChannel(QString)));

			if ( ! mp_SSH->init() )				//Lets initialize our connection object
			{
				cleanup();
				return;
			}

			//Run the rest of our login logic
			slotLoginAndCreateChannel();
		}

		readyToWork();		//Let our SSHWidget know we are live

		/* Let our thread's event loop do its thing. */
		exec();
	}

	cleanup();
}


////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::cleanup()
{
	if (mp_SSH)
	{
		mp_SSH->blockSignals(true);
		delete mp_SSH;
		mp_SSH = NULL;
	}

	if (mp_decoderThread)
	{
		mp_decoderThread->quit();
		mp_decoderThread->wait();
	
		delete mp_decoderThread;
		mp_decoderThread = NULL;
	}
}

////////////////////////////////////////////////////////////////////
RenderInfo*
SSHWidgetThread::getRenderInfo()
{
	QMutexLocker locker( &m_mutex );
	if (mp_decoderThread)
		return mp_decoderThread->getRenderInfo();
	else
		return NULL;
}

////////////////////////////////////////////////////////////////////
bool
SSHWidgetThread::areKeypadKeysInAppMode()
{
	QMutexLocker locker( &m_mutex );
	if (mp_decoderThread)
		return mp_decoderThread->areKeypadKeysInAppMode();

	return false;
}

////////////////////////////////////////////////////////////////////
bool
SSHWidgetThread::areCursorKeysInAppMode()
{
	QMutexLocker locker( &m_mutex );
	if (mp_decoderThread)
		return mp_decoderThread->areCursorKeysInAppMode();

	return false;
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::toggleDebugPrinting(bool value)
{
	QMutexLocker locker( &m_mutex );
	if (mp_decoderThread)
		mp_decoderThread->toggleDebugPrinting(value);
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::quit()
{
	QMutexLocker locker( &m_mutex);
	m_bQuit = true;
	QThread::quit();
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::createSSHTunnels()
{
   int count = mp_connectionDetails->tunnelList.count();
	for (int i = 0; i < mp_connectionDetails->tunnelList.count(); i++)
	{
		if ( ! mp_SSH->createTunnel(
			mp_connectionDetails->tunnelList.at(i).sourcePort,
			mp_connectionDetails->tunnelList.at(i).remoteHost,
			mp_connectionDetails->tunnelList.at(i).remotePort) )
		{
			printf("Could not create the tunnel!\n");
		}
	}
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::slotRequestCorrectPassword()
{
	emit loginFailedNeedNewPWD();
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::slotLoginAndCreateChannel(QString newPassword)
{
	if ( ! newPassword.isEmpty())
		mp_SSH->setPassword(newPassword);

	if ( ! mp_SSH->login() )
	{
		emit sendRawTextToConsole("Login FAILED!\n");
		return;
	}

		/* If we have any tunnels to create, create them now */
	createSSHTunnels();


	//Create a new console on the channel
	m_consoleChannelNum = mp_SSH->newChannel(
		TERMINAL_XTERM,
		//TERMINAL_VT100,
		mp_connectionDetails->envVars,
		m_consoleWidth,
		m_consoleHeight );

	if ( m_consoleChannelNum  < 0)
		return;


}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::slotSSHDataAvailable(char *pData, int pDataSize, int channelNumber)
{
	if ( pDataSize )
	{
		if ( m_consoleChannelNum == channelNumber)
			emit decodeData( pData, pDataSize );
	}
	//else
	//	printf("Blank data recieved!\n");
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::slotSendConsoleData(QString data)
{
	int result = mp_SSH->writeToChannel(m_consoleChannelNum, data);
	//printf("SENT: %s\n", data.toLatin1().constData() );
	
	//TODO: this needs some testing... if result is 0, does that mean we are disconnected?
	//If the result was negative, the connection is dead
	if (result < 0 && ! m_alreadySentTimeout)
	{
		m_alreadySentTimeout = true;
		emit connectionStatusChanged(CS_disconnected); 
	}
}

////////////////////////////////////////////////////////////////////
/* PT Note: I've made the resizing retry automatically if it fails for some
 * reason. Seems like there is a good chance that resizing will fail on the
 * first attempt. Not sure why... Seems like the 2nd attempt normally succeeds */
void
SSHWidgetThread::slotResizeConsole(int width, int height, int widthPx, int heightPx)
{
	bool result = mp_SSH->adjustTerminalSize(m_consoleChannelNum, width, height, widthPx, heightPx);
	if ( ! result)
	{
		//printf("Resizing failed! Retrying...\n");
		result = mp_SSH->adjustTerminalSize(m_consoleChannelNum, width, height, widthPx, heightPx);
	}
	
	if ( ! result)
		printf("Resizing failed!\n");
	//else
	//	printf("Resizing suceeded!\n");
}

////////////////////////////////////////////////////////////////////
void
SSHWidgetThread::slotClose()
{
	//Our console got the logout confirmation. Shut down normally
	mp_SSH->closeChannel( m_consoleChannelNum);
}
