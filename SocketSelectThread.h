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
#include <QSemaphore>

/******************
 Includes
*******************/
#include <libssh2.h>
#include "SSHConnection.h"


class SocketSelectThread :
	public QThread
{
	//Struct used in our socket list
	struct SocketListItem{
		int
			sock;
		
		LIBSSH2_CHANNEL
			*pChannel;
	};

public:
	SocketSelectThread(SSHConnection * const pSSH);
	~SocketSelectThread(void);

	/** THREAD SAFE
	 * This adds a socket for us to poll. The channel pointer is used when
	 * we accept a connection on this socket; we tell our SSH library that when
	 * it gets data on this channel, to use the registered socket */
	void addSocketToPoll(int sock, LIBSSH2_CHANNEL * const pChannel);

	/** THREAD SAFE
	 * Removes a socket from our list of incoming connection sockets */
	void removeListenSocket(int sock);

	/* Starts our polling frenzy */
	void run();

	/* Blocks until the thread is ready to return its status */
	void readyToWork();

	/** THREAD SAFE
	 * Tells the thread to stop whatever-in-the-hell its doing and die */
	void quit();

private:
	/**********************
	 Private Functions
	**********************/
	/* Listens on sockets in our m_socketList for new incoming
	 * connections. When a new connection comes in, we add it to our
	 * m_activeSocket list */
	void checkForNewConnections();

	/* Listens on the activeSockets for data. When we get it, we write
	 * it to the remote SSH server via the SSHConnection object */
	void checkForSocketData();

	/* Does whatever we need to do to service the new connection */
	void handleNewConnection(SocketListItem * const item);


	/**********************
	 Private Vars
	**********************/
	QList<SocketListItem>
		m_newConnectionListenList; //List of sockets to listen for incoming connections

	QList<SocketListItem>
		m_socketForwardList; /*List of sockets that when we get data on them, we forward
									 * that data to the associated channel */
	bool
		m_bKeepRunning;		//Used to stop this thread from running

	QMutex
		m_mutex;

	QSemaphore
		*mp_semaWhore;		//Used to provide blocking functionallity on our readyToWork()

	SSHConnection
		* const mp_SSH;	/*Pointer to our ssh object so that we can register
								 * socket connections. */
								
};
