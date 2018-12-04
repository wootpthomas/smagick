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

/*******************
 Qt Includes
*******************/
#include <QMessageBox>

/*******************
Custom header files
*******************/
#include "MasterPasswordLoginDlg.h"
#include "ConnectionManager.h"


#define MASTERPASSWORDLOGINDLG_1 "sMagick"
#define MASTERPASSWORDLOGINDLG_2 "The password you entered was not correct.\nPlease try again."
#define MASTERPASSWORDLOGINDLG_3 "The password you entered was not correct, again.\nsMagick will now exit since you have exceeded 3 attempts."


MasterPasswordLoginDlg::MasterPasswordLoginDlg(ConnectionManager *pConnectionMgr):
	QDialog(),
	mp_connectionMgr(pConnectionMgr),
	m_isLoggedIn(false),
	m_isGuestLoggedIn(false),
	m_loginAttemptCount(0)
{
	setupUi(this);
	show();

	init();
}

MasterPasswordLoginDlg::~MasterPasswordLoginDlg(void)
{
}


void MasterPasswordLoginDlg::init()
{
	connect( m_loginBtn, SIGNAL(clicked()),
		this, SLOT( login()));
	connect( m_guestLoginBtn, SIGNAL(clicked()),
		this, SLOT( loginAsGuest()));
	
}

void MasterPasswordLoginDlg::loginAsGuest()
{
	login(true);
	mp_connectionMgr->setGuestMode(true);
}

void MasterPasswordLoginDlg::login(bool loginAsGuest)
{
	m_loginAttemptCount++;

	if (loginAsGuest)
	{
		this->m_isGuestLoggedIn = true;
		QDialog::accept();
		return;
	}
	
	if (mp_connectionMgr->isCorrectPassword( m_password->text() ) )
	{
		m_isLoggedIn = true;
		QDialog::accept();
	}
	else	//Password is WRONG!
	{
		if (m_loginAttemptCount == 3)
		{
			//Let the user know they are stupid and do NOT get another chance
			 QMessageBox::warning(this, tr(MASTERPASSWORDLOGINDLG_1),
				 tr(MASTERPASSWORDLOGINDLG_3), QMessageBox::Ok);
			 QDialog::reject();
			 return;
		}
		else
		{
			//Let the user know they are stupid and get another chance
			 QMessageBox::warning(this, tr(MASTERPASSWORDLOGINDLG_1),
				 tr(MASTERPASSWORDLOGINDLG_2), QMessageBox::Ok);

			 m_password->selectAll();
		}
	}
}