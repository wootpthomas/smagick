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
*************************/
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QApplication>

/***********************
 Other Includes
*************************/
#include "TransferManager.h"
#include "SFTP.h"
#include "SFTPThread.h"
#include "FileTransferProgressDlg.h"

/* Our Transfer Manager will keep workers alive for a period of time
 * after they have completed all of their tasks. However, we periodically
 * check to see what workers are idle every so often. If we check a worker and
 * it is idle, we toggle a flag in our SFTP_roster list for that worker. The
 * next time we check for idle workers, if a worker is already listed as idle,
 * then this is the 2nd time they were idle in a row. Get rid of the lazy worker */
#define MSEC_BEFORE_IDLE_WORKER_CLEANUP 120000  //Get rid of idle workers after 1 minute


//////////////////////////////////////////////////////////////////////
TransferManager::TransferManager(FileTransferProgressDlg *pFTPD):
QThread(),
mp_FTPD(pFTPD)
{
	//Register our type (if not registered) so we can send our custom object across in a signal
	int typeNum = QMetaType::type("TransferRequest");
	if ( ! typeNum)
		qRegisterMetaType<TransferRequest>("TransferRequest");
}

//////////////////////////////////////////////////////////////////////
TransferManager::~TransferManager(void)
{
	//Time to shutdown all our worker threads
	for (int i = m_SFTP_roster.size()-1; i >= 0; i--)
		m_SFTP_roster[i].mp_sftpWorkerThread->quit();

	//Now wait for each thread to quit
	for (int i = m_SFTP_roster.size()-1; i >= 0; i--)
	{
		printf("Waiting on worker thread %d to die\n", i);
		m_SFTP_roster[i].mp_sftpWorkerThread->wait();

		delete m_SFTP_roster[i].mp_sftpWorkerThread;
		m_SFTP_roster[i].mp_sftpWorkerThread = NULL;
	}

	//Remove all items from the list
	m_SFTP_roster.clear();
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::run()
{
	//Setup our timer to get rid of idle workers
	mp_jackTheRipper = new QTimer();
	mp_jackTheRipper->setSingleShot(false);	//Reocurring
	mp_jackTheRipper->setInterval(MSEC_BEFORE_IDLE_WORKER_CLEANUP / 2);
	connect (mp_jackTheRipper, SIGNAL(timeout()),
		this, SLOT(slotStalk()));

	//Let this thread get to work
	exec();

	if (mp_jackTheRipper)
	{
		delete mp_jackTheRipper;
		mp_jackTheRipper = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::slotFileTransfer(TransferRequest request)
{
	SFTPThread *pSFTPThread = NULL;
	mp_jackTheRipper->start();

	//Look in our list 
	//Make a new SFTP connection if needed
	bool useExistingConnection = false;
	for (int i = 0 ; i < m_SFTP_roster.size(); i++)
	{
		if ( m_SFTP_roster[i].m_remoteHostAddress == request.hostAddress &&
			m_SFTP_roster[i].m_port == request.hostPort &&
			m_SFTP_roster[i].m_username == request.username)
		{
			useExistingConnection = true;
			pSFTPThread = m_SFTP_roster[i].mp_sftpWorkerThread;
			m_SFTP_roster[i].m_bIsIdle = false;
			break;
		}
	}

	//If we can't use an existing connection, make a new worker
	if ( ! useExistingConnection)
	{
		SFTP_Info worker;
		worker.m_remoteHostAddress		= request.hostAddress;
		worker.m_port						= request.hostPort;
		worker.m_username					= request.username;
		worker.m_bIsIdle					= false;
		worker.mp_sftpWorkerThread		= new SFTPThread(
			request.hostAddress,
			request.username,
			request.password,
			request.hostPort);

		m_SFTP_roster.append(worker);
		pSFTPThread = worker.mp_sftpWorkerThread;

		//Connect up important signals
		connect(pSFTPThread, SIGNAL(finished()),
			this, SLOT(slotSFTPWorkerQuit()));
		connect(pSFTPThread, SIGNAL(terminated()),
			this, SLOT(slotSFTPWorkerQuit()));
		connect(pSFTPThread, SIGNAL(readyToWork(SFTPThread *)),
			this, SLOT(slotConnectWorker(SFTPThread *)));

		pSFTPThread->start();
		//IMPORTANT: Let's change the thread affinity to events are handled by its event loop
		pSFTPThread->moveToThread(pSFTPThread);
	}

	//Give our request to the worker
	addJobsToWorker(request, pSFTPThread);
	emit getToWorkYouLazyShit(pSFTPThread);
	printf("TransferMAnager gave work a job to do...\n");
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::slotSFTPWorkerQuit()
{
	for (int i = m_SFTP_roster.size()-1; i >= 0; i--)
	{
		if ( m_SFTP_roster[i].mp_sftpWorkerThread->isFinished() ||
			! m_SFTP_roster[i].mp_sftpWorkerThread->isRunning() )
		{
			//TODO: Check this for memory leaks
			m_SFTP_roster.removeAt(i);
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::slotConnectWorker(SFTPThread *pSFTPThread)
{
	//Connect up the SFTP object's stuff
	SFTP *pSFTP = pSFTPThread->getSFTPPtr();
	if ( ! pSFTP)
		return;

	//Connect the pSFTP object's signals to our TransferManagerDlg class's slots
	//For now, we connect it up to the MainWindow's FileProgressDlg
	connect( pSFTP, SIGNAL(transferStarted(QString, QString, qint64)),
		mp_FTPD, SLOT(slotNewTransfer(QString, QString, qint64)));

	connect( pSFTP, SIGNAL(transferProgress(qint64, QString)),
		mp_FTPD, SLOT(slotUpdateProgress(qint64, QString)));

	connect( pSFTP, SIGNAL(transferCompleted(QString)),
		mp_FTPD, SLOT(slotTransferCompleted(QString)));

	connect(this, SIGNAL(getToWorkYouLazyShit(SFTPThread *)),
		pSFTPThread, SLOT(slotProcessQueue(SFTPThread *)));

	//Tell our SFTPThread to get to work
	emit getToWorkYouLazyShit(pSFTPThread);
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::slotStalk()
{
	
	if ( ! m_SFTP_roster.size() )
	{
		mp_jackTheRipper->stop();
		return;
	}
	
	printf("Jack roams the dark streets of London...\n");
	for (int i = m_SFTP_roster.size()-1; i >= 0; i--)
	{
		if ( m_SFTP_roster[i].m_bIsIdle && ! m_SFTP_roster[i].m_bHasPoliceEscort)
		{
			m_SFTP_roster[i].mp_sftpWorkerThread->quit();
			m_SFTP_roster[i].mp_sftpWorkerThread->wait();
			m_SFTP_roster[i].mp_sftpWorkerThread = NULL;

			m_SFTP_roster.removeAt(i);
		}
		else 
		{
			if ( m_SFTP_roster[i].mp_sftpWorkerThread->isIdle() )
			{
				m_SFTP_roster[i].m_bIsIdle = true;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::addJobsToWorker(TransferRequest &request, SFTPThread *pSFTPThread)
{
	//Create any needed file transfer requests
	SFTPTransferRequest *pRequest = NULL;
	for (int i = 0; i < request.fileFolderList.size(); i++)
	{
		if ( ! request.fileFolderList[i].isFolder )
		{
			if ( ! pRequest)
			{
				pRequest = new SFTPTransferRequest();
				pRequest->requestType = COPY_FILE_TO_REMOTE_HOST;
				pRequest->filePermissions = request.filePermissions;
			}

			pRequest->localItems.append( request.fileFolderList[i].localPath );
			pRequest->remoteItems.append( request.fileFolderList[i].remotePath );
		}
	}

	if (pRequest)
	{
		pSFTPThread->addNewJob( pRequest);
		pRequest = NULL;
	}

	//Now we handle folders. 
	if ( request.isRecursive )	//Handle them and their contents recursively
	{
		for (int i = 0; i < request.fileFolderList.size(); i++)
		{
			if ( request.fileFolderList[i].isFolder )
			{
				addFilesAndFoldersRecursively(
					pSFTPThread,
					request.makeDIRsExecutable,
					request.filePermissions,
					request.fileFolderList[i].localPath,
					request.fileFolderList[i].remotePath);
			}
		}
	}
	else		//Only create folder, do not copy contents
	{
		for (int i = 0; i < request.fileFolderList.size(); i++)
		{
			if ( request.fileFolderList[i].isFolder )
			{
				if ( ! pRequest)
				{
					pRequest = new SFTPTransferRequest();
					pRequest->requestType = CREATE_DIR;
					pRequest->filePermissions = request.filePermissions;
					if (request.makeDIRsExecutable)
						makeDirPermissionsExecutable( request.filePermissions);
				}

				pRequest->localItems.append( request.fileFolderList[i].localPath );
				pRequest->remoteItems.append( request.fileFolderList[i].remotePath );
			}
		}

		if (pRequest)
		{
			pSFTPThread->addNewJob( pRequest);
			pRequest = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::makeDirPermissionsExecutable(long &permissions)
{
	//Set execute permissions as needed for owner, group, others
	permissions |= LIBSSH2_SFTP_S_IXUSR;

	if ( permissions & LIBSSH2_SFTP_S_IRGRP)
		permissions |= LIBSSH2_SFTP_S_IXGRP;

	if ( permissions & LIBSSH2_SFTP_S_IROTH)
		permissions |= LIBSSH2_SFTP_S_IXOTH;
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::addFilesAndFoldersRecursively(
	SFTPThread *pSFTPThread, bool DIRsExecutable, long permissions, QString localPath, QString remotePath)
{
	SFTPTransferRequest *pRequest = NULL;
	
	/* 1) Make a create directory request for this DIR */
	pRequest = new SFTPTransferRequest();
	pRequest->requestType = CREATE_DIR;
	pRequest->filePermissions = permissions;
	if (DIRsExecutable)
		makeDirPermissionsExecutable( pRequest->filePermissions);

	pRequest->remoteItems.append(remotePath);
	pSFTPThread->addNewJob(pRequest);
	pRequest = NULL;


	QDir localDIR( localPath);
	if (localDIR.isReadable() )
	{
		QStringList fileList = localDIR.entryList(QDir::Files, QDir::Name);

		/* 2) Add all files to a transfer request and add to SFTPThread */
		pRequest = new SFTPTransferRequest();
		pRequest->requestType = COPY_FILE_TO_REMOTE_HOST;
		pRequest->filePermissions = permissions;
		pRequest->localItems = fileList;

		//Build a list of remote paths for each local file
		buildRemotePaths(remotePath, fileList, pRequest->remoteItems);

		//Now modify all the files in our list so they contain their full path
		QString newLocalPath = localDIR.absolutePath() + "/";
		for (int i = 0; i < pRequest->localItems.size(); i++)
			pRequest->localItems[i] = newLocalPath + pRequest->localItems[i];

		QString asdf = pRequest->localItems[0];
		pSFTPThread->addNewJob(pRequest);
		pRequest = NULL;

		/* 3) For each folder, add a CreateDIR request
		 *    call addFilesAndFoldersRecursively() for each folder */
		QStringList dirList = localDIR.entryList(QDir::AllDirs, QDir::Name);
		for (int i = 0; i < dirList.size(); i++)
		{
			QDir cd( dirList[i]);
			QString dirName = cd.dirName();
			if (dirName.compare( ".") == 0 || dirName.compare("..") == 0)
				continue;

			addFilesAndFoldersRecursively(
				pSFTPThread,
				DIRsExecutable,
				permissions,
				localPath + "/" + dirName,
				remotePath + "/" + dirName);
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
TransferManager::buildRemotePaths(const QString &remoteBasePath, const QStringList &fileList, QStringList &remoteFileList)
{
	for (int i = 0; i < fileList.size(); i++)
	{
		QString localFile = fileList[i];
		QString remoteDest = remoteBasePath;
		if (remoteDest[remoteDest.size()-1] != '/')
			remoteDest.append( '/');
		remoteDest += localFile;

		remoteFileList.append( remoteDest);
	}
}
