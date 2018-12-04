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
#include <QThread>
#include <QMutexLocker>
#include <QQueue>

/***********************
 Other Includes
*************************/
#include "SFTP.h"
#include "DataTypes.h"

///*****************
// * Memory Leak Detection
//*****************/
//#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
//	#define _CRTDBG_MAP_ALLOC
//	#include <stdlib.h>
//	#include <crtdbg.h>
//
//	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
//	#define new DEBUG_NEW
//#endif


class SFTPThread :
	public QThread
{
	Q_OBJECT

public:
	SFTPThread(
		const QString remoteHostAddress,
		const QString username,
		const QString password,
		int port);

	~SFTPThread(void);

	/* Start this thread up so that it can do file transfers and stuff */
	void run();

	/** THREAD SAFE ***
	 * returns a pointer to our internal SFTP object */
	SFTP * const getSFTPPtr();

	/** THREAD SAFE
	 * Returns the status of the worker thread. If true, our queue is empty
	 * and we are not processing any transfer requests */
	bool isIdle();

	/** THREAD SAFE ***
	 * Adds a new task to our transfer queue. */
	void addNewJob(SFTPTransferRequest *pRequest);

signals:
	/* Lets the world know that this thread is ready to do
	 * transfer requests */
	void readyToWork(SFTPThread*);

public slots:
	/* Force our worker to process its queue */
	void slotProcessQueue(SFTPThread*);

private:

	SFTP
		*mp_sftp;

	QQueue<SFTPTransferRequest *>
		workQueue;

	QMutex
		m_mutex;

	QString 
		m_remoteHostAddress,
		m_username,
		m_password;

	int
		m_port;

	bool
		m_processingQueue;
};
