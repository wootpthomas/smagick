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


/*****************
 Qt Includes
******************/
#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>
#include <QMutexLocker>

/******************
 Includes
*******************/
#include <libssh2.h>
#include "DataTypes.h"


/******************
 Forward Class Declarations
******************/
class PollWorkerThread;
class SocketSelectThread;
class QTcpSocket;
class DecoderThread;


/******************
 # defines
******************/
#define AUTH_METHOD_UNKNOWN					0
#define AUTH_METHOD_PASSWORD					1
#define AUTH_METHOD_KEYBOARD_INTERACTIVE	2
#define AUTH_METHOD_PUBLIC_KEY				4


/***********************
 Class definition
***********************/
class SSHConnection:
	public QObject
{
	Q_OBJECT 

public:

	enum AuthMethod {
		KEYBOARD_INTERACTIVE,
		RSA_KEY
	};


	/* Constructor. This takes the remote host address either in
	 * xxx.xxx.xxx.x format or in rah.timmy.com format. The port number
	 * will default to 22 if not supplied.
	 * Requires:
	 *	remoteHostAddress
	 * Optional:
	 *	port to connect to	 */
	SSHConnection(
		const QString &remoteHostAddress,
		const QString &username,
		const QString &password,
		int port = 22,
		DecoderThread *pDecoder = NULL);
	
	/* Destructor */
	~SSHConnection();

	/* This attempts to initialze the socket toconnect to the remote host specified in the
	 * constructor. If the socket connection is successful it returns
	 * true otherwise, this SSHConection object is useless and false is
	 * returned.
	 * If this is successful, this object will automatically get the remote Host 
	 * Fingerprint*/
	bool init();
	
	/* If init() fails, this could be a for a number of reasons. Here's one possible
	 * check that we'd need to do. */
	bool isHostAddressValid() const { return m_isHostAddressValid; }

	/* If an error occured, we'll stick a hopefuly informative message in place
	 * so that we can call this and find out what went wrong */
	const QString &getLastErrorMessage() const { return m_lastErrorMessage; }

	/* After performing init() and succeeding, we can get the remote host's fingerprint.
	 * If you attempt to get it before calling init() or if init() fails, then an empty
	 * QString is returned. */
	const QString getRemoteHostFingerPrint() const { return m_hostFingerPrint; }

	/* Gives us a way to check and see what authentication methods are supported by
	 * the server we are connecting to. Note: You must run getAuthMethods() or this
	 * function will always return false. */
	bool isValidAuthenticationMethod( AuthMethod method);

	/* Tries to login by using any available login method. Returns true if successful */
	bool login();

	/* Gets a list of authentication methods that this server supports */
	void setAuthMethods();

	/* Used to re-set the password to something different. Useful if login attempt fails */
	void setPassword(const QString &password)		{ m_password = password; }

	/* this is the calback funciton used by the libssh2 library. It's public so that
	 * the library can access it. It should not be used elsewhere. */
	static void kbdCallBack(
		const char *name,
		int name_len, 
		const char *instruction,
		int instruction_len,
		int num_prompts,
		const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
		LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
		void **abstract);

	/* The library calls this when a disconnect occurs. Note that if the connection is
	 * not cleanly terminated, this will not get called */
	static void disconnectCallback(
		LIBSSH2_SESSION *session,
		int reason,
		const char *message,
		int message_len,
		const char *language,
		int language_len,
		void **abstract);

	/* This is called from the static disconnectCallback function */
	void handleDisconnect(int reason, const QString &message, const QString &language);

	/* Tries to login by using the provided RSA public and private keys, the passphrase
	 * and the username. 
	 * Note: for this function to work, we will need to add a new function to the libssh2
	 * library so that it doesn;t read in the key stuff from files. Or we can hack this to
	 * work by creating a temp file with the needed data. 0_o */
	bool loginByRSA(
		const QString username,
		const QString RSAPublicKey,
		const QString RSAPrivateKey,
		const QString passphrase);

	/* Creates a new channel and returns the number of the channel. Use
	* the number returned to referr to the channel later on. If a negative
	* number was returned, then the channel was not created successfully.
	* if the env vars list is populated, then this will also setup any specified
	* enviornmental variables for the channel.
	* Returns: -1 on failure, otherwise channelNumber created.*/
	int newChannel(
		ChannelType channelType,
		QStringList enviornmentalVarList,
		int termWidth = 80,
		int termHeight = 24);

	/* Tries to create the specified TCP/IP tunnel from localhost on the sourcePort
	 * to the remote host and port */
	bool createTunnel(
		int sourcePort,
		const QString &destHost,
		int destPort);

	/* Adjusts the terminal with on the given channel. Note: The channel number specified
	 * MUST have a pty on the channel or it will fail immediately. If you do not specify
	 * a channel number (using -1), it will auto-select the first one that has a VT100 terminal.
	 * You can optionally also specify the width and height in pixels */
	bool adjustTerminalSize(
		int channelNumber,
		int termWidth,
		int termHeight,
		int termWidthPx = -1,
		int termHeightPx = -1);

	/* Writes the given data to the channel */
	int writeToChannel(int channelNumber, const QString data);

	/* Writes tot he given channel. This is normally only used by tunnels
	 * to quickly send data */
	int writeToChannel(LIBSSH2_CHANNEL * const pChannel, const char *pData, int length);

	/* Closes the channel and frees any associated resources */
	bool closeChannel(int channelNumber);

	/** THREAD SAFE
	 * If we setup a tunnel and something like windows remote desktop connects to our socket
	 * then we tell the library that when ever this particular channel gets data, we shove
	 * it down this socket */
	bool registerTunnelConnection(LIBSSH2_CHANNEL * const pChannel, int sockToSendOn);

//public slots:
	/** THREAD SAFE
	 * Gets called when our polling worker found data on a channel. The number is the
	 * channel number to read from. */
	void slotReadFromChannel(LIBSSH2_CHANNEL * const pChannel);


signals:
	/* Lets the world know when there is data available. Says which channel number it is */
	void channelDataAvailable(char *pData, int pDataSize, int channelNumber);

	/*  Emitted whenever the connection status changes. This is how we communicate all
	 * or our connection, connection status signals to the outside world */
	void statusChanged(StatusChangeEvent newStatus);

	// Sent when we have a bad password and need the correct one cause auth failed
	void requestCorrectPassword();

protected:
	/* We keep a list of channel's and there info so we can keep tabs
	 * on what types of channels we have and stuff */
	struct ChannelItem{
		bool
			isActive;			//Is this channel still active/alive?

		StatusChangeEvent
			channelStatus;		//Holds a status flag indicating current channel status

		ChannelType
			channelType;		//Type of channel

		LIBSSH2_CHANNEL
			*pChannel;			//Pointer to the actual channel
		
		QString
			tunnel_destHostAddress;	//Address to remote host
		int
			tunnel_destPort,			//remote host's port (port forwarding too)
			tunnel_sourcePort,		//Port we are forwarding from on localhost
			tunnel_replyPort;			/* Port we send recieved channel data on. Will be
											 * set to a non-zero value when a client connects
											 * to our tunnel*/
		ChannelItem(){
			isActive = false;
			channelStatus = INVALID_CHANGE_EVENT;
			channelType = TERMINAL_XTERM;
			pChannel = NULL;
			tunnel_destHostAddress = "";
			tunnel_destPort = 0;
			tunnel_sourcePort = 0;
			tunnel_replyPort = 0;
		}
	};

	//Pointer to the session object
	LIBSSH2_SESSION *mp_session;

	/* Ok, so we need a way to store the channels, so lets throw them in QList.*/
	QList<ChannelItem *>
		m_channelList;

	QString
		m_hostAddress,			//Address of the remote host
		m_lastErrorMessage,	//Contains info about the last error
		m_hostFingerPrint,	//Holds the hex representation of the host's fingerprint
		m_username,				//Username we will try to connect as
		m_password;

	int
		m_socket,				//The socket itself
		m_hostPort;				//Port number to connect the socket to

	unsigned char
		m_authMethods;				/* Each bit represents the auth method:
										 * password
										 * keyboard-interactive
										 * publickey */
private:
	/*************************
	 Private Functions
	*************************/
	/* The first time, we try to login with the suplied password. If the given password
	 * was wrong, then the user actually will get prompted to enter in the correct password */
	QString getPassword(int num_prompts);

	/* Closes any open channels and closes the socket */
	void shutdown();

	/* Creates a socket to the hostAddress at <port> and returns the socket descriptor
	 * on success and -1 on failure */
	int createSocket(const char* hostAddress, int port, int socketType);

	/* Sets up our polling worker thread to listen for activity on SSH channels.
	 * This method can safely be called multiple times. It will only create the worker
	 * if it hasn;t yet been created. */
	bool setupPollWorkerThread();

	/* Sets up our socket select listener to listen for activity on sockets. */
	bool setupSocketWorkerThread();

	/*************************
	 Private Variables
	*************************/
	bool
		m_isHostAddressValid,	//After calling init(), is the host address valid? Can only false if init() failed
		m_bAlreadySentDisconnect; //Helps keep us from blasting out disconnect signals when shit hits the fan
	
	int
		m_emptyReadCount;		//Counter to help detect when the socket times out

	/* This is our multifunctional threaded listener. It can poll channels and sockets */
	PollWorkerThread
		*mp_channelPollingWT;	/* Our polling object responzible for polling a list of
										 * channels. We can add and remove channels from it while
										 * its running. */
	SocketSelectThread
		*mp_socketPollingWT;		/* Our polling object responsible for using select on a list of
										 * sockets to listen for activity. This is only used for TCP/IP tunnels */

	DecoderThread					//Used to decode any terminal stuff
		*mp_decoder;

	QMutex
		m_mutex;
};
