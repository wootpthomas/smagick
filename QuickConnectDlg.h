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
#include "ui_QuickConnect.h"
#include "ConnectionManager.h"



/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif



class QuickConnectDlg :
	public QDialog,
	public Ui_QuickConnect
{
	Q_OBJECT

public:
	QuickConnectDlg(QWidget *parent, ConnectionManager *pConnectionMgr);
	~QuickConnectDlg(void);

	/* Override the default exec behavior so that we can save the connection
	 * if the user has chosen to save it */
	int exec();

	/* Returns a pointer to a ConnectionData object which contains the details
	 * of the connection the user is making. */
	ConnectionDetails *getConnectionDataObj();

public slots:
	/* If the user clicks the More button, this unhides advanced options */
	void slotShowAdvancedOptions();

	void slotPopulateUsers(const QString &host);
	void slotPopulatePassword(const QString &user);

private:
	/*********************
	Private Functions
	*********************/
	void init();

	void toggleAdvancedOptions();

	/*********************
	Private Variables
	*********************/
	bool mb_hideAdvOptions;
	ConnectionManager *mp_connectionMgr;
};
