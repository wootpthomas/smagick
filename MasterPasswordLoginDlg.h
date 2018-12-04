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

/***********************
 Other Includes
*************************/
#include "ui_MasterPasswordLogin.h"



/*************************
 Forward Class Declarations
**************************/
class ConnectionManager;

/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/********************************************************

 ********************************************************/
class MasterPasswordLoginDlg :
	public QDialog,
 	public Ui_MasterPasswordLogin
{
	Q_OBJECT

public:
	MasterPasswordLoginDlg(ConnectionManager *pConnectionMgr);
	~MasterPasswordLoginDlg(void);

	//Lets us know if the login was successful or not
	bool isLoggedIn() const { return m_isLoggedIn; }

	bool isGuestLogin() const { return m_isGuestLoggedIn; }


public slots:
	//Tests the users password via the connectionMgr. Gives the user 3 attempts
	void login(bool loginAsGuest = false);

	//calls login(true)
	void loginAsGuest();

private:
	/*******************
	 Functions
	*******************/
	//Called to setup the dialog and connect signals/slots
	void init();

	/********************
	 Variables
	********************/
	bool
		m_isLoggedIn,		//True if the correct password was entered
		m_isGuestLoggedIn;	//True if the user clicked guest login
	
	int m_loginAttemptCount;

	ConnectionManager *mp_connectionMgr;
};
