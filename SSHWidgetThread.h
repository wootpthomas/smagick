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
#include <QThread>
#include <QMutex>


/***********************
 Includes
***********************/
#include "DataTypes.h"
#include "ConsoleChar.h"

/***********************
 Forward Class declarations
***********************/
class SSHConnection;
class SSHWidget;
class DecoderThread;


/*****************
 * Memory Leak Detection
 *****************/
//#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
//	#define _CRTDBG_MAP_ALLOC
//	#include <stdlib.h>
//	#include <crtdbg.h>
//
//	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
//	#define new DEBUG_NEW
//#endif



class SSHWidgetThread :
	public QThread
{
	Q_OBJECT

public:
	SSHWidgetThread(
		ConnectionDetails * const pConnectionDetails,
		SSHWidget * const pSSHWidget,
		int consoleCols,
		int consoleRows);

	~SSHWidgetThread(void);

	/* Lets this thread loose to connect up and get going */
	void run();

	/** THREAD SAFE
	 * Returns a RenderInfo object so our render widget can draw the screen */
	RenderInfo* getRenderInfo();

	/** THREAD SAFE
	 * Returns the status of keypad keys */
	bool areKeypadKeysInAppMode();

	/** THREAD SAFE
	 * Returns the status of cursor keys */
	bool areCursorKeysInAppMode();

	/** THREAD SAFE
	 * Toggles debug output from the Decoder */
	void toggleDebugPrinting(bool value);

	/** THREAD SAFE
	 * Overloading the QThread quit() so that we can bomb out early if we
	 * haven;t yet gone into our event loop */
	void quit();

signals:
	/* This is normally only used when we are sending out the initial
	 * login info to the console */
	void sendRawTextToConsole(QString text);

	/* Sent out when our SSHWidgetThread has something for the decoder to decode */
	void decodeData(char *pData, int pDataSize);

	/* Sent out when our SSHConnection status changed and we want to
	 * alert the user */
	void connectionStatusChanged( StatusChangeEvent e);

	/* Sent so that our SSHWidget can prompte the user for the correct password */
	void loginFailedNeedNewPWD();

	/* Sent when everything is up and alive. This is mainly used to let our SSHWidget
	 * know when things are working so it can send a resize event over and size the
	 * console for first use. */
	void readyToWork();

public slots:

	/* Ran when the SSHConneciton library fails to login with its current password. 
	 * So we just emit a signal to get a new password */
	void slotRequestCorrectPassword();

	/* The console's will use this slot to send their data. The second parameter
	 * is the keypress responsible for causing the data to be sent. */
	void slotSendConsoleData(QString dataToSend);

	/* Slot is called from SSHWidget whenever it detects that we need
	 * to change the size of the console */
	void slotResizeConsole(int width, int height, int widthPx, int heightPx);

	/* Called when loggin in for the first time, or when we are retrying a login attempt
	 * with a new password */
	void slotLoginAndCreateChannel(QString newPassword = QString("") );

private slots:

	void slotSSHDataAvailable(char *pData, int pDataSize, int channelNumber);

	//Called when the console session has ended. Sends a CS_shutdownNormal event
	void slotClose();

private:
	/*********************
	Private Functions
	*********************/
	/* Creates any tunnels specified in the connection object */
	void createSSHTunnels();

	/* Cleans up any items created in the run() */
	void cleanup();

	/*********************
	Private Variables
	*********************/
	int
		m_consoleWidth,			//Holds the width of our console
		m_consoleHeight,			//Holds the height of our console
		m_consoleChannelNum;		//Channel number for the user's SSH console

	bool
		m_alreadySentTimeout,		//Set to true after we send our disconnect signal
		m_bQuit;						/* Set to true to help us bomb out early if we haven;t yet
										 * reached the Qt event loop */
	QMutex
		m_mutex;					//Mutex to help us lock data

	SSHConnection
		*mp_SSH;					//Pointer to the SSHConnection object for the user's SSH console

	DecoderThread
		*mp_decoderThread;	/*Used to decode the ssh data and manage the drawing of
									 * all console text */

	/******************** Do NOT use except for signal/slot connections*****************
	 * NOT THREAD SAFE!
	 * These are pointers to widgets that live in another thread, calling their functions
	 * is not safe. ONLY use these for connecting signal/slots. 
	 **********************************************************************************/
	ConnectionDetails
		* const mp_connectionDetails;		//Holds the connection details for this object

	SSHWidget
		 * const mp_SSHWidget;		//Pointer to use for signal/slot connection ONLY
};
