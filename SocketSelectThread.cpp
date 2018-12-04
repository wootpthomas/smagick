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
#ifdef WIN32
	#include "winsock2.h"
	//Redefine the close() function on windows so we don't break on linux
	#define close(SOCKET)				closesocket(SOCKET)
#else
	#include <sys/select.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#define SOCKET_ERROR -1
#endif

#include "SocketSelectThread.h"


/*******************
 #Defines
*******************/
#define READ_BUF_SIZE 4096

////////////////////////////////////////////////////////////////////////////////////
SocketSelectThread::SocketSelectThread(SSHConnection * const pSSH):
m_bKeepRunning(true),
mp_SSH(pSSH)
{
	mp_semaWhore = new QSemaphore(1);
	mp_semaWhore->acquire();
}

////////////////////////////////////////////////////////////////////////////////////
SocketSelectThread::~SocketSelectThread(void)
{
	if ( mp_semaWhore)
	{
		delete mp_semaWhore;
		mp_semaWhore = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::addSocketToPoll(int sock, LIBSSH2_CHANNEL * const pChannel)
{
	SocketListItem item;
	item.sock		= sock;
	item.pChannel	= pChannel;

	QMutexLocker lock( &m_mutex);
	//Search through the list. If the channel is already in it, don;t add it again
	for (int i = 0; i < m_newConnectionListenList.size(); i++)
		if ( m_newConnectionListenList[i].pChannel == pChannel)
			return;

	m_newConnectionListenList.append( item);
	printf("Added socket to poll\n");
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::removeListenSocket(int sock)
{
	QMutexLocker lock( &m_mutex);
	for (int i = m_newConnectionListenList.size()-1; i >= 0 ; i--)
	{
		if (m_newConnectionListenList[i].sock == sock)
			m_newConnectionListenList.removeAt( i);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::run()
{
	mp_semaWhore->release();

	while(1)
	{
		checkForNewConnections();
		checkForSocketData();

		/* Finally, we see if we are still running. If we are we sleep, otherwise we quit */
		m_mutex.lock();
		if ( ! m_bKeepRunning)
		{
			m_mutex.unlock();
			break;
		}
		m_mutex.unlock();
	}

	//Close any open sockets we have
	for (int i = m_socketForwardList.size()-1; i >= 0 ; i--)
		close( m_socketForwardList[i].sock );
}

////////////////////////////////////////////////////////////////////////////////////
void
SocketSelectThread::readyToWork()
{
	mp_semaWhore->acquire();
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::quit()
{
	QMutexLocker lock( &m_mutex);
	m_bKeepRunning = false;
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::checkForNewConnections()
{
	//The timeout is set to 0, we will return immediately if the socket
	// has no data to be read
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	/* Check all the sockets for data */
	int highestSockNum = -1;

	/* Create and clear out our fd_set */
	fd_set readfds;
	FD_ZERO( &readfds);

	m_mutex.lock();
	if ( m_newConnectionListenList.size() == 0)
	{
		m_mutex.unlock();
		usleep( 500);			//We have no connections, sleep and return
		return;
	}

	/* Set our fd_set and find highest FD number */
	for (int i = 0; i < m_newConnectionListenList.size(); i++)
	{
		if (m_newConnectionListenList[i].sock > highestSockNum)
			highestSockNum = m_newConnectionListenList[i].sock;

		//Add this file descriptor to our list of FDs to check
		FD_SET( m_newConnectionListenList[i].sock, &readfds);
	}

	int result = 0;
	result = select(
		highestSockNum + 1,
		&readfds,
		NULL,
		NULL,
		&timeout);

	if (result < 0)
		printf("Select in the SocketListerner thread encountered an error: %d\n", result);
	else if ( result > 0)
	{
		for (int i = 0; i < m_newConnectionListenList.size(); i++)
		{
			if (FD_ISSET(m_newConnectionListenList[i].sock, &readfds) )
				handleNewConnection( &m_newConnectionListenList[i] );
		}
	}
	m_mutex.unlock();
	//else, nothing to read
}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::checkForSocketData()
{
	/* The timeout is set to 500usec, we give connected sockets priority
	 * over new connections. Plus this allows us to let the thread "sleep"
	 * for a little bit */
	// has no data to be read
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;

	/* Check all the sockets for data */
	int highestSockNum = -1;

	/* Create and clear out our fd_set */
	fd_set readfds;
	FD_ZERO( &readfds);

	/* Set our fd_set and find highest FD number */
	m_mutex.lock();
	if ( m_socketForwardList.size() == 0)
	{
		m_mutex.unlock();
		usleep( 500);			//We have no connections, sleep and return
		return;
	}

	for (int i = 0; i < m_socketForwardList.size(); i++)
	{
		if (m_socketForwardList[i].sock > highestSockNum)
			highestSockNum = m_socketForwardList[i].sock;

		//Add this file descriptor to our list of FDs to check
		FD_SET( m_socketForwardList[i].sock, &readfds);
	}

	int result = 0;
	result = select(
		highestSockNum + 1,
		&readfds,
		NULL,
		NULL,
		&timeout);

	if (result < 0)
		printf("Select in the SocketListerner thread encountered an error: %d\n", result);
	else if ( result > 0)
	{
		for (int i = 0; i < m_socketForwardList.size(); i++)
		{
			if (FD_ISSET(m_socketForwardList[i].sock, &readfds) )
			{
				//initialize variables
				char data[READ_BUF_SIZE];
				int length = 0;

				do {
					//Read in data from socket
					length = recv(m_socketForwardList[i].sock, data, READ_BUF_SIZE, 0);
					
					if ( length > 0)
					{
						//Write the data to the remote host
						printf("----> Read %d bytes sock %d ---> channel\n", length, m_socketForwardList[i].sock);
						mp_SSH->writeToChannel(m_socketForwardList[i].pChannel, data, length);
					}
					else if ( length == SOCKET_ERROR)
					{
#ifdef WIN32						
						switch ( WSAGetLastError() )
						{
						case WSAEWOULDBLOCK:
							//printf("Would block\n");
							break;
						default:
							printf( "Socket error %d!\n", WSAGetLastError() );
						}
#endif
					}
					else
					{
						printf("Client has disconnected from tunnel\n", length);
						m_socketForwardList.removeAt( i);
						m_mutex.unlock();
						return;
					}
				} while (length > 0);
			}
		}
	}
	m_mutex.unlock();
	//else, nothing to read


}

////////////////////////////////////////////////////////////////////////////////////
void 
SocketSelectThread::handleNewConnection(SocketListItem * const pItem)
{
	struct sockaddr_in *pClientAddress = new sockaddr_in;
	int len = sizeof(*pClientAddress);
#ifdef WIN32
	int fd = accept( pItem->sock, (sockaddr*) pClientAddress, &len);
#else
	int fd = accept( pItem->sock, (sockaddr*) pClientAddress, (socklen_t*)&len);
#endif
	if ( fd < 0)
	{
		printf("Accepting an incoming socket errored with code %d\n", fd);
		return;
	}

	//We tell the library: "When you get data on this channel, stuff it into this socket
	bool forwardingSock = mp_SSH->registerTunnelConnection(pItem->pChannel, fd);
	if (forwardingSock)
	{
		/* We successfully setup a connection with the client that we are helping
		 * to tunnel under the wall and registered the client with our SSHConnection
		 * object. So when our SSH obj gets data on the channel for the client, it
		 * will automagically write it to the 'fd' socket. What we need to do is
		 * listen for any messages from the client and relay them through the library.
		 * So add our 'fd' to our connectedList and begin listening for messages
		 * to relay */
		SocketListItem sockItem;
		sockItem.pChannel = pItem->pChannel;
		sockItem.sock		= fd;
		printf("Listening for data to forward on socket %d\n", fd);
		
		//The mutex was locked before entering this function, so its all good
		m_socketForwardList.append( sockItem);
	}
}
