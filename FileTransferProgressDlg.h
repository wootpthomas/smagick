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
#include <QDialog>
#include <QTimer>

/***********************
 Includes
*************************/
#include "ui_FileTransferProgress.h"

/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif



class FileTransferProgressDlg :
	public QDialog,
	public Ui_FileTransferProgress
{
	Q_OBJECT

public:
	FileTransferProgressDlg(void);
	~FileTransferProgressDlg(void);

signals:
	/* Sent if the user clicks on the cancel button during a transfer */
	void cancelTransfer();

public slots:
	/* called when we are beginning a new file transfer */
	void slotNewTransfer(QString localFilePath, QString remoteFilePath, qint64 fileSize);

	/* Called when we recieve an update of the transfer progress */
	void slotUpdateProgress(qint64 bytesTransferred, QString localFilePath);

	/* Called when the user clicks the cancel button during a transfer */
	void slotCancelTransfer()		{ emit cancelTransfer(); }

	/* Called when a file transfer completes */
	void slotTransferCompleted(QString localFilePath);

	/* Called when our timer goes off */
	void slotCloseDlg();

	void slotFade();

private:
	qint64
		m_totalFileSize;

	QTimer
		m_timer,
		m_fadeOutTimer;

	qreal
		m_opacity;
};
