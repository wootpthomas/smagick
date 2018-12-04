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


/*********************
 Other includes
*********************/
#include "CreateChangeMasterPasswordDlg.h"
#include "ConnectionManager.h"

#define CREATECHANGEMASTERPASSWORDDLG_1 "Strong passwords use a combination of\nalphanumeric and special characters"

CreateChangeMasterPasswordDlg::CreateChangeMasterPasswordDlg(UsageType type, ConnectionManager *pConnectionMgr):
	QDialog(),
	m_usageType(type),
	mp_connectionMgr(pConnectionMgr)
{
	setupUi(this);
	init();


}

CreateChangeMasterPasswordDlg::~CreateChangeMasterPasswordDlg(void)
{
}

void CreateChangeMasterPasswordDlg::init()
{
	if (m_usageType == NEW_PASSWORD)
	{
		m_oldPassword->setVisible( false);
		m_oldPasswordLabel->setVisible( false);
	}

	//Get our password strength meter setup for first use
	m_passwordStrengthMeter->setMinimum(0);
	m_passwordStrengthMeter->setMaximum(10);
	m_passwordStrengthMeter->setValue(0);
	m_passwordStrengthMeter->setTextVisible(false);

	setMinimumHeight( minimumSizeHint().height() );
	setMaximumHeight( minimumSizeHint().height() );

	/* connect up the signals with the appropriate slots */
	connect( m_password, SIGNAL( textChanged(QString)), this, SLOT(slotSecurityCheck(QString)));
	connect( m_passwordConfirm, SIGNAL( textChanged(QString)), this, SLOT(slotConfirmPassword()));
	connect( m_okButton, SIGNAL(clicked()), this, SLOT(accept()));

	m_password->setToolTip( CREATECHANGEMASTERPASSWORDDLG_1 );

	slotSecurityCheck("");
}

void CreateChangeMasterPasswordDlg::slotSecurityCheck(QString newText)
{
	//Let's compute how strong the password is
	int strength = 0,
		upperCase = 0,		//Ex: ABCDEF
		numbers = 0,		//Ex: 12345
		specials = 0,		//Ex: `~!@#$%^&*()_+-=
		extraSpecials = 0;	//Ex: {}|:"<>?,./;'[]\
	
	QString strSpecials = "~`!@#$%^&*()_+-=";
	QString strExtraSpecials = "{}|:\"<>?,./;'[]\\";

	if (newText.length() > 0 && newText.length() < 4)
		strength += 1;
	else if ( newText.length() > 3)	//Length 4 or bigger
	{
		strength += 1;
		
		if ( newText.length() > 7)	//Length 8 or bigger
			strength += 1;

		//a point for 1 uppercase, 2 for 3 or more uppercase (65-90)
		for (int i = 0; i < newText.length(); i++)
		{
			if (newText[i].isUpper())
				upperCase ++;
		}
		if (upperCase > 0 && upperCase < 3 )
			strength += 1;
		else if (upperCase >= 3)
			strength += 2;

		//a point for numbers, 2 for 3 or more numbers
		for (int i = 0; i < newText.length(); i++)
		{
			if (newText[i].isDigit() )
				numbers ++;
		}
		if (numbers > 0 && numbers < 3)
			strength += 1;
		else if (numbers >= 3)
			strength += 2;

		//a point for special chars, 2 for 3 or more special chars
		for (int i = 0; i < newText.length(); i++)
		{
			if (strSpecials.contains( QString(newText[i]) ))
				specials++;
		}
		if (specials > 0 && specials < 3)
			strength += 1;
		else if (specials >= 3)
			strength += 2;

		//a point for something other than alphanumeric or special chars
		for (int i = 0; i < newText.length(); i++)
		{
			if (strExtraSpecials.contains( QString(newText[i]) ))
				extraSpecials++;
		}
		if (extraSpecials > 0 && extraSpecials < 3)
			strength += 1;
		else if (extraSpecials >= 3)
			strength += 2;
	}

	//Show the results
	m_passwordStrengthMeter->setValue( strength);
	
	if (strength == 0) this->m_smartAssComment->setText("Please enter in something!");
	if (strength == 1) this->m_smartAssComment->setText("Your password is...<b>WEAK!</b>");
	if (strength == 2) this->m_smartAssComment->setText("I could guess that in my sleep.");
	if (strength == 3) this->m_smartAssComment->setText("Hackers could crack this with paper and pencil!");
	if (strength == 4) this->m_smartAssComment->setText("You can do better...I hope!");
	if (strength == 5) this->m_smartAssComment->setText("Still just plain weak!");
	if (strength == 6) this->m_smartAssComment->setText("Might as well let your cat dance on the keyboard!");
	if (strength == 7) this->m_smartAssComment->setText("The <i>matrix</i> still has you...");
	if (strength == 8) this->m_smartAssComment->setText("Now you're making it better.");
	if (strength == 9) this->m_smartAssComment->setText("Password is approaching 1337 status!");
	if (strength >= 10) this->m_smartAssComment->setText("An army of monkeys on typewriters wouldn't guess this password!");

}

void CreateChangeMasterPasswordDlg::slotConfirmPassword()
{
	m_okButton->setEnabled( m_password->text() == m_passwordConfirm->text() );
}

void CreateChangeMasterPasswordDlg::accept()
{
	if( m_usageType == NEW_PASSWORD )
	{
		if( m_password->text().compare( m_passwordConfirm->text(), Qt::CaseSensitive ) != 0 ) {
			//passwords don't match, alert the user!!!
			QMessageBox::critical(0, "Error", "Passwords Don't Match!", QMessageBox::Ok);
			return;
		}
		this->mp_connectionMgr->setNewMasterPassword( m_password->text() );
		this->mp_connectionMgr->setGuestMode(false);
	} else {
		if( !this->mp_connectionMgr->isCorrectPassword( m_oldPassword->text() ) ) {
			//They entered the wrong old password
			QMessageBox::critical(0, "Error", "Old Password Incorrect!", QMessageBox::Ok);
			return;
		}
		if( m_password->text().compare( m_passwordConfirm->text(), Qt::CaseSensitive ) != 0 ) {
			//passwords don't match, alert the user!!!
			QMessageBox::critical(0, "Error", "Passwords Don't Match!", QMessageBox::Ok);
			return;
		}
		this->mp_connectionMgr->changeMasterPassword( m_password->text() );
	}
	QDialog::accept();
}