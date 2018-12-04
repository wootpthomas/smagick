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
#include <QListWidgetItem>

/***********************
 Includes
*************************/
#include <libssh2.h>
#include <libssh2_sftp.h>
#include "FileTransferDlg.h"




////////////////////////////////////////////////////////////////////////////////////
FileTransferDlg::FileTransferDlg(QList<FileFolderTransferInfo> *pFileList, const QString &cwd, const QString &username):
QDialog(),
mp_fileList(pFileList)
{
	setupUi(this);

	if ( cwd.isEmpty() || cwd.compare("~") == 0)
		m_destination->setText( "/home/" + username + "/" );	//default to home directory of user
	else
		m_destination->setText(cwd);

	//First, we add Folders to our list
	for (int i = 0; i < mp_fileList->size(); i++)
	{
		QIcon icon;
		if ( (*mp_fileList)[i].isFolder )
		{
			icon = QIcon(":/icons/folder_add.png");

			QListWidgetItem *pItem = new QListWidgetItem( icon, (*mp_fileList)[i].localPath, m_transferList);
			pItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
			pItem->setCheckState(Qt::Checked);
		}
	}

	//Now add all files
	for (int i = 0; i < mp_fileList->size(); i++)
	{
		QIcon icon;
		if ( ! (*mp_fileList)[i].isFolder )
		{
			icon = QIcon(":/icons/page_add.png");

			QListWidgetItem *pItem = new QListWidgetItem( icon, (*mp_fileList)[i].localPath, m_transferList);
			pItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
			pItem->setCheckState(Qt::Checked);
		}
	}

	show();
}

////////////////////////////////////////////////////////////////////////////////////
FileTransferDlg::~FileTransferDlg(void)
{
}

////////////////////////////////////////////////////////////////////////////////////
int
FileTransferDlg::exec()
{
	int result = QDialog::exec();

	//Remove any un-checked items in our list
	for (int i = 0; i < m_transferList->count(); i++)
	{
		QListWidgetItem *pItem = m_transferList->item(i);
		if (pItem && pItem->checkState() != Qt::Checked)
		{
			bool found = false;
			int idx = 0;
			for (; idx < mp_fileList->size(); idx++)
			{
				QString name = pItem->data(Qt::DisplayRole).toString();
				if (name.compare( (*mp_fileList)[idx].localPath) == 0)
				{
					found = true;
					break;
				}
			}

			if (found)
				mp_fileList->removeAt(idx);
		}
	}

	return result;
}


////////////////////////////////////////////////////////////////////////////////////
///* Read, write, execute/search by owner */
//#define LIBSSH2_SFTP_S_IRWXU        0000700     /* RWX mask for owner */
//#define LIBSSH2_SFTP_S_IRUSR        0000400     /* R for owner */
//#define LIBSSH2_SFTP_S_IWUSR        0000200     /* W for owner */
//#define LIBSSH2_SFTP_S_IXUSR        0000100     /* X for owner */
///* Read, write, execute/search by group */
//#define LIBSSH2_SFTP_S_IRWXG        0000070     /* RWX mask for group */
//#define LIBSSH2_SFTP_S_IRGRP        0000040     /* R for group */
//#define LIBSSH2_SFTP_S_IWGRP        0000020     /* W for group */
//#define LIBSSH2_SFTP_S_IXGRP        0000010     /* X for group */
///* Read, write, execute/search by others */
//#define LIBSSH2_SFTP_S_IRWXO        0000007     /* RWX mask for other */
//#define LIBSSH2_SFTP_S_IROTH        0000004     /* R for other */
//#define LIBSSH2_SFTP_S_IWOTH        0000002     /* W for other */
//#define LIBSSH2_SFTP_S_IXOTH        0000001     /* X for other */
long
FileTransferDlg::getPermissions()
{
	long perm = 0;
	
	//Set owner permissions
	if (m_ownerRead->isChecked() )
		perm |= LIBSSH2_SFTP_S_IRUSR;
	if (m_ownerWrite->isChecked() )
		perm |= LIBSSH2_SFTP_S_IWUSR;
	if (m_ownerExecute->isChecked() )
		perm |= LIBSSH2_SFTP_S_IXUSR;

	//Set group permissions
	if (m_groupRead->isChecked() )
		perm |= LIBSSH2_SFTP_S_IRGRP;
	if (m_groupWrite->isChecked() )
		perm |= LIBSSH2_SFTP_S_IWGRP;
	if (m_groupExecute->isChecked() )
		perm |= LIBSSH2_SFTP_S_IXGRP;

	//Set other permissions
	if (m_othersRead->isChecked() )
		perm |= LIBSSH2_SFTP_S_IROTH;
	if (m_othersWrite->isChecked() )
		perm |= LIBSSH2_SFTP_S_IWOTH;
	if (m_othersExecute->isChecked() )
		perm |= LIBSSH2_SFTP_S_IXOTH;

	return perm;
}