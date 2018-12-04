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


/***********************
 Other Includes
*************************/
#include <libssh2.h>
#include <libssh2_sftp.h>
#include "SSHConnection.h"


///*****************
// * Memory Leak Detection
// *****************/
//#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
//	#define _CRTDBG_MAP_ALLOC
//	#include <stdlib.h>
//	#include <crtdbg.h>
//
//	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
//	#define new DEBUG_NEW
//#endif


class SFTP :
	public SSHConnection
{
	Q_OBJECT

public:
	SFTP(
		const QString &remoteHostAddress,
		const QString &username,
		const QString &password,
		int port);

	~SFTP(void);

	/* Makes a remote connection and logs in. If return value is true, this
	 * is a live connection to the remote host and we can use it to do file
	 * transfers */
	bool init();

	/* Provided so that an outisde object can figure out where we are connected */
	const QString &username()			const		{ return m_username; }
	const QString &getHostAddress()	const		{ return m_hostAddress; }
	const int &getPort()					const		{ return m_hostPort; }

	/* Allows us to get the error message for the last function that failed */
	QString getErrorMsg()				const		{ return m_lastErrorMsg; }

	/* Transfers the given file to the remote system at the destination path
	 * with the specified file permissions. */
	bool transferFileToRemote(const QString &localFilePath, const QString remoteFilePath, long filePermissions);

	/* Creates the given directory on the remote system */
	bool createDir(const QString &remoteDIR, long filePermissions);


signals:
	
	/* Sent out when a file begins to transfer */
	void transferStarted(QString localFilePath, QString remoteFilePath, qint64 totalFileSizeBytes);

	/* Sent out periodically during the transfer of a file.*/
	void transferProgress(qint64 bytesTransferred, QString localFilePath);

	/* Sent when a file transfer has completed */
	void transferCompleted(QString localFileName);

	/* Sent when a transfer fails */
	void transferFailed(QString fileName);

public slots:
	


private:
	/********************
	 Private Functions
	********************/
	/* When a requested action encounters an error, we call this to figure out
	 * the details of the error. The error can then be retrieved with
	 * getErrorMsg() */
	void getErrorDetails();

	/********************
	 Private Variables
	********************/
	LIBSSH2_SFTP
		*mp_sftpSession;

	QString
		m_lastErrorMsg;		//Holds a description of the last error message
};
