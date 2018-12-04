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
***********************/
#ifdef WIN32
	#include "Winsock2.h"
#endif

#include <QtGui>


#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>

//http://discuss.joelonsoftware.com/default.asp?design.4.374637.18
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW

#endif

/***********************
 Other Includes
***********************/
#include <libssh2.h>
#include <exception>
#include "AES_encryption.h"
#include "CreateChangeMasterPasswordDlg.h"
#include "MasterPasswordLoginDlg.h"
#include "ConnectionManager.h"
#include "MainWindowDlg.h"
#include "ConsoleChar.h"


int main(int argc, char *argv[])
{

#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	enum AuthType {
		LOGGED_IN,
		NOT_LOGGED_IN,
		GUEST_LOGIN
	};


#ifdef WIN32
	WSADATA wsadata;
	int sResult = WSAStartup(WINSOCK_VERSION, &wsadata);
	if (sResult)
	{
		printf("Error initializing the windows socket DLL\n");
		return -1;
	}
#endif

	QApplication app(argc, argv);
	int result = 0;
	AuthType authType = NOT_LOGGED_IN;
	CreateChangeMasterPasswordDlg *pCreatePasswordDlg = NULL;
	MasterPasswordLoginDlg *pLoginDlg = NULL;
	ConnectionManager *pConnectionMgr = NULL;

	try{
		/* Create the ConnectionInfo class and tell it to open up the
		 * default connection storage file. If it can't open up the 
		 * file, then assume we are starting for the first time. */
		pConnectionMgr = new ConnectionManager();

		if (pConnectionMgr->isFirstRun())
		{
			pCreatePasswordDlg = new CreateChangeMasterPasswordDlg(CreateChangeMasterPasswordDlg::NEW_PASSWORD, pConnectionMgr);
			
			/* This will block until the user has entered in a password. Once
			 * the password is created, the dialog will set the new password
			 * in the connectionInfo object */
			pCreatePasswordDlg->exec();
			authType = LOGGED_IN;
			
			if ( pCreatePasswordDlg)
			{
				delete pCreatePasswordDlg;
				pCreatePasswordDlg = NULL;
			}
		}
		else 
		{
			pLoginDlg = new MasterPasswordLoginDlg(pConnectionMgr);
			
			/* This will block until the user has entered in the correct password
			 * or the user entered in the password incorrectly 3 times or if the
			 * user is going to use sMagick in guest mode */
			pLoginDlg->exec();

			if (pLoginDlg->isLoggedIn()) {
				authType = LOGGED_IN;
				pConnectionMgr->setGuestMode(false);
			}
			else if (pLoginDlg->isGuestLogin())
				authType = GUEST_LOGIN;

			if ( pLoginDlg)
			{
				delete pLoginDlg;
				pLoginDlg = NULL;
			}
		}

		if ( authType == LOGGED_IN || authType == GUEST_LOGIN ) 
		{
			/* We are authenticated, or we are using the program as a guest.
			 * Time to startup the main window and have some fun! */
			MainWindowDlg mainWin(pConnectionMgr, argc, argv);
			result = app.exec();		//Start the Qt event loop
		}

	}

	catch (std::exception &e) {
		printf("Something crapped out big time\n");
		printf("%s\n", e.what() );
		result = 1;
	}

	/*******************
	  Cleanup!
	*******************/
	try{
		if ( pCreatePasswordDlg)
			delete pCreatePasswordDlg;
		if ( pLoginDlg)
			delete pLoginDlg;
		if ( pConnectionMgr)
			delete pConnectionMgr;

#if defined(CC_MEM_OPTIMIZE)
		//Defined in the ConsoleChar.h file. Cleans up our static list
		ConsoleChar::cleanupFreeList();
#endif
	}

	catch (std::exception &e) 
	{
		printf("Something crapped out big time\n");
		printf("%s\n", e.what() );
	}


#ifdef WIN32
	//Cleanup windows sockets and free any resources they use
	WSACleanup();
#endif

	return result;
}
						

