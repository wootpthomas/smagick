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
#include <QDialog>
#include <QColorDialog>
#include <QFontComboBox>
#include <QList>



/***********************
 Other Includes
*************************/
#include "UserPreferencesDlg.h"
#include "UserPreferences.h"



////////////////////////////////////////////////////////////////////
UserPreferencesDlg::UserPreferencesDlg(MainWindowDlg * const pParent, UserPreferences *pUserPrefs):
QDialog( pParent),
mp_mainWin( pParent),
mp_userPrefs(pUserPrefs)
{
	setupUi(this);
	init();

	show();
}

////////////////////////////////////////////////////////////////////
UserPreferencesDlg::~UserPreferencesDlg(void)
{
}


////////////////////////////////////////////////////////////////////
int
UserPreferencesDlg::exec()
{
	mp_userPrefs->saveSettings();

	return QDialog::exec();
}


////////////////////////////////////////////////////////////////////
void
UserPreferencesDlg::init()
{
	QFont font = mp_userPrefs->font();

	m_fontComboBox->setFontFilters( QFontComboBox::MonospacedFonts);
	m_fontComboBox->setCurrentFont( font );
	m_fontSize->setValue( font.pointSize() );

	//Now we get to set all the buttons to their default colors
	mp_color0_39Btn->setButtonColor( mp_userPrefs->m_fgColor0_39 );
	mp_color30Btn->setButtonColor( mp_userPrefs->m_fgColor30 );
	mp_color31Btn->setButtonColor( mp_userPrefs->m_fgColor31 );
	mp_color32Btn->setButtonColor( mp_userPrefs->m_fgColor32 );
	mp_color33Btn->setButtonColor( mp_userPrefs->m_fgColor33 );
	mp_color34Btn->setButtonColor( mp_userPrefs->m_fgColor34 );
	mp_color35Btn->setButtonColor( mp_userPrefs->m_fgColor35 );
	mp_color36Btn->setButtonColor( mp_userPrefs->m_fgColor36 );
	mp_color37Btn->setButtonColor( mp_userPrefs->m_fgColor37 );
	
	mp_color0_49Btn->setButtonColor( mp_userPrefs->m_bgColor0_49 );
	mp_color40Btn->setButtonColor( mp_userPrefs->m_bgColor40 );
	mp_color41Btn->setButtonColor( mp_userPrefs->m_bgColor41 );
	mp_color42Btn->setButtonColor( mp_userPrefs->m_bgColor42 );
	mp_color43Btn->setButtonColor( mp_userPrefs->m_bgColor43 );
	mp_color44Btn->setButtonColor( mp_userPrefs->m_bgColor44 );
	mp_color45Btn->setButtonColor( mp_userPrefs->m_bgColor45 );
	mp_color46Btn->setButtonColor( mp_userPrefs->m_bgColor46 );
	mp_color47Btn->setButtonColor( mp_userPrefs->m_bgColor47 );

	mp_cursorColorBtn->setButtonColor( mp_userPrefs->m_cursorColor );

	//Connect up buttons to their actions
	//Foreground color buttons
	QList<QObject*> widgets = this->mp_foregroundGrpBox->children();
	for (int i = 0; i < widgets.size(); i++)
	{
		ColorButton *pBtn = dynamic_cast<ColorButton*>( widgets[i] );
		if ( pBtn)
		{
			connect( pBtn, SIGNAL(getNewColor(ColorButton *)),
				this, SLOT(slotChangeColor(ColorButton  *)));
		}
	}

	//Background color buttons
	widgets = this->mp_backgroundGrpBox->children();
	for (int i = 0; i < widgets.size(); i++)
	{
		ColorButton *pBtn = dynamic_cast<ColorButton*>( widgets[i] );
		if ( pBtn)
		{
			connect( pBtn, SIGNAL(getNewColor(ColorButton *)),
				this, SLOT(slotChangeColor(ColorButton  *)));
		}
	}

	//Connect up the font type and size stuff
	connect( m_fontComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(slotFontChanged(int)));
	connect( m_fontSize, SIGNAL(valueChanged(int)),
		this, SLOT(slotFontSizeChanged(int)));
	connect( mp_cursorColorBtn, SIGNAL(getNewColor(ColorButton *)),
		this, SLOT(slotChangeCursorColor(ColorButton *)));
}

//////////////////////////////////////////////////////////////////////
//void
//UserPreferencesDlg::resetButtonToDefaultColors()
//{
//
//	}

////////////////////////////////////////////////////////////////////
void
UserPreferencesDlg::slotChangeColor(ColorButton  *pButton)
{
	QColor userCanceled( 0xffffffff);

	//Fire up the color choosing dialog
	QColor color = QColorDialog::getRgba();

	if ( color == userCanceled || color == pButton->buttonColor())
		return;

	//Set the changed button color
	pButton->setButtonColor( color);

	//Messy part......
	//Now reset all the UserPreferences QColor objects to the button colors
	mp_userPrefs->m_fgColor0_39 = mp_color0_39Btn->buttonColor();
	mp_userPrefs->m_fgColor30 = mp_color30Btn->buttonColor();
	mp_userPrefs->m_fgColor31 = mp_color31Btn->buttonColor();
	mp_userPrefs->m_fgColor32 = mp_color32Btn->buttonColor();
	mp_userPrefs->m_fgColor33 = mp_color33Btn->buttonColor();
	mp_userPrefs->m_fgColor34 = mp_color34Btn->buttonColor();
	mp_userPrefs->m_fgColor35 = mp_color35Btn->buttonColor();
	mp_userPrefs->m_fgColor36 = mp_color36Btn->buttonColor();
	mp_userPrefs->m_fgColor37 = mp_color37Btn->buttonColor();

	mp_userPrefs->m_bgColor0_49 = mp_color0_49Btn->buttonColor();
	mp_userPrefs->m_bgColor41 = mp_color41Btn->buttonColor();
	mp_userPrefs->m_bgColor42 = mp_color42Btn->buttonColor();
	mp_userPrefs->m_bgColor43 = mp_color43Btn->buttonColor();
	mp_userPrefs->m_bgColor44 = mp_color44Btn->buttonColor();
	mp_userPrefs->m_bgColor45 = mp_color45Btn->buttonColor();
	mp_userPrefs->m_bgColor46 = mp_color46Btn->buttonColor();
	mp_userPrefs->m_bgColor47 = mp_color47Btn->buttonColor();

	mp_userPrefs->populateColorHashs();
	mp_userPrefs->clearPixmapCache();

	/* Now tell the mainwindow that user preferences changed so it can tell
	 * widgets to resize and stuff */
	emit stuffChanged();
}

////////////////////////////////////////////////////////////////////
void
UserPreferencesDlg::slotFontChanged(int index)
{
	if (index == -1)
		return;

	mp_userPrefs->m_font = m_fontComboBox->currentFont();
	mp_userPrefs->m_font.setPointSize( m_fontSize->value() );
	
	mp_userPrefs->clearPixmapCache();

	/* Now tell the mainwindow that user preferences changed so it can tell
	 * widgets to resize and stuff */
	emit stuffChanged();
}

////////////////////////////////////////////////////////////////////
void
UserPreferencesDlg::slotFontSizeChanged(int value)
{
	mp_userPrefs->m_font.setPointSize( value);
	
	mp_userPrefs->clearPixmapCache();

	/* Now tell the mainwindow that user preferences changed so it can tell
	 * widgets to resize and stuff */
	emit stuffChanged();
}

////////////////////////////////////////////////////////////////////
void
UserPreferencesDlg::slotChangeCursorColor(ColorButton * pButton)
{
	QColor userCanceled( 0xffffffff);

	//Fire up the color choosing dialog
	QColor color = QColorDialog::getRgba();

	if ( color == userCanceled )
		return;

	mp_userPrefs->setCursorColor( color);
	pButton->setButtonColor( color);

	emit stuffChanged();
}