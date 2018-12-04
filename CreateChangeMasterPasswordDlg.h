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
***********************/
#include <QDialog>


/***********************
 Other Includes
***********************/
#include "ui_CreateChangeMasterPassword.h"

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
 * The CreateChangeMasterPasswordDlg is meant to be used for two different
 * occasions.
 * 1) We are starting for the first time and the
 *	user needs to create their master password.
 * 2) The user wants to change their master password
 ********************************************************/
class CreateChangeMasterPasswordDlg :
	public QDialog,
	public Ui_CreateChangeMasterPassword
{
	Q_OBJECT

public:
	enum UsageType {
		NEW_PASSWORD,
		CHANGE_PASSWORD
	};

	/* Constructor: Needs the dialog usage type and a valid
	 * ConnectionInfo object to interact with for setting or
	 * changing the password. */
	CreateChangeMasterPasswordDlg(UsageType type, ConnectionManager *pConnectionMgr);
	
	~CreateChangeMasterPasswordDlg(void);

	/* Returns true if the user successfully created a new password
	 * returns false if the user didn't create it */
	bool isPasswordCreated();

public slots:
	void accept();

protected slots:
	/* Tests how secure the password typed in is. This will
	 * update the label with a visual security indicator */
	void slotSecurityCheck(QString);

	/* Makes sure the two passwords match. When they do, the
	 * Ok button is enabled. Otherwise the label tells the
	 * user that the passwords do not match */
	void slotConfirmPassword();



private:
	//Initializes the dialog, sets up connections...
	void init();

	// Identifies the type of use expected out of this dialog
	UsageType m_usageType;
	ConnectionManager *mp_connectionMgr;
};

