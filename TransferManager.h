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
#include <QUrl>
#include <QList>


/***********************
 Other Includes
*************************/
#include "DataTypes.h"
#include "SFTPThread.h"


/*************************
 Forward Class declaration
**************************/
class SFTPThread;
class FileTransferProgressDlg;
class QTimer;


//
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


/*************************
 Class declaration
**************************/
class TransferManager:
	public QThread
{
	Q_OBJECT

public:
	TransferManager(FileTransferProgressDlg *pFTPD);
	~TransferManager(void);

	/* Let our transfer manger LIVE!!! */
	void run();

signals:
	void getToWorkYouLazyShit(SFTPThread *);

public slots:

	/* Gets called when one of our SSHWidgets gets a file/folder drop event */
	void slotFileTransfer(TransferRequest tr);

	/* Called when one of our SFTP worker threads dies or quits. We go through
	 * our list of workers and check each thread, if a thread is no longer
	 * running, we delete that worker from our list. */
	void slotSFTPWorkerQuit();

private slots:
	/* Called when Jack is on the loose and looking for a victim */
	void slotStalk();

	/* Called when a new worker has finished starting up and needs to
	 * be connected up */
	void slotConnectWorker(SFTPThread *);

private:
	/******************
	 Private Functions
	******************/
	/* Takes in a transfer request object and a thread to do the work. The request
	 * is broken apart into simple request types to be more easily dealt with. This also allows
	 * us to take a large request and start feeding the SFTPWorker jobs while we are still
	 * creating requests */
	void addJobsToWorker(TransferRequest &request, SFTPThread *pSFTPThread);

	/* Changes the permissions for owner, group, others to be executable on the
	 * directory provided each item has read permissions */
	void makeDirPermissionsExecutable(long &permissions);

	/* Given a remote base directory and a list of local files, we build a list
	 * of remote file absolute paths. This is essentially a mapping of what local file
	 * goes where on the remote host. */
	void buildRemotePaths(
		const QString &remoteBasePath,
		const QStringList &fileList,
		QStringList &remoteFileList);

	/* Called initially from the addJobsToWorker() and then it calls itself as its
	 * processing files and folders recursively */
	void addFilesAndFoldersRecursively(
		SFTPThread *pSFTPThread,
		bool DIRsExecutable,
		long permissions,
		QString localPath,
		QString remotePath);

	/******************
	 Private Variables
	******************/
	QList<SFTP_Info>
		m_SFTP_roster;		//Holds a list of our SFTP Workers

	QTimer
		*mp_jackTheRipper;	//Timer used to "get rid" of idle workers

	FileTransferProgressDlg
		*mp_FTPD;		//Used ONLY to create signal/slot connections
};
