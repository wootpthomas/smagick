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

/*******************
 Qt Includes
*******************/


/*******************
 Includes
*******************/
#include "PollWorkerThread.h"


////////////////////////////////////////////////////////////////////////////////////
PollWorkerThread::PollWorkerThread(SSHConnection *pSSH, int pollingIntervalIn_usec):
m_pollingInterval( pollingIntervalIn_usec),
m_bKeepRunning(true),
mp_SSH( pSSH),
m_bInitialized( false)
{
	if ( mp_SSH)
		m_bInitialized = true;
}

////////////////////////////////////////////////////////////////////////////////////
PollWorkerThread::~PollWorkerThread(void)
{
}

////////////////////////////////////////////////////////////////////////////////////
void 
PollWorkerThread::addChannelToPoll(LIBSSH2_CHANNEL *pChannel)
{
	QMutexLocker lock( &m_mutex);

	//Search through the list. If the channel is already in it, don;t add it again
	for (int i = 0; i < m_channelList.size(); i++)
		if ( m_channelList[i] == pChannel)
			return;

	//Channel isn;t in our list
	m_channelList.append( pChannel);
	printf("Added channel to poll\n");
}

////////////////////////////////////////////////////////////////////////////////////
void 
PollWorkerThread::removeChannel(LIBSSH2_CHANNEL* pChannel)
{
	QMutexLocker lock( &m_mutex);
	for (int i = m_channelList.size()-1; i >= 0 ; i--)
	{
		if (m_channelList[i] == pChannel)
			m_channelList.removeAt( i);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void 
PollWorkerThread::run()
{
	if ( ! m_bInitialized)
		return;

	LIBSSH2_POLLFD pfd;
	pfd.type = LIBSSH2_POLLFD_CHANNEL;

	while(1)
	{
		/* First we check all of our channels for data */
		m_mutex.lock();
		for(int i = 0; i < m_channelList.size(); i++)
		{
			if ( ! m_channelList[i] )		//Sanity check
				continue;

			#ifdef WIN32
				pfd.fd.channel = m_channelList[i];
				if ( libssh2_poll( &pfd, 1, 1) )
					mp_SSH->slotReadFromChannel(m_channelList[i]);
			#else
					mp_SSH->slotReadFromChannel(m_channelList[i]);
			#endif
		}

		/* Finally, we see if we are still running. If we are we sleep, otherwise we quit */
		if (m_bKeepRunning)
		{
			m_mutex.unlock();
			usleep(m_pollingInterval);
		}
		else
		{
			m_mutex.unlock();
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
void 
PollWorkerThread::quit()
{
	QMutexLocker lock( &m_mutex);
	m_bKeepRunning = false;
}
