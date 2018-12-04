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
#include <QDir>


/***********************
 Other Includes
*************************/
#include "ConnectionManager.h"
#include "AES_encryption.h"




//string defines!!!
// TODO: before release we may need to change a few of these (ex checkstring)
#define CONNECTIONMANAGER_CHECKSTRING "Titties are soooo freakin nice!!!!"
#define CONNECTIONMANAGER_DELETESECURITYCHECK "delete from securityCheck"
#define CONNECTIONMANAGER_INSERTNEWSECURITYCHECK "insert into securityCheck (id, passString) values (0, '%1')"
#define CONNECTIONMANAGER_GETSECURITYCHECK "select passString from securityCheck"

// query for updating connection information
#define CONNECTIONMANAGER_SETCONNECTIONDATA "update connections set hostID = %1, userName = '%2', pass = '%3', passIdEnc = '%4', loginByKeyboard = '%5', cert = '%6' where id = %7"
#define CONNECTIONMANAGER_SETHOSTDATA "update hosts set hostName = '%1', hostPort = '%2' where id = %3"
#define CONNECTIONMANAGER_INSERTHOSTDATA "insert into hosts (hostName, hostPort) values ('%1', %2)"

// query for getting connectiondata from the database
// I have also included defines so that we can access the results of this query without remebering the order of the collumns in the query.
#define CONNECTIONMANAGER_GETCONNECTIONDATA "select conn.id, hostId, userName, pass, passIsEnc, loginByKeyboard, cert, hosts.hostName, hosts.hostPort FROM connections conn, hosts"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_WITH_GROUPS ", groupCrossRef gcr WHERE gcr.connID = conn.id and hosts.id = conn.hostID and gcr.groupID = %1"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_WO_GROUPS " WHERE hosts.id = conn.hostID"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_HOSTID " and conn.hostId = %1"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_HOSTNAME " and hosts.hostName = '%1'"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_USERNAME " and conn.userName = '%1'"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_CONNID " and conn.id = %1"
#define CONNECTIONMANAGER_GETCONNECTIONDATA_SINGLE_CONNID " WHERE conn.id=%1 AND hosts.id = conn.hostID"
// defines for easy access to the collumns return from the query
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_CONNID 0
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTID 1
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_USERNAME 2
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORD 3
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORDENC 4
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_LOGINBYKEYBAORD 5
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_CERTIFICATE 6
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTNAME 7
#define CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTPORT 8

// query for getting group data from the database
#define CONNECTIONMANAGER_GETGROUPDATA "select id, groupName, groupDiscription from groups"
#define CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPID 0
#define CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPNAME 1
#define CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPDISCRIPTION 2

//database table creation queries
#define CONNECTIONMANAGER_CREATE_SECURITYCHECKTABLE "create table securityCheck (id int, passString varchar(50))"
#define CONNECTIONMANAGER_CREATE_CONNECTIONSTABLE "create table connections (id integer primary key, hostID int, userName varchar(50), pass varchar(50), passIsEnc boolean, loginByKeyboard boolean, cert varchar(200))"
#define CONNECTIONMANAGER_CREATE_HOSTSTABLE "create table hosts (id integer primary key, hostName varchar(50), hostPort int)"
#define CONNECTIONMANAGER_CREATE_GROUPSTABLE "create table groups (id integer primary key, groupName varchar(50), groupDiscription varchar(200))"
#define CONNECTIONMANAGER_CREATE_GROUPCROSSREFTABLE "create table groupCrossRef (id integer primary key, groupID int, connID int)"


ConnectionManager::ConnectionManager(void):
	m_isMasterPasswordAlreadySet(false),
	mp_crypto(NULL),
	m_inGuestMode(true)
{

	QString DBFilePath = QDir::homePath().append( QDir::separator() );
	DBFilePath += QString(".shellMagick").append( QDir::separator() );
	DBFilePath = QDir::fromNativeSeparators(DBFilePath);

	QDir dir( DBFilePath);
	if ( ! dir.exists() && ! dir.mkdir( DBFilePath) )
	{
		QString errMsg = "Cannot create Shell Magick database file.\n";
		errMsg += "You do not have write access to create the directory:\n";
		errMsg += DBFilePath;
		errMsg += "Shell Magick will not be able to save any connection details";

		QMessageBox::critical(0, "Database file creation failed.", errMsg, QMessageBox::Ok);
		return;
	}

	DBFilePath += "default.mgk";
	m_db = QSqlDatabase::addDatabase("QSQLITE");
	m_db.setDatabaseName( DBFilePath );
	if( !m_db.open() ) {
		QSqlError err = m_db.lastError();
		QString errMsg = 
			QString("Unable to establish a database connection.\n") + 
			QString("Make sure the qsqlite.dll file is in the\n<Shell Magick>\\sqldrivers\ndirectory\n") +
			QString("and that you have write access to:\n") +
			DBFilePath +"\n"+
			QString("Database info: ") + err.databaseText() + QString("\n\n");

		QMessageBox::critical(0, "Cannot open database", errMsg, QMessageBox::Ok);

		return;
	}

	if( m_db.tables().count() > 0 ) 
	{
		//this is not the first time it's been ran
		m_isFirstRun = false;
	} else {
		m_isFirstRun = true;
	}

	//m_isFirstRun = false;	//Only set to true for testing!

	if (m_isFirstRun)
	{
		m_isMasterPasswordAlreadySet = false;
		QSqlQuery query;
		query.exec(CONNECTIONMANAGER_CREATE_SECURITYCHECKTABLE);
		query.exec(CONNECTIONMANAGER_CREATE_CONNECTIONSTABLE);
		query.exec(CONNECTIONMANAGER_CREATE_HOSTSTABLE);
		query.exec(CONNECTIONMANAGER_CREATE_GROUPSTABLE);
		query.exec(CONNECTIONMANAGER_CREATE_GROUPCROSSREFTABLE);
	}
	else
	{
		m_isMasterPasswordAlreadySet = true;
	}
}

ConnectionManager::~ConnectionManager(void)
{
	if (mp_crypto)
		delete mp_crypto;
}

void ConnectionManager::setGuestMode(bool inGuestMode)
{
	m_inGuestMode = inGuestMode;
}


QList<ConnectionDetails> ConnectionManager::getConnectionDetails(QString groupID, QString hostFilter, QString userNameFilter, QString hostNameFilter, QString ConnIDFilter) 
{
	bool usingNameFilter = !userNameFilter.isEmpty();
	bool usingHostFilter = !hostFilter.isEmpty();
	bool usingGroupFilter = !groupID.isEmpty();
	bool usingHostNameFilter = !hostNameFilter.isEmpty();
	bool usingConnIDFilter = !ConnIDFilter.isEmpty();

	//make the list to return
	QList<ConnectionDetails> ConnectionList;

	//now pull connections from database
	QString sql = CONNECTIONMANAGER_GETCONNECTIONDATA;
	QSqlQuery query;


	if( usingGroupFilter )
	{
		sql += CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_WITH_GROUPS;
		groupID = groupID.replace( QString("'"), QString("''") );
		sql = sql.arg(groupID);
	} else {
		sql += CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_WO_GROUPS;
	}
	if( usingNameFilter )
	{
		userNameFilter = userNameFilter.replace( QString("'"), QString("''") );
		sql += QString(CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_USERNAME).arg(userNameFilter);
	}
	if( usingHostFilter )
	{
		hostFilter = hostFilter.replace( QString("'"), QString("''") );
		sql += QString(CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_HOSTID).arg(hostFilter);
	}	
	if( usingHostNameFilter )
	{
		hostNameFilter = hostNameFilter.replace( QString("'"), QString("''") );
		sql += QString(CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_HOSTNAME).arg(hostNameFilter);
	}
	if( usingConnIDFilter )
	{
		ConnIDFilter = ConnIDFilter.replace( QString("'"), QString("''") );
		sql += QString(CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_CONNID).arg(ConnIDFilter);
	}

	query.exec(sql);
	if( query.isActive() )
	{
		if( query.first() )
		{
			do
			{
				ConnectionDetails temp;
				temp.isDirty = false;
				temp.hostAddress = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTNAME).toString();
				temp.hostPort = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTPORT).toInt();
				temp.loginByRSA = !query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_LOGINBYKEYBAORD).toBool();
				temp.loginByUsernamePassword = !temp.loginByRSA;
				temp.encriptPasswd = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORDENC).toBool();
				temp.password = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORD).toString();
				if( m_inGuestMode ) {
					temp.password = "";
				} else 	if( temp.encriptPasswd ) 
					temp.password = mp_crypto->decrypt(temp.password);
				temp.username = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_USERNAME).toString();
				temp.connectionID = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_CONNID).toInt();
//				temp.certificate = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_CERTIFICATE).toString();

				ConnectionList.append(temp);
			} while( query.next() );
		}
	}
	return ConnectionList;
}

ConnectionDetails *
ConnectionManager::getConnectionDetails(int connectionID)
{
	ConnectionDetails *pCD = NULL;

	QString sql = CONNECTIONMANAGER_GETCONNECTIONDATA;
	sql += QString(CONNECTIONMANAGER_GETCONNECTIONDATA_SINGLE_CONNID).arg(connectionID) ;
	QSqlQuery query;

	query.exec(sql);
	if( query.isActive() )
	{
		if( query.first() )
		{
			pCD = new ConnectionDetails;

			pCD->isDirty = false;
			pCD->hostAddress = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTNAME).toString();
			pCD->hostPort = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_HOSTPORT).toInt();
			pCD->loginByRSA = !query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_LOGINBYKEYBAORD).toBool();
			pCD->loginByUsernamePassword = ! pCD->loginByRSA;
			pCD->encriptPasswd = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORDENC).toBool();
			pCD->password = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_PASSWORD).toString();
			if( m_inGuestMode ) {
				pCD->password = "";
			} else 	if( pCD->encriptPasswd ) 
				pCD->password = mp_crypto->decrypt( pCD->password);
			pCD->username = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_USERNAME).toString();
			pCD->connectionID = query.value(CONNECTIONMANAGER_GETCONNECTIONDATA_COLLUMN_CONNID).toInt();
		}
	}
	
	return pCD;
}

bool ConnectionManager::removeConnection(int ID, bool atomic) 
{
	QSqlQuery query;
	QString sql = "delete from connections where id = %1";
	sql = sql.arg(ID);
	
	if( atomic )
		m_db.transaction();

	query.exec(sql);
	if( atomic )
	{
		if( query.numRowsAffected() < 1 )
		{
			m_db.rollback();
			return false;
		}
		m_db.commit();
	}
	cleanupHostTable(atomic);
	return true;
}

bool ConnectionManager::cleanupHostTable(bool atomic)
{
	QSqlQuery query;
	QString sql = "delete from hosts where id not in (select distinct hostID from connections)";
	
	if( atomic )
		m_db.transaction();

	query.exec(sql);
	
	if( atomic ) {
		if( query.numRowsAffected() < 1 )
		{
			m_db.rollback();
			return false;
		}
		m_db.commit();
	}
	return true;
}

QStringList ConnectionManager::getAvailableHosts() {
	QSqlQuery query;
	QString sql = "select distinct hostName from hosts";
	QStringList returnValue;

	query.exec(sql);

	if( query.isActive() )
	{
		if( query.first() )
		{
			do
			{
				returnValue.append(query.value(0).toString());
			} while( query.next() );
		}
	}
	return returnValue;

}

bool ConnectionManager::updateConnectionDetails(int ConnectionID, ConnectionDetails *cd, bool atomic)
{
	QSqlQuery query;
	QString sql; 
	QString newHostname;
	int newHostport;
	int newHostID;

	if( atomic )
		m_db.transaction();

	if( ConnectionID > 0 )
	{
		sql = CONNECTIONMANAGER_GETCONNECTIONDATA;
		sql += CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_WO_GROUPS;
		sql += CONNECTIONMANAGER_GETCONNECTIONDATA_WHERE_CONNID;
		sql = sql.arg(ConnectionID);
		query.exec(sql);
		//check to see if connection is already in table
		if( query.isActive() && query.first() )
		{
			newHostname = cd->hostAddress;
			newHostport = cd->hostPort;
			newHostID = getIDofHost(newHostname, newHostport);

			sql = "update connections set hostID = %1, userName = '%2', pass = '%3', passIsEnc = '%4', loginByKeyboard = '%5', cert = '%6' where id = %7";
			sql = sql.arg(newHostID);      // set the host id
			sql = sql.arg(cd->username.replace( QString("'"), QString("''") ));   // set the username
			if( cd->encriptPasswd ) 
			{
				//encript the password then put it in the query
				sql = sql.arg(mp_crypto->encrypt(cd->password).replace( QString("'"), QString("''") ));
			} else {
				sql = sql.arg(cd->password.replace( QString("'"), QString("''") )); // store the unencripted password in the query
			}
			sql = sql.arg((cd->encriptPasswd)?"true":"false");
			sql = sql.arg((cd->loginByUsernamePassword)?"true":"false");
			sql = sql.arg(cd->RSA_publicKey.replace( QString("'"), QString("''") ));
			sql = sql.arg(ConnectionID);
			query.exec(sql);

			if( atomic ) 
			{
				if( query.numRowsAffected() < 1 )
				{
					//it failed
					m_db.rollback();
					return false;
				}
				//were done and the connection has been updated
				m_db.commit();
			}
			cd->isDirty = false;
			return true;
		}
	} else {
		if( atomic )
		{
			//connection isn't in the database we need to put it there
			m_db.rollback();
		}
		return this->saveNewConnection(cd);
	}
	return false;
}
int ConnectionManager::getIDofHost(QString HostName, int HostPort)
{
	
	//return -1 if there was an error
	//due to the problems with nested transactions all transactions and rollbacks and commits are expected to be
	// handled outside of this function
	//analize the data and determine what to do with the host information if it has changed
	// we have 3 cases to look for
	//   1. the host information doesn't show up in the database at all
	//   2. the host port is different then that in the database
	//   3. the information in the database and that provided are identical

	int tempHostPort;
	int tempHostID;
	QSqlQuery query;
	QString sql;
	sql = "select id, hostPort from hosts where upper(hostName) = upper('%1')";
	sql = sql.arg(HostName.replace( QString("'"), QString("''") ));
	query.exec(sql);

	if( query.isActive() && query.first() )
	{
		tempHostID = query.value(0).toInt();
		tempHostPort = query.value(0).toInt();
		if( tempHostPort != HostPort )
		{
			//hostport changed just update the hostport in the database
			sql = "update hosts set hostPort = %1 where id = %2";
			sql = sql.arg(HostPort);
			sql = sql.arg(tempHostID);
			query.exec(sql);
			if( query.numRowsAffected() < 1 )
			{
				//it didn't work!!!
				return -1;
			}
			// it was successfull the hostport will be the same
		}
		//it was successfull either the host ports are the same or the host port was successfully changed to tempHostPort
		return tempHostPort;
	} else {
		//this data was not in the database
		sql = "insert into hosts (hostPort, hostName) values ('%1', '%2')";
		sql = sql.arg(HostPort);
		sql = sql.arg(HostName.replace( QString("'"), QString("''") ));

		query.exec(sql);
		if( query.numRowsAffected() < 1 )
		{
			//it didn't work
			return -1;
		}
		sql = "select last_insert_rowid()";
		query.exec(sql);
		if( query.isActive() && query.first() )
		{
			return query.value(0).toInt();
		} else {
			//yet another check for database problems
			return -1;
		}
	}
}

QList<GroupDetails> ConnectionManager::getGroupDetails() 
{
	//make the list to return
	QList<GroupDetails> GroupList;
	QString sql = CONNECTIONMANAGER_GETGROUPDATA;
	QSqlQuery query;

	query.exec(sql);
	if( query.isActive() )
	{
		if( query.first() )
		{
			do
			{
				GroupDetails temp;
				temp.groupID = query.value(CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPID).toInt();
				temp.groupName = query.value(CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPNAME).toString();
				temp.groupDiscription = query.value(CONNECTIONMANAGER_GETGROUPDATA_COLLUMN_GROUPDISCRIPTION).toString();
				GroupList.append(temp);
			} while( query.next() );
		}
	}

	return GroupList;
}

bool ConnectionManager::addConnectionToGroup(int groupID, int connectionID, bool atomic)
{
	QSqlQuery query;
	QString sql = "insert into groupCrossRef (groupID, connID) values (%1, %2)";
	sql = sql.arg(groupID).arg(connectionID);

	if( atomic )
		m_db.transaction();

	query.exec(sql);
	if( atomic )
	{
		if( query.numRowsAffected() < 1 )
		{
			m_db.rollback();
			return false;
		} else {
			m_db.commit();
			return true;
		}
	} 
	return true;
}

bool ConnectionManager::removeConnectionFromGroup(int groupID, int connectionID, bool atomic)
{
	QSqlQuery query;
	QString sql = "delete from groupCrossRef where id = (select id from groupCrossRef where groupID = %1 and connID = %2 LIMIT 1)";
	sql = sql.arg(groupID).arg(connectionID);
	if( atomic )
		m_db.transaction();

	query.exec(sql);

	if( atomic )
	{
		if( query.numRowsAffected() < 1 )
		{
			m_db.rollback();
			return false;
		} else {
			m_db.commit();
			return true;
		}
	}
	return true;
}

bool ConnectionManager::setNewMasterPassword(const QString newPassword, bool atomic)
{
	/* We check here to make sure its not already set. If we were to just reset
	 * the password and its already set and being used, then the ciphertext now
	 * will not match the proper password. */
	if (m_isMasterPasswordAlreadySet)
		return false;

	QSqlQuery query;
	QString passCheckString;

	mp_crypto = new AES_encryption( newPassword );

	passCheckString = mp_crypto->encrypt(CONNECTIONMANAGER_CHECKSTRING);
	passCheckString = passCheckString.replace( QString("'"), QString("''") );

	if ( atomic )
		m_db.transaction();

	query.exec(CONNECTIONMANAGER_DELETESECURITYCHECK);
	QString sql = QString(CONNECTIONMANAGER_INSERTNEWSECURITYCHECK).arg(passCheckString);
	query.exec(sql);

	if( atomic ) {
		if( query.numRowsAffected() < 1 ) 
			m_db.rollback();
		else
			m_db.commit();
	}


	m_masterPassword = newPassword;
	m_isMasterPasswordAlreadySet = true;
	return true;
}

bool ConnectionManager::getNewGroup(GroupDetails &output, bool atomic)
{
	QString uniqueIdentifier = "0";
	QSqlQuery query;
	QStringList endTags = QStringList();
	QString temp;
	QString sql = "select substr(groupName, 12, length(groupName)- 11)  as tail "
					"from groups "
					"where groupName like '<New Group>%' "
					"    and tail <> ''";
	
	query.exec(sql);
	if( query.isActive() && query.first() )
	{
		do
		{
			endTags.append(query.value(0).toString());
		} while( query.next() );
	}

	for( int i = 0; i < endTags.count(); i++ )
	{
		endTags[i] = endTags[i].remove( QRegExp("[^12345678990]+"));
		if( uniqueIdentifier.toInt() < endTags[i].toInt() )
		{
			uniqueIdentifier = endTags[i];
		}
	}
	//we now have the highest endTag
	uniqueIdentifier = QString::number( uniqueIdentifier.toInt() + 1 );
	//uniqueIdnetifer now holds a free endTag

	if( atomic )
		m_db.transaction();

	sql = "insert into groups (groupName) values ('%1')";
	uniqueIdentifier.insert(0, "<New Group>");
	sql = sql.arg(uniqueIdentifier);
	query.exec(sql);

	if( query.numRowsAffected() < 1 )
	{
		if( atomic ) 
			m_db.rollback();
		return false;
	} else {
		sql = "select last_insert_rowid()";
		query.exec(sql);
		if( query.isActive() && query.first() )
		{
			output.groupName = uniqueIdentifier;
			output.groupID = query.value(0).toInt();
			output.groupDiscription = "";
			if( atomic )
				m_db.commit();
			return true;
		} else {
			if( atomic )
				m_db.rollback();
			return false;
		}
	}
}

bool ConnectionManager::removeGroup(const GroupDetails &groupToRemove, bool atomic)
{
	return removeGroup(groupToRemove.groupID);
}

bool ConnectionManager::removeGroup(int GroupID, bool atomic)
{
	QSqlQuery query;
	QString sql;
	sql = "delete from groups where id = %1";
	sql = sql.arg(GroupID);
	if( atomic )
		m_db.transaction();

	query.exec(sql);
	if( query.numRowsAffected() < 1 )
	{
		if( atomic )
			m_db.rollback();
		return false;
	} else {
		sql = "delete from groupCrossRef where groupId = %1";
		sql = sql.arg(GroupID);
		query.exec(sql);
		if( query.numRowsAffected() < 1 )
		{
			if( atomic )
				m_db.rollback();
			return false;
		} else {
			if( atomic )
				m_db.commit();
			return true;
		}
	}
}

bool ConnectionManager::updateGroupDetails(int groupID, const GroupDetails &gd, bool atomic)
{
	QSqlQuery query;
	QString sql;
	QString groupName = gd.groupName;
	QString groupDiscription = gd.groupDiscription;
	sql = "update groups set groupName = '%1', groupDiscription = '%2' where id = %3";
	groupName = groupName.replace( QString("'"), QString("''") );
	groupDiscription = groupDiscription.replace( QString("'"), QString("''") );
	sql = sql.arg(groupName).arg(groupDiscription).arg(groupID);

	if( atomic )
		m_db.transaction();

	query.exec(sql);
	if( query.numRowsAffected() < 1 )
	{
		//it failed
		if( atomic )
			m_db.rollback();
		return false;
	} else {
		if( atomic )
			m_db.commit();
		return true;
	}
}

bool ConnectionManager::changeMasterPassword( const QString password)
{
	AES_encryption *reserve = mp_crypto;

	//First pull out all the current data
	QList<ConnectionDetails> totalList = getConnectionDetails();

	/*update our internal encrypt/decrypter */
	mp_crypto = new AES_encryption( password);		//point to the new one
	
	m_db.transaction();
	
	//loop through all the ConnectionDetails and update them
	foreach(ConnectionDetails temp, totalList) {
		if( !this->updateConnectionDetails(temp.connectionID, &temp) ) {
			m_db.rollback();
			delete mp_crypto;
			mp_crypto = reserve;
			return false;
		}
	}
	
	QString passCheckString = mp_crypto->encrypt(CONNECTIONMANAGER_CHECKSTRING);
	passCheckString = passCheckString.replace( QString("'"), QString("''") );


	QSqlQuery query;
	query.exec(CONNECTIONMANAGER_DELETESECURITYCHECK);
	QString sql = QString(CONNECTIONMANAGER_INSERTNEWSECURITYCHECK).arg(passCheckString);
	query.exec(sql);

	if( query.numRowsAffected() < 1 ) {
		m_db.rollback();
		delete mp_crypto;
		mp_crypto = reserve;
		return false;
	} else
		m_db.commit();


	//And also update our internal password stuff
	m_oldMasterPassword = m_masterPassword;
	m_masterPassword = password;

	delete reserve;					//delete the old one

	return true;
}


bool ConnectionManager::isCorrectPassword( const QString password)
{

	//encrpyt the test phrase with this key
	AES_encryption *pNewEncryptDecrypt = new AES_encryption( password);
	QString encriptedTestPhrase = pNewEncryptDecrypt->encrypt(CONNECTIONMANAGER_CHECKSTRING);
	QString databaseCheckPhrase;

	//get the checkString from the database
	QSqlQuery query;
	query.exec( CONNECTIONMANAGER_GETSECURITYCHECK );
	if( !query.isActive() )
	{
		printf( "The database call failed!!!\n" );
		delete pNewEncryptDecrypt;
		return false;
	} else {
		query.first();
		databaseCheckPhrase = query.value(0).toString();
		if( encriptedTestPhrase == databaseCheckPhrase )
		{
			if( mp_crypto )
				delete mp_crypto;

			mp_crypto = pNewEncryptDecrypt;
			pNewEncryptDecrypt = NULL;
			return true;
		}
		delete pNewEncryptDecrypt;
		return false;
		
	}
}

bool ConnectionManager::saveNewConnection(
		const QString in_hostAddress,
		int hostPort,
		const QString in_username,
		const QString in_passwordIN,
		bool atomic)
{
	int hostID;
	QString password;
	QString sql;
	QString hostAddress = in_hostAddress;
	QString username = in_username;
	QString passwordIN = in_passwordIN;

	if( mp_crypto)
	{
		password = mp_crypto->encrypt(passwordIN);
	} else {
		password = passwordIN;
	}
	QSqlError sqlError;

	//prep for database insertion
	password = password.replace( QString("'"), QString("''") );
	hostAddress = hostAddress.replace( QString("'"), QString("''") );
	username = username.replace( QString("'"), QString("''") );
	passwordIN = passwordIN.replace( QString("'"), QString("''") );

	if( atomic )
		m_db.transaction();
		
		QSqlQuery query;
		// TODO: move the querys to a #define up top
		sql = "Select id from hosts where hostName = '%1' and hostPort = %2";
		sql = sql.arg(hostAddress).arg(hostPort);
		query.exec(sql);
		if( query.first() )
		{
			//this host is already in the database save it's ID
			hostID = query.value(0).toInt();
			sql = "Select id from connections where hostID = %1 and userName = '%2'";
			sql = sql.arg(hostID).arg(username);
			query.exec(sql);
			if( query.first() )
			{
				//this connection already exists
				if( atomic )
					m_db.rollback();
				printf("\nsaveNewConnection FAILED!!  Connection already exists! 1\n");
				return false;
			} else {
				//the host exists but not the exact connection.
				sql = "insert into connections (hostID, userName, pass, passIsEnc, loginByKeyboard) values (%1, '%2', '%3', 'true', 'true')";
				sql = sql.arg(hostID).arg(username).arg(password);
				query.exec(sql);
				if( query.numRowsAffected() < 1 )
				{
					//something didn't work
					if( atomic )
						m_db.rollback();
					printf("\nsaveNewConnection FAILED!!  failed to insert into connections! 2\n");
					return false;
				} else {
					if( atomic )
						m_db.commit();
					printf("\nsaveNewConnection SUCCESS!!\n");
					return true;
				}
			}
		} else {
			//this host does not exist were safe to assume that this connection doesn't exist
			sql = "insert into hosts (hostName, hostPort) values ('%1', %2)";
			sql = sql.arg(hostAddress).arg(hostPort);
			query.exec(sql);
			if( query.numRowsAffected() < 1 )
			{
				if( atomic )
					m_db.rollback();
				printf("\nsaveNewConnection FAILED!!  failed to insert into hosts! 3\n");
				return false;
			} else {
				sql = "select last_insert_rowid()";
				query.exec(sql);
				if( query.first() )
				{
					hostID = query.value(0).toInt();
				} else {
					if( atomic )
						m_db.rollback();
					printf("\nsaveNewConnection FAILED!!  failed to find last_insert_rowid()! 4\n");
					return false;
				}
				sql = "insert into connections (hostID, userName, pass, passIsEnc, loginByKeyboard) values (%1, '%2', '%3', 'true', 'true')";
				sql = sql.arg(hostID).arg(username).arg(password);
				query.exec(sql);
				if( query.numRowsAffected() < 1 )
				{
					//something didn't work
					if( atomic )
						m_db.rollback();
					printf("\nsaveNewConnection FAILED!!  failed to insert into connections! 5\n");
					sqlError = m_db.lastError();
					printf(sqlError.text().toAscii().data());
					printf("\n");
					return false;
				} else {
					if( atomic )
						m_db.commit();
					printf("\nsaveNewConnection SUCCESS!!\n");
					return true;
				}
			}
		}
}

bool ConnectionManager::saveNewConnection(ConnectionDetails *cd, bool atomic)
{
	//just calls the other saveNewConnection
	this->saveNewConnection(cd->hostAddress, cd->hostPort, cd->username, cd->password, atomic);
	return true;
}


//AES_encryption *mp_encrypter = new AES_encryption("This is my pwd");
//
//QString cipherText = mp_encrypter->encrypt("Now is the time for all good men to come to the aide...");
//QString plainText = mp_encrypter->decrypt( cipherText );
