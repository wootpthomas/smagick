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
#include <QFileInfo>


/***********************
 Includes
*************************/
#include "SFTP.h"





////////////////////////////////////////////////////////////////////////////////////
SFTP::SFTP(const QString &remoteHostAddress,
		const QString &username,
		const QString &password,
		int port):
SSHConnection(remoteHostAddress, username, password, port)
{
	
}

////////////////////////////////////////////////////////////////////////////////////
SFTP::~SFTP(void)
{
	if (mp_sftpSession)
		libssh2_sftp_shutdown(mp_sftpSession);
}

////////////////////////////////////////////////////////////////////////////////////
bool
SFTP::init()
{
	if ( ! SSHConnection::init())
	{
		printf("SFTP: Failed to create a remote connection\n");
		return false;
	}

	if ( ! login())
	{
		printf("SFTP: Failed to login\n");
		return false;
	}

	//Lets try and setup this object for SFTP
	mp_sftpSession = libssh2_sftp_init( mp_session);
	if ( ! mp_sftpSession)
	{
		printf("SFTP: Failed to create a SFTP session\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SFTP::transferFileToRemote(const QString &localFilePath, const QString remoteFilePath, long filePermissions)
{
	LIBSSH2_SFTP_HANDLE *pHandle = libssh2_sftp_open(
		mp_sftpSession,
		remoteFilePath.toLatin1().constData(),
		LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,	//Write, create, trucate
		filePermissions);

	QFile qFile(localFilePath);
	if ( ! qFile.open(QIODevice::ReadOnly) )
	{
		transferFailed(localFilePath);
		printf("Could not open file %s for reading\n", localFilePath.toLatin1().constData() );
		return false;
	}

	if ( ! pHandle)
	{
		unsigned long errorNum = libssh2_sftp_last_error(mp_sftpSession);
		transferFailed(localFilePath);
		printf("Could not create file sftp handle for file %s\n", localFilePath.toLatin1().constData() );
		return false;
	}

	//Lets get some info about the file so we can correctly update on transfer progress
	QFileInfo fileInfo(localFilePath);
	qint64
		fileSize = fileInfo.size(),
		bytesTransferred = 0;

	bool result = true;
	emit transferStarted(localFilePath, remoteFilePath, fileSize);

	QString temp("Transferred %1 of %2 bytes");
	char *pData = new char[256000];		//Reserve 1/4 a MegaByte!!!
	while ( ! qFile.atEnd() )
	{
		int charsRead = qFile.read(pData, 256000);
		if (charsRead > 0)
		{
			int rc = libssh2_sftp_write(pHandle, pData, charsRead);
			if (rc < 0)
			{
				printf("Transfer FAILED!\n");
				result = false;
				break;
			}
			bytesTransferred += rc;
			
			QString update = temp.arg(bytesTransferred).arg(fileSize);
			//printf("%s\n", update.toLatin1().constData() );
			emit transferProgress( bytesTransferred, localFilePath);
		}
	}
	delete [] pData;

	qFile.close();
	libssh2_sftp_close(pHandle);

	//Alert the world
	if ( result)
	{
		printf("Transfer completed successfully\n");
		transferCompleted(localFilePath);
	}
	else
	{
		transferFailed(localFilePath);
		unsigned long errorNum = libssh2_sftp_last_error(mp_sftpSession);
		printf("Transfer Error: %d\n", errorNum);
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SFTP::createDir(const QString &remoteDIR, long filePermissions)
{
	int result = libssh2_sftp_mkdir_ex(
		mp_sftpSession,
		remoteDIR.toLatin1().constData(),
		remoteDIR.size(),
		filePermissions);

	if (result == 0)
		return true;

	getErrorDetails();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////
void
SFTP::getErrorDetails()
{
	char *pErrorMsg = NULL;
	int errorMsgLen = 0;
	int error = libssh2_session_last_error(mp_session, &pErrorMsg, &errorMsgLen, 1);

	int sftpErrorNum = 0;
	if (error == LIBSSH2_ERROR_SFTP_PROTOCOL)
		sftpErrorNum = libssh2_sftp_last_error(mp_sftpSession);
	
	//printf("ErrorMsg: %s.\nSFTP error number %d occured.\n", pErrorMsg, sftpErrorNum);
	if (pErrorMsg)
	{
		delete [] pErrorMsg;
		pErrorMsg = NULL;
	}

	QString m_lastErrorMsg = "";
	switch( sftpErrorNum )
	{
		case LIBSSH2_FX_OK:								m_lastErrorMsg = "Ok, stuff worked.";
			break;
		case LIBSSH2_FX_EOF:								m_lastErrorMsg = "End of file.";
			break;
		case LIBSSH2_FX_NO_SUCH_FILE:					m_lastErrorMsg = "No such file.";
			break;
		case LIBSSH2_FX_PERMISSION_DENIED:			m_lastErrorMsg = "Permission denied.";
			break;
		case LIBSSH2_FX_FAILURE:						m_lastErrorMsg = "Can't create item.";
			break;
		case LIBSSH2_FX_BAD_MESSAGE:					m_lastErrorMsg = "Bad message.";
			break;
		case LIBSSH2_FX_NO_CONNECTION:				m_lastErrorMsg = "No connection to remote host.";
			break;
		case LIBSSH2_FX_CONNECTION_LOST:				m_lastErrorMsg = "Connection has been lost.";
			break;
		case LIBSSH2_FX_OP_UNSUPPORTED:				m_lastErrorMsg = "Unsupported operation.";
			break;
		case LIBSSH2_FX_INVALID_HANDLE:				m_lastErrorMsg = "Invalid handle.";
			break;
		case LIBSSH2_FX_NO_SUCH_PATH:					m_lastErrorMsg = "No such path.";
			break;
		case LIBSSH2_FX_FILE_ALREADY_EXISTS:		m_lastErrorMsg = "The file already exists.";
			break;
		case LIBSSH2_FX_WRITE_PROTECT:				m_lastErrorMsg = "That item is write protected.";
			break;
		case LIBSSH2_FX_NO_MEDIA:						m_lastErrorMsg = "No media.";
			break;
		case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:	m_lastErrorMsg = "Not enough disk space.";
			break;
		case LIBSSH2_FX_QUOTA_EXCEEDED:				m_lastErrorMsg = "Quota exceeded.";
			break;
		case LIBSSH2_FX_UNKNOWN_PRINCIPLE:			m_lastErrorMsg = "Unknown principle. WTF does that mean?";
			break;
		case LIBSSH2_FX_LOCK_CONFlICT:				m_lastErrorMsg = "Lock conflict.";
			break;
		case LIBSSH2_FX_DIR_NOT_EMPTY:				m_lastErrorMsg = "The directory is not empty.";
			break;
		case LIBSSH2_FX_NOT_A_DIRECTORY:				m_lastErrorMsg = "Not a directory.";
			break;
		case LIBSSH2_FX_INVALID_FILENAME:			m_lastErrorMsg = "Invalid file name.";
			break;
		case LIBSSH2_FX_LINK_LOOP:						m_lastErrorMsg = "Can't create the link, it would make a link loop.";
			break;
		default:												m_lastErrorMsg = "No available SFTP error number.";
			break;
	}		
}
