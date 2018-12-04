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


/*****************
 Qt Includes
******************/
#include <QThread>
#include <QList>
#include <QMutex>
//#include <QSemaphore>

/******************
 Includes
*******************/
#include <libssh2.h>
#include "SSHConnection.h"

/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


class PollWorkerThread :
	public QThread
{
public:
	PollWorkerThread(SSHConnection *pSSH, int pollingIntervalIn_usec);
	~PollWorkerThread(void);

	/** THREAD SAFE
	 * This lets us add in a new channel that this object will poll
	 * frequently. When a channel has data, the value emitted is the given
	 * int in the channelNumToEmit. */
	void addChannelToPoll(LIBSSH2_CHANNEL *pChannel);

	/** THREAD SAFE
	 * Lets us remove a channel to poll from our list */
	void removeChannel(LIBSSH2_CHANNEL* pChannel);

	/* Starts our polling frenzy */
	void run();

	/** THREAD SAFE
	 * Blocks until the PollWorkerThread is 100% ready to go */
	bool readyToWork() { return m_bInitialized; }

	/** THREAD SAFE
	 * Tells the thread to stop whatever-in-the-hell its doing and die */
	void quit();

private:

	QList<LIBSSH2_CHANNEL * >
		m_channelList;			//Holds a list of the channels that we poll
	
	int
		m_pollingInterval;	/*The polling interval in msec. This is the amount
									 * of time that we sleep in between polling. */
	bool
		m_bKeepRunning,		//Used to stop this thread from running
		m_bInitialized;		//Set to true if the thread has all it needs to start

	QMutex
		m_mutex;

	SSHConnection
		*mp_SSH;				//Used to call our library and read data from a channel
};
