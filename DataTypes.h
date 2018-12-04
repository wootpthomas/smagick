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

/**********************
 * QT Includes
 *********************/
#include <QMetaType>
#include <QString> 
#include <QStringList>
#include <QColor>
#include <QFont>


/**********************
 * Forward class declarations
 *********************/
class SFTPThread;
//class ConsoleChar;


/***********************
 Defines
*************************/
#define CHAR_NULL                0
#define CHAR_START_OF_HEADING		1
#define CHAR_START_OF_TEXT			2
#define CHAR_END_OF_TEXT			3
#define CHAR_END_OF_TRANSMISSION	4
#define CHAR_ENQUIRY				   5
#define CHAR_ACKNOWLEDGE			6
#define CHAR_BELL				   	7
#define CHAR_BACKSPACE				8
#define CHAR_HORIZONTAL_TAB		9
#define CHAR_NEW_LINE				10
#define CHAR_VERTICAL_TAB			11
#define CHAR_NEW_PAGE				12
#define CHAR_CARRIAGE_RETURN		13
#define CHAR_SHIFT_OUT				14
#define CHAR_SHIFT_IN				15
#define CHAR_DATA_LINK_ESC			16
#define CHAR_DEVICE_CONTROL_1		17
#define CHAR_DEVICE_CONTROL_2		18
#define CHAR_DEVICE_CONTROL_3		19
#define CHAR_DEVICE_CONTROL_4		20
#define CHAR_ESC					   27
#define CHAR_SPACE					32
#define CHAR_DEL						127


/***********************
 Defines - Specifically for QSettings. Nothing else!
*************************/
#define QSETTINGS_ORGANIZATION "Shell sMagick Team"
#define QSETTINGS_APP_NAME		 "Shell sMagick"

//Quick connect dialog settings
#define QSETTINGS_QUICKCONNECT_HOST_ADDRESS	"QuickConnect_HostAddress"
#define QSETTINGS_QUICKCONNECT_PORT				"QuickConnect_HostPort"
#define QSETTINGS_QUICKCONNECT_USERNAME		"QuickConnect_Username"

//User preferences settings
#define QSETTINGS_USER_PREFS_FONT				"UserPreferences_font"
//cursor color
#define QSETTINGS_USER_PREFS_CURSOR_COLOR		"UserPreferences_CursorColor"
//Foreground colors
#define QSETTINGS_USER_PREFS_FGC_0_39			"UserPreferences_FC_0_39"
#define QSETTINGS_USER_PREFS_FGC_30				"UserPreferences_FC_30"
#define QSETTINGS_USER_PREFS_FGC_31				"UserPreferences_FC_31"
#define QSETTINGS_USER_PREFS_FGC_32				"UserPreferences_FC_32"
#define QSETTINGS_USER_PREFS_FGC_33				"UserPreferences_FC_33"
#define QSETTINGS_USER_PREFS_FGC_34				"UserPreferences_FC_34"
#define QSETTINGS_USER_PREFS_FGC_35				"UserPreferences_FC_35"
#define QSETTINGS_USER_PREFS_FGC_36				"UserPreferences_FC_36"
#define QSETTINGS_USER_PREFS_FGC_37				"UserPreferences_FC_37"
//Background colors
#define QSETTINGS_USER_PREFS_BGC_0_49			"UserPreferences_FC_0_49"
#define QSETTINGS_USER_PREFS_BGC_40				"UserPreferences_FC_40"
#define QSETTINGS_USER_PREFS_BGC_41				"UserPreferences_FC_41"
#define QSETTINGS_USER_PREFS_BGC_42				"UserPreferences_FC_42"
#define QSETTINGS_USER_PREFS_BGC_43				"UserPreferences_FC_43"
#define QSETTINGS_USER_PREFS_BGC_44				"UserPreferences_FC_44"
#define QSETTINGS_USER_PREFS_BGC_45				"UserPreferences_FC_45"
#define QSETTINGS_USER_PREFS_BGC_46				"UserPreferences_FC_46"
#define QSETTINGS_USER_PREFS_BGC_47				"UserPreferences_FC_47"



/***********************
 Structures
 **********************/


/** Used in SSHConnection class
 * Anytime the status changes, its updated and a signal in the class is
 * emitted letting the world know that somethings changed or in progress */
enum StatusChangeEvent {
	INVALID_CHANGE_EVENT,				//The default "no event set"
	CS_creatingSocket,					//Socket is being created
	CS_creatingSocketFailed,			//Socket failed to be created
	CS_lookingUpHostsIPAddress,			//Hostname given, looking up IP address
	CS_lookingUpHostsIPAddressFailed,	//Failed to look up IP address
	CS_IPAddressInvalid,				//The IP address supplied is not correct
	CS_connectingToRemoteHost,			//Trying to create a socket with the remote host
	CS_connectingToRemoteHostFailed,	//Failed to create a socket with the remote host
	CS_initLIBSSH2,						//Initializing the LibSSH2 library
	CS_initLIBSSH2Failed,				//Initializing the LibSSH2 library failed.
	CS_gettingHostFingerPrint,			//Getting the remote host's fingerprint
	CS_gettingHostFingerPrintFailed,	//Failed to get the remote host's fingerprint
	CS_gettingAllowedAuthTypes,			//Getting the allowed authentication types
	CS_gettingAllowedAuthTypesFailed,	//Failed to get the allowed authentication types
	CS_openChannelRequest,				//We are requesting a new channel
	CS_openChannelRequestFailed,		//New channel request failed
	CS_loggingInByPassword,				//Using password authentication
	CS_loggingInByPasswordFailed,		//Set when login by password fails
	CS_loggingInByKeyboardAuth,			//Attempting to login by keyboard authentication
	CS_loggingInByKeyboardAuthFailed,	//Failed to login by keyboard authentication
	CS_loggingInByRSA,					//Attempting to login by RSA authentication
	CS_loggingInByRSAFailed,			//Failed to login by RSA authentication
	CS_requestingPTY,					//Requesting a PTY on an open channel
	CS_requestingPTYFailed,				//Faile to get a PTY
	CS_requestingShell,					//Requesting a shell on a channel with an open PTY
	CS_requestingShellFailed,			//Failed to get a shell
	CS_requestingTunnel,				//Requesting a tunnel setup
	CS_requestingTunnelFailed,			//Failed to setup tunnel
	CS_setEnvironmentVariables,			//Setting up enviornmental variables on the PTY
	CS_setEnvironmentVariablesFailed,	//Failed to set up enviornmental variables
	CS_connected,						//Everything is connected and running
	CS_disconnected,					//Timeout, lost connection
	CS_shutdownNormal					//Connection closed by some normal means, user logout, etc...
};



/** Used in SSHConnection class
 * When requesting a channel, this will tell it what type of
 * channel that it is setting up */
enum ChannelType {
	TERMINAL_VT100,
	TERMINAL_VT102,
	TERMINAL_XTERM,
	SSH_TUNNEL
};


/** Used in Decoder, TextConsole, GraphicalConsole classes
 */
enum CharSet {
	CHAR_SET_G0,
	CHAR_SET_G1,
	CHAR_SET_UK_as_G0,
	CHAR_SET_UK_as_G1,
	CHAR_SET_US_as_G0,
	CHAR_SET_US_as_G1,
	CHAR_SET_LINE_as_G0,
	CHAR_SET_LINE_as_G1
};

enum MoveDirection {
   MOVE_LEFT,
   MOVE_RIGHT,
   MOVE_UP,
   MOVE_DOWN,
	MOVE_NEXT_LINE,
	MOVE_PREVIOUS_LINE
};


enum EraseType {
	/* 	Used by the 'K' attribute */
	ERASE_TO_END_OF_LINE,
	ERASE_TO_BEGINNING_OF_LINE,
	ERASE_ENTIRE_LINE_NO_MOVE_CURSOR,

	/* 	Used by the 'J' attribute */
	ERASE_TO_END_OF_SCREEN,
	ERASE_TO_BEGINNING_OF_SCREEN,
	ERASE_ENTIRE_SCREEN_NO_MOVE_CURSOR
};




//When the SSHWidget is created, we tell it what type of render widget to use
enum RENDER_TYPE {
	QTEXTEDIT_HTML,
	QPAINTER,
	OPENGL
};


/* Created so that given a folder or file, we know its:
 * local path,
 * remote path,
 * type: folder or file */
struct FileFolderTransferInfo{
	QString
		localPath,		//Specifies the full local path of the file
		remotePath;		//Specifies the full remote path (where item is going)

	bool
		isFolder;		//Lets us know if this is a file or a folder
};

struct TransferRequest {
	QList<FileFolderTransferInfo>
		fileFolderList;		//Holds file/folder info
	
	QString
		remoteBasePath,		//Remote folder where the copy started
		hostAddress,			//Remote host address
		username,				//Username to use
		password;				//Password to use
	
	int
		hostPort;				//Remote host port
	
	long
		filePermissions;

	bool
		isRecursive,
		makeDIRsExecutable;

	TransferRequest(){
		isRecursive = true;
		makeDIRsExecutable = true;
		hostAddress = "";
		username = "";
		password = "";
		filePermissions = 0;
		hostPort = 0;
	}
};


/** These next two items are used when we are doing drag-n-drop file
 * operations */
enum TransferRequestType{
	INVALID_TRANSFER_TYPE,
	COPY_FILE_TO_REMOTE_HOST,
	MOVE_FILE,
	DELETE_FILE,
	CREATE_DIR,
	DELETE_DIR,
	MOVE_DIR
};

/* A small, simple request meant to be given to SFTP worker threads */
struct SFTPTransferRequest {
	TransferRequestType
		requestType;

	QStringList
		localItems,			//Specifies item location on local disk
		remoteItems;		//Specifies the destination of the corresponding localItem[index]

	long
		filePermissions;
};

/* Anytime you need to change any item in the statusbar, we send the tab widget
 * a statusBarInfo object. The tabbinator will then update its statusbar with the
 * contents of the object depending on the widget being shown. */
struct StatusBarInfo {
	StatusChangeEvent
		statusEvent;		//Help us find out exactly what SSHConnection is doing

	QString
		statusMsg,			//User-friendly message about the connectionStatus
		cwd,					//Holds the current working directory for the console
		iconPath;			//Holds the path of the icon to be displayed

	bool
		isEventUserVisible;		//If false, then the event should not be shown to the user

	StatusBarInfo(){
		statusEvent = INVALID_CHANGE_EVENT;
		statusMsg = "";
		cwd = "";
		iconPath = "";
		
		isEventUserVisible = true;
	}
};

/* Typically the TransferManager sends this object down to a SFTPWorker thread.
 * It tells the worker its login credentials. */
struct SFTP_Info{
	QString
		m_remoteHostAddress,
		m_username;
	int
		m_port;

	bool
		m_bIsIdle,					//True if the thread is idle
		m_bHasPoliceEscort;		//True if we do not want to shutdown this thread if idle

	SFTPThread
		*mp_sftpWorkerThread;

	SFTP_Info(){
		m_bIsIdle = true;
		m_bHasPoliceEscort = false;
		m_remoteHostAddress = "";
		m_username = "";
		m_port = 0;
		mp_sftpWorkerThread = NULL;
	}
};




struct SSHTunnelItem
{
	int
		sourcePort,
		remotePort;
	QString
		remoteHost;
};

struct ConnectionDetails
{
	int
		connectionID,			//Used by the database to uniquely ID this connection
		hostPort;				//Remote host port number

	QString
		hostAddress,			//Remote host port, can be IP or fully qualified DNS namd
		username,
		password,
		RSA_publicKey,
		RSA_privateKey;

	QStringList					// The list has a variable name and the next item is its value
		envVars;					//Ex: list[0] = "VARIABLE", list[1] = "Value of variable"
	
	bool
		encriptPasswd,
		loginByUsernamePassword,	//Speficy Login type to attempt
		loginByRSA,
		isDirty;							//variable to determin if the data is dirty (needs to recorded in database)

	/* Helps store a list of SSH Tunnel objects. Each item in the list
	 * represents a tunnel that this object requests to be created */
	QList<SSHTunnelItem>
		tunnelList;

	//Extend values as needed....

	//Constructor. Sets up default values
	ConnectionDetails()
	{
		hostPort = 22;
		connectionID = -1;
		loginByUsernamePassword = true;
		loginByRSA = false;
		isDirty = false;
	}
};

struct GroupDetails
{
	QString
		groupName,
		groupDiscription;

	int
		groupID;
};

Q_DECLARE_METATYPE(GroupDetails);
Q_DECLARE_METATYPE(ConnectionDetails);		//Let Qt know about our struct so we can use it with QVariant



