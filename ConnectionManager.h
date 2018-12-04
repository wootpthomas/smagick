#pragma once

/***********************
 Qt Includes
*************************/
#include <QString>
#include <QStringList>
#include <QList>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/***********************
 Other Includes
*************************/
#include "AES_encryption.h"
#include "DataTypes.h"

/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/*************************
 Forward Class Declarations
**************************/
class AES_encryption;


class ConnectionManager
{
public:
	ConnectionManager(void);
	~ConnectionManager(void);

	bool isFirstRun() const { return m_isFirstRun; }

	/* Returns true if the connection manager has been initialized
	 * and is ready to recieve data requests. So correct password
	 * is set and stuff. */
	bool isInit() const { return mp_crypto; }

	/* Call this when setting up the password for the first time.
	 * NOTE: This function's intended use is ONLY to be ran when the
	 * user is running Shell Magic for the first time.*/
	bool setNewMasterPassword( const QString password, bool atomic = true);

	/* Used for when the user wants to change the master password.
	 * This will update all encrypted info with ciphertext generated
	 * from the new password.*/
	bool changeMasterPassword( const QString password);

	/* When the user is logging in, we have to test their master password
	 * to see if its correct or not. Use their password and try and decrypt
	 * the magick passphrase. If it decrypts ok, password is correct. Else
	 * password is WRONG B!0tCH.
	 * Upon successful password, the internal encrypt/decrypt object is created
	 * if it has not yet been. */
	bool isCorrectPassword( const QString password);

	/* This function is used to set the connection manager into guest mode */
	void setGuestMode(bool inGuestMode);

	/* Tries to save the given connection information.*/
	bool saveNewConnection(
		const QString hostAddress,
		int hostPort,
		const QString username,
		const QString password,
		bool atomic = true);
	bool saveNewConnection(ConnectionDetails *cd, bool atomic = true);
	
	bool removeConnection(int ID, bool atomic = true);

	//given the primary key of a connection "ConnectionID" and the conection details cd update the information in the database
	bool updateConnectionDetails(int ConnectionID, ConnectionDetails *cd, bool atmoic = true);

	/* retrive a list of connections filtered by the parameters 
	 * This is also the method for getting the connections within a group */
	QList<ConnectionDetails> getConnectionDetails(
		QString groupID = "",
		QString hostFilter = "",
		QString userNameFilter = "",
		QString hostNameFilter = "",
		QString ConnIDFilter = "");

	/* If the given connection ID exists in the database, a valid pointer
	 * to a new ConnectionDetails object is returned. Otherwise a NULL pointer
	 * is returned. Recieveing function takes ownership of pointer. */
	ConnectionDetails * getConnectionDetails(int connectionID);

	QList<GroupDetails> getGroupDetails();

	QStringList getAvailableHosts();

	bool addConnectionToGroup(int groupID, int connectionID, bool atomic = true);
	bool removeConnectionFromGroup(int groupID, int connectionID, bool atmoic = true);

	bool getNewGroup(GroupDetails &output, bool atomic = true);
	bool removeGroup(const GroupDetails &groupToRemove, bool atomic = true);
	bool removeGroup(int GroupID, bool atomic = true);
	bool updateGroupDetails(int groupID, const GroupDetails &gd, bool atomic = true);

private:
	int getIDofHost(QString HostName, int HostPort);
	
	//This function just calls a simple query that 
	//deletes all the hosts not currently used by the connection table
	bool cleanupHostTable(bool atomic = true);

	bool
		m_isFirstRun,					// Is this the 1st time user started sMagick?
		m_isMasterPasswordAlreadySet,	// Is the master password set already?
		m_inGuestMode;					// Is the user running the application in Guest Mode.

	QString
		m_masterPassword,				// Holds the current master password
		m_oldMasterPassword; 			/* Holds the old master password. Only
										 * set when changeMasterPassword is called */


	AES_encryption *mp_crypto;			//Our cryptography object for encrypting/decrypting

	QSqlDatabase m_db;


};
