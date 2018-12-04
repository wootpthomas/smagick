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
#include <QSocketNotifier>
#include <QTcpSocket>


/*******************
 Other Includes
*******************/
#include "SSHConnection.h"
#include "PollWorkerThread.h"
#include "DecoderThread.h"
#include "SocketSelectThread.h"


/**** Windows specific ******/
#ifdef WIN32
	#include "winsock2.h"

	//Redefine the close() function on windows so we don't break on linux
	#define close(SOCKET)				closesocket(SOCKET)
#else
	/**** *nix specific ******/
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netdb.h>
#endif

/*******************
 #Defines
*******************/
/****Messages the user will see ***/
#define SSHCONNECTION_1 "The host address is not valid."
#define SSHCONNECTION_2 "Unable to create a socket to connect to the remote host."
#define SSHCONNECTION_3 "Library error, could not initialize the session."
#define SSHCONNECTION_4 "Library error, could not establish a SSH session."
#define SSHCONNECTION_5 "Library error, could not get the list of authentication methods."
#define SSHCONNECTION_6 "Library error, login by keyboard interactive method failed."
#define SSHCONNECTION_7 "The specified channel type is not valid."
#define SSHCONNECTION_8 "Library error, could not create a new channel."
#define SSHCONNECTION_9 "Library error, request for a terminal was denied."
#define SSHCONNECTION_10 "Library error, unable to open a shell on the pty."

#define POLLING_INTERVAL_IN_USEC 500	// essentially 500msec
//#define BUFFER_SIZE 16384
#define BUFFER_SIZE 65536		//65KB buffer!

#define LOCAL_SOCKET 0
#define REMOTE_SOCKET 1

#ifndef WIN32
	#define SOCKET_ERROR -1
#endif

////////////////////////////////////////////////////////////////////////////////////
SSHConnection::SSHConnection(const QString &remoteHostAddress, const QString &username, const QString &password, int port, DecoderThread *pDecoder):
m_hostAddress(remoteHostAddress),
m_hostPort(port),
m_username(username),
m_password(password),
mp_session(NULL),
m_emptyReadCount(0),
m_bAlreadySentDisconnect(false),
mp_channelPollingWT(NULL),
mp_socketPollingWT(NULL),
mp_decoder(pDecoder),
m_authMethods( NULL)
{

}

////////////////////////////////////////////////////////////////////////////////////
SSHConnection::~SSHConnection(void)
{
	if ( mp_socketPollingWT)
	{
		if ( mp_socketPollingWT->isRunning())
		{
			mp_socketPollingWT->quit();
			mp_socketPollingWT->wait();
		}
		delete mp_socketPollingWT;
		mp_socketPollingWT = NULL;
	}

	//Kill off the polling worker so that we don't get notifications during our destruction
	if ( mp_channelPollingWT)
	{
		if ( mp_channelPollingWT->isRunning())
		{
			mp_channelPollingWT->quit();
			mp_channelPollingWT->wait();
		}
		delete mp_channelPollingWT;
		mp_channelPollingWT = NULL;
	}

	//Close all open channels and delete our channel list
	for (int i = m_channelList.size()-1; i >= 0; i--)
	{
		closeChannel( i);
		ChannelItem *pChannelItem = m_channelList.takeLast();
		if ( pChannelItem)
		{
			delete pChannelItem;
			pChannelItem = NULL;
		}
	}

	if ( mp_session)
	{
		libssh2_session_disconnect( mp_session, "shutting down normally");
		libssh2_session_free(mp_session);
		mp_session = NULL;
	}

	close( m_socket);

}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::init()
{
	//Let's setup the worker thread to listen for channel activity
	if ( ! setupPollWorkerThread())
		return false;

	//Create our socket to the remote host
	m_socket = createSocket( m_hostAddress.toLatin1().constData(), m_hostPort, REMOTE_SOCKET);
	if ( m_socket == -1)
		return false;

	/* Create a session instance and start it up
	 * This will trade welcome banners, exchange keys, and setup crypto, compression, and MAC layers.
	 * Its VERY important that we use the init_ex function so that we can store a pointer to our class
	 * in its abstract storage area. Now inside the static callback function, we can get back to the
	 * original calling class. */
	emit statusChanged(CS_initLIBSSH2);
	mp_session = libssh2_session_init_ex(NULL, NULL, NULL, this);
	if (mp_session == NULL)
	{
		emit statusChanged(CS_initLIBSSH2Failed);
		m_lastErrorMessage = SSHCONNECTION_3;
		return false;
	}

	/********************
	* Callback function registering
	********************/
	libssh2_session_callback_set(mp_session, LIBSSH2_CALLBACK_DISCONNECT, (void*) &disconnectCallback );

	//Let's startup the session so we can start doing stuff
	if (libssh2_session_startup(mp_session, m_socket )) {
		m_lastErrorMessage = SSHCONNECTION_4;
		return false;
	}

	/* At this point we havn't authenticated,
	 * The first thing to do is check the hostkey's fingerprint against our known hosts */
	emit statusChanged(CS_gettingHostFingerPrint);
	const char *pCharFingerPrint = libssh2_hostkey_hash(mp_session, LIBSSH2_HOSTKEY_HASH_MD5);;
	if (! pCharFingerPrint)
		emit statusChanged(CS_gettingHostFingerPrintFailed);	
	else
	{
		for(unsigned int i = 0; i < 16; i++) 
		{
			if (i)
				m_hostFingerPrint.append(':');

			int dec = (unsigned char) pCharFingerPrint[i];
			m_hostFingerPrint.append("%1");
			m_hostFingerPrint = m_hostFingerPrint.arg(dec, 0, 16); 
		}
		pCharFingerPrint = NULL;
		printf("Remote host fingerprint: %s\n", m_hostFingerPrint.toLatin1().constData() );
		if ( mp_decoder)
		{
			char *msg = new char[128];
			memset(msg, 0x0, 128);
			sprintf( msg, "Host finger print: %s\n\r", m_hostFingerPrint.toLatin1().constData());
			mp_decoder->addToQueue( msg);
		}
	}

	//Gets the authentication methods allowed by this server
	setAuthMethods();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::login()
{
	if ( m_authMethods & AUTH_METHOD_PASSWORD)
	{
		emit statusChanged(CS_loggingInByPassword);
		if (libssh2_userauth_password(mp_session, m_username.toLatin1(), m_password.toLatin1() ) == 0)
		{
			if ( libssh2_userauth_authenticated( mp_session) )		//Sanity check
				return true;
		}
		else
		{
			m_lastErrorMessage = SSHCONNECTION_6;
			emit requestCorrectPassword();
		}

		emit statusChanged(CS_loggingInByPasswordFailed);
		return false;
	}
	else if ( m_authMethods & AUTH_METHOD_KEYBOARD_INTERACTIVE)
	{
		emit statusChanged(CS_loggingInByKeyboardAuth);
		if (libssh2_userauth_keyboard_interactive(mp_session, m_username.toLatin1(), &kbdCallBack) == 0)
		{
			if ( libssh2_userauth_authenticated( mp_session) )		//Sanity check
				return true;
		}
		else
		{
			m_lastErrorMessage = SSHCONNECTION_6;
			emit requestCorrectPassword();
		}

		emit statusChanged(CS_loggingInByKeyboardAuthFailed);
		return false;
	}
	else if ( m_authMethods & AUTH_METHOD_PUBLIC_KEY)
	{
		return false;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////
void
SSHConnection::setAuthMethods()
{
	//Let's check ans see what authentication types are available
	/* check what authentication methods are available */
	emit statusChanged(CS_gettingAllowedAuthTypes);
	const char *pUserAuthList = libssh2_userauth_list(mp_session, m_username.toLatin1() , strlen(m_username.toLatin1()));
	if ( pUserAuthList == NULL)
	{
		emit statusChanged(CS_gettingAllowedAuthTypesFailed);
		m_lastErrorMessage = SSHCONNECTION_5;
		return;
	}
	
	QString authList(pUserAuthList);
	if ( authList.contains("password"))
		m_authMethods |= AUTH_METHOD_PASSWORD;
	if ( authList.contains("keyboard-interactive"))
		m_authMethods |= AUTH_METHOD_KEYBOARD_INTERACTIVE;
	if ( authList.contains("keyboard-interactive"))
		m_authMethods |= AUTH_METHOD_PUBLIC_KEY;
}

////////////////////////////////////////////////////////////////////////////////////
/* STATIC FUNCTION
 * This is the callback function that the libssh2 library will use when doing 
 * keyboard authentication */
void
SSHConnection::kbdCallBack(
const char *name,
int name_len, 
const char *instruction,
int instruction_len,
int num_prompts,
const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
void **abstract)
{
	(void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;

    (void)prompts;
    (void)abstract;

	SSHConnection* this_ = static_cast<SSHConnection *>(*abstract);
	QString password = this_->getPassword(num_prompts);

	if (num_prompts == 1)
	{
		responses[0].text = strdup( password.toLatin1() );
        responses[0].length = strlen( password.toLatin1() );
    }


}

////////////////////////////////////////////////////////////////////////////////////
void
SSHConnection::disconnectCallback(
LIBSSH2_SESSION *session,
int reason,
const char *message,
int message_len,
const char *language,
int language_len,
void **abstract)
{
	printf("woo-woo disconnect called\n");
	SSHConnection *this_ = static_cast<SSHConnection *>(*abstract);
	this_->handleDisconnect(reason, QString(message), QString(language));
}

////////////////////////////////////////////////////////////////////////////////////
void
SSHConnection::handleDisconnect(int reason, const QString &message, const QString &language)
{

	switch (reason)
	{
	case SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT:
	case SSH_DISCONNECT_PROTOCOL_ERROR:
	case SSH_DISCONNECT_KEY_EXCHANGE_FAILED:
	case SSH_DISCONNECT_RESERVED:
	case SSH_DISCONNECT_MAC_ERROR:
	case SSH_DISCONNECT_COMPRESSION_ERROR:
	case SSH_DISCONNECT_SERVICE_NOT_AVAILABLE:
	case SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED:
	case SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE:
	case SSH_DISCONNECT_CONNECTION_LOST:
	case SSH_DISCONNECT_BY_APPLICATION:
	case SSH_DISCONNECT_TOO_MANY_CONNECTIONS:
	case SSH_DISCONNECT_AUTH_CANCELLED_BY_USER:
	case SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE:
	case SSH_DISCONNECT_ILLEGAL_USER_NAME:
	default:
		printf("Disconnect reason: %d\n", reason);
		printf("message: %s\n", message.toLatin1().constData());
		printf("language: %s\n", language.toLatin1().constData());
	}
}

////////////////////////////////////////////////////////////////////////////////////
QString
SSHConnection::getPassword(int num_prompts)
{
	return m_password;

//TO DO: all of it 

	if (num_prompts == 1)
	{
		//We need to write the login crap to the correct console
		//So the user knows whats going on
		return m_password;
	}
	else{
		//QMessageBox::warning(this, "Shit, error!", "Could not login successfully!", QMessageBox::Ok);
		//QMessageBox::warning(NULL, "Login, 2nd attempt", "2nd attemtp at login", QMessageBox::Ok);
		return QString ("");
	}
}

////////////////////////////////////////////////////////////////////////////////////
int
SSHConnection::newChannel(ChannelType channelType, QStringList envVarList, int termWidth, int termHeight)
{
	//TODO: Memork leak central if we ever return a failure!!!!

	QString terminalType;

	switch ( channelType)
	{
		case TERMINAL_VT100:
			terminalType = "vt100";
			break;
		case TERMINAL_VT102:
			terminalType = "vt102";
			break;
		case TERMINAL_XTERM:
			terminalType = "xterm";
			break;
		default:
			m_lastErrorMessage = SSHCONNECTION_7;
			return -1;
	}


	ChannelItem *pChannel = new ChannelItem();
	pChannel->channelType = channelType;
	pChannel->isActive = true;
	pChannel->pChannel = libssh2_channel_open_session(mp_session);

	if ( ! pChannel->pChannel)
	{
		m_lastErrorMessage = SSHCONNECTION_8;
		char * errorMsg = NULL;
		int errorMsgLen = 0;
		int errorNum = libssh2_session_last_error(mp_session, &errorMsg, &errorMsgLen, 0);
		delete pChannel;
		return -1;
	}

	emit statusChanged(CS_requestingPTY);
	if (libssh2_channel_request_pty_ex(pChannel->pChannel, terminalType.toLatin1().constData(),
		terminalType.toLatin1().length(), NULL, 0, termWidth, termHeight,	0, 0) == -1)
	{
		emit statusChanged(CS_requestingPTYFailed);
		m_lastErrorMessage = SSHCONNECTION_9;
		return -1;
	}

	emit statusChanged(CS_setEnvironmentVariables);
	if ( envVarList.count() > 1 )
	{
		if (envVarList.count() %2 == 0)	//List must be an even number
		{
			for (int i = 0; i < envVarList.count(); i+=2)
			{
				if ( libssh2_channel_setenv(
					pChannel->pChannel,
					envVarList[i].toLatin1(),
					envVarList[i+1].toLatin1() ) == -1)
				{
					m_lastErrorMessage = "Error, could not set: %1: %2";
					m_lastErrorMessage.arg(envVarList[i], envVarList[i+1]);	
					emit statusChanged(CS_setEnvironmentVariablesFailed);
				}
			}
		}
	}

	/* Open a SHELL on that pty */
	emit statusChanged(CS_requestingShell);
	if (libssh2_channel_shell(pChannel->pChannel)) {
		emit statusChanged(CS_requestingShellFailed);
		m_lastErrorMessage = SSHCONNECTION_10;
		return -1;
	}
	pChannel->channelStatus = CS_connected;

	/* Turn off blocking so that reads on the channel will return immediately if
	 * there is not data to be read */
	libssh2_channel_set_blocking(pChannel->pChannel, 0);

	//Cool, all went well so far. Put the new channel in the list
	m_channelList.append( pChannel );
	emit statusChanged(CS_connected);

	//Add our channel to our pollingWorker's queue
	mp_channelPollingWT->addChannelToPoll(pChannel->pChannel);

	return m_channelList.count() -1;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::setupPollWorkerThread()
{
	/* Check and see if we have a channelPoller listening for channel data. IF not
	 * then create a new channel poller thread and add this channel to it. */
	if ( ! mp_channelPollingWT)
	{
		mp_channelPollingWT = new PollWorkerThread(this, POLLING_INTERVAL_IN_USEC);
		mp_channelPollingWT->start();
		mp_channelPollingWT->moveToThread( mp_channelPollingWT );
		if ( ! mp_channelPollingWT->readyToWork() )
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::setupSocketWorkerThread()
{
	if ( ! mp_socketPollingWT)
	{
		mp_socketPollingWT = new SocketSelectThread(this);
		mp_socketPollingWT->start();
		mp_socketPollingWT->moveToThread( mp_socketPollingWT);
		mp_socketPollingWT->readyToWork();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::createTunnel(int sourcePort, const QString &destHost, int destPort)
{
	if ( ! mp_socketPollingWT)
		if ( ! setupSocketWorkerThread() )
			return false;

	//Open a local socket for reading and writing
	int sock = createSocket( "127.0.0.1", sourcePort, LOCAL_SOCKET);
	if ( sock == -1)
		return false;

	//Try and create the tunnel
	LIBSSH2_CHANNEL *pChannel = libssh2_channel_direct_tcpip(
		mp_session,
		destHost.toLatin1().constData(),
		destPort);

	if (pChannel)
	{
		ChannelItem *pChannelItem = new ChannelItem();
		pChannelItem->channelType = SSH_TUNNEL;
		pChannelItem->isActive = true;
		pChannelItem->tunnel_destHostAddress = destHost;
		pChannelItem->tunnel_destPort = destPort;
		pChannelItem->tunnel_sourcePort = sourcePort;
		pChannelItem->pChannel = pChannel;

		m_mutex.lock();
		m_channelList.append( pChannelItem );
		m_mutex.unlock();

		//Register this to socket be polled for activity
		mp_socketPollingWT->addSocketToPoll(sock, pChannel);

		return true;
	}
	else
	{
		printf("Failed to create tunnel!\n");
		char *pErrorMsg;
		int msgLen;
		int result = libssh2_session_last_error(mp_session, &pErrorMsg, &msgLen, 1);

		close(sock);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::adjustTerminalSize(int channelNumber, int termWidth, int termHeight, int termWidthPx, int termHeightPx)
{
	//Lets work on getting a pointer to the channel that has the console on it
	LIBSSH2_CHANNEL *pChannel = NULL;
	if ( channelNumber < 0)	//Auto choose the first console connection
	{
		for (int i = 0; i < m_channelList.count(); i++)
		{
			if ( m_channelList[0]->channelType == TERMINAL_VT100)
			{
				pChannel = m_channelList[i]->pChannel;
				break;
			}
		}
	}
	else
	{
		pChannel = m_channelList[channelNumber]->pChannel;
	}

	if (pChannel)
	{
		int result;
		if ( termWidthPx < 0 || termHeightPx < 0)
			result = libssh2_channel_request_pty_size( pChannel, termWidth, termHeight);
		else
			result = libssh2_channel_request_pty_size_ex( pChannel, termWidth, termHeight, termWidthPx, termHeightPx);

		if (! result)
		{
			//Now let's tell our decoder/terminal that the screen size has changed
			if (mp_decoder)
				mp_decoder->adjustScreenSize(termWidth, termHeight);
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
/* It might be a better idea to create a pooling object, one for each channel that
 * runs as a seperate thread. Then when data is found for that channel, it emits
 * a signal with the channel data */
void
SSHConnection::slotReadFromChannel(LIBSSH2_CHANNEL * const pChannel)
{
	QMutexLocker locker( &m_mutex);

	//Make sure the channel is in our list
	ChannelItem *pCItem = NULL;
	int channelNum = -1;
	for (int i = 0; i < m_channelList.size(); i++)
	{
		if ( m_channelList[i]->pChannel == pChannel)
		{
			pCItem = m_channelList[i];
			break;
		}
	}
	
	if ( ! pCItem)
	{
		printf("Asked to read from a channel that we don't have in our list!\n");
		return;
	}

	/* Get the data we need from the member variables so we can unlock our mutex and
	 * let other threads to reads */
	ChannelType channelType		= pCItem->channelType;
	int replyPort					= pCItem->tunnel_replyPort;
	locker.unlock();

	char *pBuf = new char[BUFFER_SIZE];
	if ( ! pBuf)
		return;

   int len = 0,
		space = BUFFER_SIZE -1;	//Space that our buffer has (leaving room for NULL
	
	char *pBufIter;
	pBufIter = pBuf;		//Pointer to use as an iterator

   do{
      len = libssh2_channel_read( pChannel, pBufIter, space);
		if (len > 0)
		{
			space = space - len;		//Mark down how much space is left to write in
			if (space > 0)
				pBufIter += len;		//Move the pBuf pointer to the beginning of blank space
		}
   } while (len > 0 && space > 0);

	
	switch (len)
	{
	case 0:
		break;
	case -37:
		//printf("Recv'd: LIBSSH2_ERROR_EAGAIN\n");
		break;
	default:
		printf("Recv'd Length: %d\n", len);
	}

	if ( channelType != SSH_TUNNEL && libssh2_channel_eof(pChannel) )
	{
		emit statusChanged(CS_shutdownNormal);
		printf("Channel Close detected\n");
	}

	if (space == BUFFER_SIZE-1)
	{
		delete [] pBuf;
		return;
	}
   else
   {
		if ( pBuf == pBufIter)			//We filled the buffer in a single read
			pBufIter += BUFFER_SIZE-1;	//Move our iterator to the end of the string
		//else pBufIter is already at the end of the string
		
		*pBufIter = 0x0;	//Put our NULL terminator in place

		//Now figure out what to do with this data. Do we decode it? Do we write it to a socket?
		if ( channelType == SSH_TUNNEL)
		{
			//Do we have an associated socket to send this data to?
			if ( replyPort)
			{
				int result = send( replyPort, pBuf, (BUFFER_SIZE-1) - space, NULL );
				if ( result == SOCKET_ERROR)
					printf("Error sending recieved channel data to local socket!\n");
				else
					printf("<---- sock %d got %d bytes from chanel\n", replyPort, (BUFFER_SIZE-1) - space);
			}
			else
				printf("Couldn't do anything with recieved data from channel. No reply socket is registered!\n");

			delete [] pBuf;
		}
		else	//Channel is a terminal, decoder the data with our Decoder
		{
			if ( mp_decoder)
				mp_decoder->addToQueue( pBuf);
			else
			{
				printf("Couldn't do anything with data read from the channel. No Decoder!\n");
				delete [] pBuf;
			}
		}
   }
}

////////////////////////////////////////////////////////////////////////////////////
int
SSHConnection::writeToChannel(int channelNumber, const QString data)
{
	if (channelNumber < 0 || channelNumber >= m_channelList.size())
		return 0;

	if ( m_channelList[channelNumber]->channelStatus == CS_connected &&
		m_channelList[channelNumber]->isActive)
	{
		int result = libssh2_channel_write(
			m_channelList[channelNumber]->pChannel,
			data.toLatin1().constData(),
			data.size() );

		if (result > 0)
			return result;

		//We might want to send a CS_disconnected signal out
		//emit statusChanged(CS_disconnected);
		return result;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int
SSHConnection::writeToChannel(LIBSSH2_CHANNEL * const pChannel, const char *pData, int length)
{
	if ( pChannel)
	{
		int result = libssh2_channel_write(
			pChannel,
			pData,
			length );

		if (result > 0)
		{
			//printf("Wrote %d bytes to channel\n", length);
			return result;
		}

		//We might want to send a CS_disconnected signal out
		//emit statusChanged(CS_disconnected);
		return result;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::closeChannel(int channelNumber)
{
	//Sanity check
	if ( channelNumber < 0 ||
		channelNumber >= m_channelList.size() ||
		( ! m_channelList[channelNumber]->isActive) )
		return false;

	//Turn off activity monitoring on this channel if polling is active
	if (mp_channelPollingWT)
		mp_channelPollingWT->removeChannel( m_channelList[channelNumber]->pChannel );

	//Tell our library to close the channel
	if ( libssh2_channel_close( m_channelList[channelNumber]->pChannel ) == -1)
	{
		/* Closing channel failed...
		 * A channel can fail to close if its already closed: by typing in "exit"
		 * in an open terminal */
		printf("Warning: failed to close channel... maybe its already gone home?\n");
	}
	m_channelList[channelNumber]->isActive = false;

	//Let's let the library free the resources associated with this channel
	if ( libssh2_channel_free( m_channelList[channelNumber]->pChannel ) == -1)
		printf("Warning: Failed to free channel resouces\n");

	m_channelList[channelNumber]->pChannel = NULL;
	m_channelList[channelNumber]->channelStatus = CS_shutdownNormal;

	//If all channels are shutdown, close the console
	bool activeChannels = false;
	for (int i = 0; i < m_channelList.size(); i++)
	{
		if ( m_channelList[i]->isActive == true)
		{
			activeChannels = true;
			break;
		}
	}

	if ( ! activeChannels)
		emit statusChanged(CS_shutdownNormal);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool
SSHConnection::registerTunnelConnection(LIBSSH2_CHANNEL * const pChannel, int sockToSendOn)
{
	for (int i = 0; i < m_channelList.size(); i++)
	{
		if ( m_channelList[i]->pChannel == pChannel)
		{
			m_channelList[i]->tunnel_replyPort = sockToSendOn;

			//Register this channel to be polled for activity
			mp_channelPollingWT->addChannelToPoll(pChannel);

			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
int
SSHConnection::createSocket(const char* hostAddress, int port, int socketType)
{
	struct sockaddr_in sin;

	//Let's get our socket ready
	emit statusChanged(CS_creatingSocket);
	int sock = socket(PF_INET, SOCK_STREAM, 0);

#ifdef WIN32
	if (sock  == INVALID_SOCKET)
#else
	if (sock < 0)
#endif
	{
		emit statusChanged(CS_creatingSocketFailed);
		return -1;
	}

#ifndef WIN32
	fcntl(sock , F_SETFL, 0);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons( port );
	
	if (isalpha( hostAddress[0]) )		//Get the IP address and stick that in our "sin"
	{
		emit statusChanged(CS_lookingUpHostsIPAddress);
		// Getting the host by NAME
		struct hostent *remoteHost = gethostbyname( hostAddress );
		if (remoteHost == NULL)
		{
			emit statusChanged(CS_lookingUpHostsIPAddressFailed);
			m_lastErrorMessage = SSHCONNECTION_1;
			close(sock);
			return -1;
		}
		char *ip = inet_ntoa( *(struct in_addr *)*remoteHost->h_addr_list);
		sin.sin_addr.s_addr = inet_addr(ip);
	}
	else		//Getting the host by IP address
		sin.sin_addr.s_addr = inet_addr( hostAddress );

	if ( sin.sin_addr.s_addr == INADDR_NONE) {
		m_lastErrorMessage = SSHCONNECTION_1;
		close(sock);
		return -1;
	}


#ifdef WIN32
			u_long iMode = 1;
#else
		  int x=fcntl(sock, F_GETFL, 0);              // Get socket flags
#endif

	int result;
	switch( socketType)
	{
		case REMOTE_SOCKET:
			//Let's try and connect to the host with our socket
			emit statusChanged(CS_connectingToRemoteHost);
			if ( ::connect(sock , (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
				emit statusChanged(CS_connectingToRemoteHostFailed);
				m_lastErrorMessage = SSHCONNECTION_2;
				close(sock);
				return -1;
			}
			break;
		case LOCAL_SOCKET:
			if (bind(sock, (struct sockaddr*)(&sin), sizeof(sin)) == SOCKET_ERROR)
			{
				close(sock);
				return -1;
			}
			
			//Important: Let's make the socket non-blocking

#ifdef WIN32
			if ( ioctlsocket(sock, FIONBIO, &iMode) )
				printf("Failed to set non-blocking\n");
#else
			fcntl(sock, F_SETFL , x | O_NONBLOCK);   // Add non-blocking flag
#endif

			/*Now lets create a queue to store pending connection requests
			 * will will only let 5 clients at a time try to connect */
			result = listen(sock, 5);
			if ( result == -1)
			{
				printf("Error creating a connection queue\n");
				close(sock);
				return -1;
			}

			break;
		default:
			printf("Bad socket type sent: %d\n", socketType);
			close(sock);
			return -1;
	}

	return sock;
}