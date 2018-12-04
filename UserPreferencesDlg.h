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
#include "ui_UserPreferences.h"
#include "ColorButton.h"
#include "MainWindowDlg.h"



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
class UserPreferences;

class UserPreferencesDlg :
	public QDialog,
	public Ui_UserPreferences
{
	Q_OBJECT

public:
	UserPreferencesDlg(MainWindowDlg * const pParent, UserPreferences *pUserPrefs);
	~UserPreferencesDlg(void);

	/* Override so we can update our preferences if the user clicked ok
	 * otherwise we discard changes if the user clicked cancel */
	int exec();

signals:
	/* if a user changes a preference, this signal gets sent out to the world */
	void stuffChanged();

private slots:
	/* Called when the user changes font colors */
	void slotChangeColor(ColorButton *pButton);

	/* Called when the user changes font type */
	void slotFontChanged(int index);

	/* Called when the font size changes */
	void slotFontSizeChanged(int value);

	/* Called when the user changes the cursor color */
	void slotChangeCursorColor(ColorButton * pButton);

private:
	/* Initialize any needed stuff*/
	void init();

	UserPreferences
		*mp_userPrefs;

	MainWindowDlg
		* const mp_mainWin;
};

