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
*************************/
#include <QString>
#include <QSettings>

/***********************
 Other Includes
*************************/
#include "QuickConnectDlg.h"
#include "DataTypes.h"



QuickConnectDlg::QuickConnectDlg(QWidget *parent, ConnectionManager *pConnectionMgr):
	QDialog(parent),
	mp_connectionMgr(pConnectionMgr),
	mb_hideAdvOptions(true)
{
	setupUi(this);
	init();
	show();
}

////////////////////////////////////////////////////////////////////

QuickConnectDlg::~QuickConnectDlg(void)
{
}

////////////////////////////////////////////////////////////////////

void QuickConnectDlg::init() 
{
	//Connect signals and slots
	connect(m_moreOptions, SIGNAL(clicked()),
		this, SLOT(slotShowAdvancedOptions()));
	connect(m_hostAddress, SIGNAL(editTextChanged(const QString &)),
		this, SLOT(slotPopulateUsers(const QString &)));
	connect(m_username, SIGNAL(editTextChanged(const QString &)),
		this, SLOT(slotPopulatePassword(const QString &)));

	//hides advanced options
	toggleAdvancedOptions();

	/* To make things easier on the user, populate the
	 * host address, and username drop-downs. */
	if ( mp_connectionMgr && mp_connectionMgr->isInit() )
	{
		/* 1) Get list of hostAddresses & their ports
		 * 2) upon host address selection, populate the usernames drop down
		 * 3) if the user chooses a username thats valid for the host address
		 *	then set the password for that user */
		QStringList availableHosts = mp_connectionMgr->getAvailableHosts();
		if( !availableHosts.isEmpty() )
		{
			m_hostAddress->clear();
			foreach( QString temp, availableHosts)
				m_hostAddress->addItem(temp);
		}
	}

	//Pull out any past history
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);
	QString hostAddress =	settings.value(QSETTINGS_QUICKCONNECT_HOST_ADDRESS, QString("") ).toString();
	int hostPort =				settings.value(QSETTINGS_QUICKCONNECT_PORT, 22 ).toInt();
	QString username =		settings.value(QSETTINGS_QUICKCONNECT_USERNAME, QString("") ).toString();

	if ( hostAddress.length() )	//Found history item
	{
		//Find and select it
		int index = m_hostAddress->findText( hostAddress, Qt::MatchExactly);
		if ( index > -1)
		{
			m_hostAddress->setCurrentIndex(index);
			m_hostPort->setValue( hostPort);

			/* Select all text in the host address so that if the user doesn't want this connection
			 * they can just begin typing ina new one */
			m_hostAddress->lineEdit()->selectAll();
		}
	}

	if ( username.length())
	{
		int index = m_username->findText( username, Qt::MatchExactly);
		if (index > -1)
			m_username->setCurrentIndex(index);
	}

	//If we have a host address and username, make the connect button clickable, otherwise disable it
	m_connect->setEnabled( m_hostAddress->currentText().length() && m_username->currentText().length() );
}

////////////////////////////////////////////////////////////////////

int QuickConnectDlg::exec()
{
	bool execResult = QDialog::exec();

	if ( execResult == QDialog::Accepted)
	{
		//Lets save the history so the next time, we open with it
		QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);
		settings.setValue(QSETTINGS_QUICKCONNECT_HOST_ADDRESS, m_hostAddress->currentText() );
		settings.setValue(QSETTINGS_QUICKCONNECT_PORT, m_hostPort->value() );
		settings.setValue(QSETTINGS_QUICKCONNECT_USERNAME, m_username->currentText() );

		if ( m_saveThisConnection->isChecked() )
		{
			if (mp_connectionMgr)
			{
				mp_connectionMgr->saveNewConnection(
					m_hostAddress->currentText(),
					m_hostPort->value(),
					m_username->currentText(),
					m_password->text() );
			}
		}
	}

	return execResult;
}

////////////////////////////////////////////////////////////////////

ConnectionDetails *QuickConnectDlg::getConnectionDataObj()
{
	ConnectionDetails *cd = new ConnectionDetails();
	if ( cd)
	{
		cd->hostAddress = m_hostAddress->currentText();
		cd->hostPort = m_hostPort->value();
		cd->username = m_username->currentText();
		cd->password = m_password->text();

		//Do we have any Tunnels to create?
      if ( m_tunneling->isChecked() && m_tunnelList->count())
		{
			QRegExp regex("Localhost port (\\d+) to (.*) port (\\d+)");
			for (int i = 0; i < m_tunnelList->count(); i++)
			{
				QListWidgetItem *pListItem = m_tunnelList->item( i);
				if (pListItem)
				{
					QString tunnelRequest = pListItem->text();
					if ( regex.indexIn(tunnelRequest) != -1)
					{
						int count = regex.numCaptures();
						if (count == 3)
						{
							QString 
								sourcePort = regex.cap(1),
								remoteHost = regex.cap(2),
								remotePort = regex.cap(3);

							SSHTunnelItem item;
							item.sourcePort = sourcePort.toInt();
							item.remoteHost = remoteHost;
							item.remotePort = remotePort.toInt();
							
							cd->tunnelList.append( item);
						}
					}
				}
			}
		}
		return cd;
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////

void QuickConnectDlg::slotShowAdvancedOptions()
{
	mb_hideAdvOptions = false;
	toggleAdvancedOptions();
}

////////////////////////////////////////////////////////////////////

void QuickConnectDlg::slotPopulateUsers(const QString &host) {
	if( mp_connectionMgr ) {
		QList<ConnectionDetails> connectionsOnHost = mp_connectionMgr->getConnectionDetails( "", "", "", host);
		m_username->clear();
		if( !connectionsOnHost.isEmpty() ) {
			m_hostPort->setValue(connectionsOnHost.first().hostPort);
			foreach( ConnectionDetails temp, connectionsOnHost) {
				m_username->addItem(temp.username);
			}
		}
	}
	if( !m_username->currentText().isEmpty() )
		slotPopulatePassword( m_username->currentText() );
	
	//If we have a host address and username, make the connect button clickable, otherwise disable it
	m_connect->setEnabled( m_hostAddress->currentText().length() && m_username->currentText().length() );
}

////////////////////////////////////////////////////////////////////

void QuickConnectDlg::slotPopulatePassword(const QString &user) {
	if( mp_connectionMgr ) {
		QList<ConnectionDetails> connectionWUser = mp_connectionMgr->getConnectionDetails( "", "", user,  m_hostAddress->currentText());
		m_password->clear();
		if( !connectionWUser.isEmpty() ) {
			m_password->setText(connectionWUser.first().password);
		}
	}

	//If we have a host address and username, make the connect button clickable, otherwise disable it
	m_connect->setEnabled( m_hostAddress->currentText().length() && m_username->currentText().length() );
}

////////////////////////////////////////////////////////////////////

void QuickConnectDlg::toggleAdvancedOptions()
{
	m_advancedOptionsGroupBox->setHidden( mb_hideAdvOptions);
	m_moreOptions->setVisible( mb_hideAdvOptions);

	m_tunnelList->setMinimumHeight( 50);
	setMaximumHeight( sizeHint().height());
}

////////////////////////////////////////////////////////////////////