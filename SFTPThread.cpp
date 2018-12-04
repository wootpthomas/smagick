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
 Includes
*************************/
#include "SFTPThread.h"

/***********************
 Qt Includes
*************************/


////////////////////////////////////////////////////////////////////////////////////
SFTPThread::SFTPThread(
		const QString remoteHostAddress,
		const QString username,
		const QString password,
		int port):
m_remoteHostAddress(remoteHostAddress),
m_username(username),
m_password(password),
m_port(port),
m_processingQueue(false),
mp_sftp(NULL)
{
	
}

////////////////////////////////////////////////////////////////////////////////////
SFTPThread::~SFTPThread(void)
{
}

////////////////////////////////////////////////////////////////////////////////////
SFTP * const
SFTPThread::getSFTPPtr()
{
	QMutexLocker lock( &m_mutex);
	SFTP *pSFTP = mp_sftp;
	return pSFTP;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SFTPThread::isIdle()
{
	QMutexLocker lock( &m_mutex);
	if ( workQueue.size() == 0 && ! m_processingQueue)
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////
void
SFTPThread::run()
{
	mp_sftp = new SFTP(m_remoteHostAddress, m_username, m_password, m_port);
	if ( ! mp_sftp->init() )
	{
		printf("Worker SFTPThread could not create a valid SFTP object\n");
		return;
	}

	printf("Worker SFTPThread running\n");

	//Let the world know we are ready for jobs
	emit readyToWork(this);

	//Run our event loop
	exec();

	delete mp_sftp;
	printf("Worker SFTPThread QUIT!\n");
}

////////////////////////////////////////////////////////////////////////////////////
void
SFTPThread::addNewJob(SFTPTransferRequest *pRequest)
{
	QMutexLocker lock( &m_mutex);
	workQueue.enqueue(pRequest);
}

////////////////////////////////////////////////////////////////////////////////////
void
SFTPThread::slotProcessQueue(SFTPThread* pSFTP)
{
	if (pSFTP != this)		//Were we supposed to process events?
		return;

	SFTPTransferRequest *pRequest = NULL;
	int tasks;

	m_mutex.lock();
	m_processingQueue = true;
	tasks = workQueue.size();
	m_mutex.unlock();

	while (tasks)
	{
		bool result = false;

		//Get a job out of the queue
		m_mutex.lock();
		if ( workQueue.size())
			pRequest = workQueue.dequeue();
		m_mutex.unlock();

		switch (pRequest->requestType){
			case COPY_FILE_TO_REMOTE_HOST:
				//Transfer each file in the list
				for (int i = 0; i < pRequest->localItems.size(); i++)
				{
					result = mp_sftp->transferFileToRemote(
						pRequest->localItems.at(i),
						pRequest->remoteItems.at(i),
						pRequest->filePermissions);
				}
				break;
			case MOVE_FILE:
			case DELETE_FILE:
			case CREATE_DIR:
				for (int i = 0; i < pRequest->remoteItems.size(); i++)
				{
					result = mp_sftp->createDir(
						pRequest->remoteItems.at(i),
						pRequest->filePermissions);
				}
				break;
			case DELETE_DIR:
			case MOVE_DIR:
			default:
				printf("Unhandled transfer request. The code isn't in place for it.\n");
				break;
		}

		//Cleanup the job
		delete pRequest;
		pRequest = NULL;

		m_mutex.lock();
		tasks = workQueue.size();
		m_mutex.unlock();
	}

	m_mutex.lock();
	m_processingQueue = false;
	m_mutex.unlock();
}