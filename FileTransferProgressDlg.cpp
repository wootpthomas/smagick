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
#include <QDialog>
#include <QCheckBox>


/***********************
 Includes
*************************/
#include "FileTransferProgressDlg.h"



////////////////////////////////////////////////////////////////////////////////////
FileTransferProgressDlg::FileTransferProgressDlg(void):
m_totalFileSize(0)
{
	setupUi(this);

	connect(m_cancelBtn, SIGNAL(clicked()),
		this, SLOT(slotCancelTransfer()));

	connect(m_closeBtn, SIGNAL(clicked()),
		this, SLOT(hide()));

	connect( &m_timer, SIGNAL(timeout()),
		this, SLOT(slotCloseDlg()));

	connect( &m_fadeOutTimer, SIGNAL(timeout()),
		this, SLOT(slotFade()));
}

FileTransferProgressDlg::~FileTransferProgressDlg(void)
{
}

////////////////////////////////////////////////////////////////////////////////////
void
FileTransferProgressDlg::slotNewTransfer(QString localFilePath, QString remoteFilePath, qint64 fileSize)
{
	m_timer.stop();

	m_opacity = 1.0;
	setWindowOpacity( m_opacity);

	m_remoteFilePath->setText( remoteFilePath);
	m_localFilePath->setText( localFilePath);
	m_totalFileSize = fileSize;
	
	m_progress->reset();
	m_closeBtn->hide();
	m_cancelBtn->show();
	show();
}

////////////////////////////////////////////////////////////////////////////////////
void
FileTransferProgressDlg::slotUpdateProgress(qint64 bytesTransferred, QString localFilePath)
{
	double value = ((double)bytesTransferred / (double)m_totalFileSize) *100;
	int intValue = value;
	m_progress->setValue( intValue);

	QString temp = "Copied %1 of %2 bytes";
	m_progressLbl->setText( temp.arg(bytesTransferred).arg(m_totalFileSize) );
}

////////////////////////////////////////////////////////////////////////////////////
void
FileTransferProgressDlg::slotTransferCompleted(QString localFilePath)
{
	m_closeBtn->show();
	m_cancelBtn->hide();

	m_timer.setInterval(2000);
	m_timer.start();

	m_progressLbl->setText("Transfer completed.");
}

////////////////////////////////////////////////////////////////////////////////////
void
FileTransferProgressDlg::slotCloseDlg()
{
	if ( ! m_keepWindowOpen->isChecked() )
	{
		//Make the window fade out
		m_fadeOutTimer.setInterval(33);
		m_fadeOutTimer.setSingleShot(false);
		m_fadeOutTimer.start();
	}
}

////////////////////////////////////////////////////////////////////////////////////
void
FileTransferProgressDlg::slotFade()
{
	m_opacity -= .03333;
	if ( m_opacity <= 0)
	{
		hide();
		m_fadeOutTimer.stop();
	}
	else
	{
		if ( m_keepWindowOpen->isChecked() )
		{
			m_opacity = 1.0;
			m_fadeOutTimer.stop();
		}	
		setWindowOpacity( m_opacity);
	}
}
