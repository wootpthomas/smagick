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

#include "UserPreferences.h"


/*************************
 Overloaded operators
**************************/
//Allows us to write our class out in serialized form
QDataStream& UserPreferences::operator << (QDataStream& stream)
{
	stream<< m_font;

	return stream;
}

//Allows us to read in from serialized data to our user preferences object
QDataStream& UserPreferences::operator >> (QDataStream& stream)
{
	stream>> m_font;

	return stream;
}


////////////////////////////////////////////////////////////////////
UserPreferences::UserPreferences()
{
	mp_mutex = new QMutex(QMutex::Recursive );


	/***********************
	 * Restore saved UserPreferences
	 **********************/
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);

	//font type and size
	m_font = qvariant_cast<QFont>( settings.value(QSETTINGS_USER_PREFS_FONT, QFont("Courier", 8) ));
	
	//cursor color
	m_cursorColor = qvariant_cast<QColor>( settings.value(QSETTINGS_USER_PREFS_CURSOR_COLOR, QColor(Qt::green) ) );
	
	//Foreground colors
	m_fgColor0_39 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_0_39, QColor(Qt::white)) );
	m_fgColor30 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_30, QColor(Qt::black)) );
	m_fgColor31 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_31, QColor(Qt::red)) );
	m_fgColor32 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_32, QColor(Qt::green)) );
	m_fgColor33 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_33, QColor(Qt::yellow)) );
	m_fgColor34 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_34, QColor(Qt::blue)) );
	m_fgColor35 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_35, QColor(Qt::magenta)) );
	m_fgColor36 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_36, QColor(Qt::cyan)) );
	m_fgColor37 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_FGC_37, QColor(Qt::white)) );

	//Background colors
	m_bgColor0_49 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_0_49, QColor(Qt::black)) );
	m_bgColor40 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_40, QColor(Qt::black)));
	m_bgColor41 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_41, QColor(Qt::red)) );
	m_bgColor42 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_42, QColor(Qt::green)) );
	m_bgColor43 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_43, QColor(Qt::yellow)) );
	m_bgColor44 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_44, QColor(Qt::blue)) );
	m_bgColor45 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_45, QColor(Qt::magenta)) );
	m_bgColor46 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_46, QColor(Qt::cyan)) );
	m_bgColor47 = qvariant_cast<QColor>(settings.value(QSETTINGS_USER_PREFS_BGC_47, QColor(Qt::lightGray)) );

	mp_fgColor = new QHash<int, QColor>();
	mp_bgColor = new QHash<int, QColor>();
	populateColorHashs();
	
	m_bReopenPreviousConnections = true;
	mp_PixMapHash = new PixMapHash(m_font, mp_bgColor, mp_fgColor);

#ifdef _DEBUG
	m_debugToolTipEnabled = true;
#endif
}

////////////////////////////////////////////////////////////////////
UserPreferences::~UserPreferences(void)
{
	saveSettings();

	QPixmap *tempPixmapPointer;
	QHash<ConsoleChar, QPixmap *>::iterator pixMapIterator = mp_PixMapHash->begin();
	while( pixMapIterator != mp_PixMapHash->constEnd() )
	{
		tempPixmapPointer = pixMapIterator.value();
		pixMapIterator = mp_PixMapHash->erase(pixMapIterator);
		delete tempPixmapPointer;
	}
	if(mp_PixMapHash)
	{
		delete mp_PixMapHash;
		mp_PixMapHash = NULL;
	}

	if (mp_mutex)
	{
		delete mp_mutex;
		mp_mutex = NULL;
	}

	if(mp_fgColor)
	{
		mp_fgColor->clear();
		delete mp_fgColor;
		mp_fgColor = NULL;
	}
	
	if(mp_bgColor)
	{
		mp_bgColor->clear();
		delete mp_bgColor;
		mp_bgColor = NULL;
	}

}

#ifdef _DEBUG
////////////////////////////////////////////////////////////////////
bool
UserPreferences::debugToolTipEnabled()
{
	QMutexLocker lock( mp_mutex);
	return m_debugToolTipEnabled;
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::toggleDebugToolTip()
{
	QMutexLocker lock( mp_mutex);
	m_debugToolTipEnabled = !m_debugToolTipEnabled;
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::toggleDebugToolTip(bool toggleStatus) 
{
	QMutexLocker lock( mp_mutex);
	m_debugToolTipEnabled = toggleStatus;
}
#endif

////////////////////////////////////////////////////////////////////
void
UserPreferences::populateColorHashs()
{
	//foreground colors
	mp_fgColor->insert(0, m_fgColor0_39);
	mp_fgColor->insert(39, m_fgColor0_39);
	mp_fgColor->insert(30, m_fgColor30);
	mp_fgColor->insert(31, m_fgColor31);
	mp_fgColor->insert(32, m_fgColor32);
	mp_fgColor->insert(33, m_fgColor33);
	mp_fgColor->insert(34, m_fgColor34);
	mp_fgColor->insert(35, m_fgColor35);
	mp_fgColor->insert(36, m_fgColor36);
	mp_fgColor->insert(37, m_fgColor37);
	mp_fgColor->insert(99, m_cursorColor);

	//Background colors
	mp_bgColor->insert(0, m_bgColor0_49);
	mp_bgColor->insert(49, m_bgColor0_49);
	mp_bgColor->insert(40, m_bgColor40);
	mp_bgColor->insert(41, m_bgColor41);
	mp_bgColor->insert(42, m_bgColor42);
	mp_bgColor->insert(43, m_bgColor43);
	mp_bgColor->insert(44, m_bgColor44);
	mp_bgColor->insert(45, m_bgColor45);
	mp_bgColor->insert(46, m_bgColor46);
	mp_bgColor->insert(47, m_bgColor47);
	mp_bgColor->insert(99, m_cursorColor);
}

////////////////////////////////////////////////////////////////////
QPixmap
UserPreferences::getPixmapForCharacter(const ConsoleChar *CC)
{
	QMutexLocker lock( mp_mutex);
	return *mp_PixMapHash->value(*CC);;
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::clearPixmapCache()
{
	if ( mp_PixMapHash)
		delete mp_PixMapHash;

	mp_PixMapHash = new PixMapHash(m_font, mp_bgColor, mp_fgColor);
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::setFont(const QFont &font)
{
	QMutexLocker lock( mp_mutex);
	m_font = font;
	//generateHash();
}

////////////////////////////////////////////////////////////////////
QFont
UserPreferences::font()
{
	QMutexLocker lock( mp_mutex);
	return m_font;
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::saveSettings()
{
	QSettings settings(QSETTINGS_ORGANIZATION, QSETTINGS_APP_NAME);

	//Save the font type and size
	settings.setValue(QSETTINGS_USER_PREFS_FONT, m_font );
	
	//cursor color
	settings.setValue(QSETTINGS_USER_PREFS_CURSOR_COLOR, m_cursorColor );
	//Foreground colors
	settings.setValue(QSETTINGS_USER_PREFS_FGC_0_39, mp_fgColor->value( 39) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_30, mp_fgColor->value(30) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_31, mp_fgColor->value(31) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_32, mp_fgColor->value(32) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_33, mp_fgColor->value(33) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_34, mp_fgColor->value(34) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_35, mp_fgColor->value(35) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_36, mp_fgColor->value(36) );
	settings.setValue(QSETTINGS_USER_PREFS_FGC_37, mp_fgColor->value(37) );
	//Background colors
	settings.setValue(QSETTINGS_USER_PREFS_BGC_0_49, mp_bgColor->value(49) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_40, mp_bgColor->value(40) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_41, mp_bgColor->value(41) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_42, mp_bgColor->value(42) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_43, mp_bgColor->value(43) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_44, mp_bgColor->value(44) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_45, mp_bgColor->value(45) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_46, mp_bgColor->value(46) );
	settings.setValue(QSETTINGS_USER_PREFS_BGC_47, mp_bgColor->value(47) );
}

////////////////////////////////////////////////////////////////////
void
UserPreferences::setColor(int colorNum, QColor qColor, bool foreground)
{
	switch(colorNum)
	{
		case 0:
			if (foreground)
				m_fgColor0_39 = qColor;
			else
				m_bgColor0_49 = qColor;
			break;
		case 30:		m_fgColor30 = qColor;			break;
		case 31:		m_fgColor31 = qColor;			break;
		case 32:		m_fgColor32 = qColor;			break;
		case 33:		m_fgColor33 = qColor;			break;
		case 34:		m_fgColor34 = qColor;			break;
		case 35:		m_fgColor35 = qColor;			break;
		case 36:		m_fgColor36 = qColor;			break;
		case 37:		m_fgColor37 = qColor;			break;
		case 39:		m_fgColor0_39 = qColor;			break;
		case 40:		m_bgColor40 = qColor;			break;
		case 41:		m_bgColor41 = qColor;			break;
		case 42:		m_bgColor42 = qColor;			break;
		case 43:		m_bgColor43 = qColor;			break;
		case 44:		m_bgColor44 = qColor;			break;
		case 45:		m_bgColor45 = qColor;			break;
		case 46:		m_bgColor46 = qColor;			break;
		case 47:		m_bgColor47 = qColor;			break;
		case 49:		m_bgColor0_49 = qColor;			break;
	}
}

////////////////////////////////////////////////////////////////////
QColor
UserPreferences::getColor(int colorID, bool isBackground)
{
	if ( isBackground )
		return mp_bgColor->value(colorID);
	else
		return mp_fgColor->value(colorID);
}

////////////////////////////////////////////////////////////////////
/*		Foreground Colours
	30	Black
	31	Red
	32	Green
	33	Yellow
	34	Blue
	35	Magenta
	36	Cyan
	37	White */
/* Background Colours
	40	Black
	41	Red
	42	Green
	43	Yellow
	44	Blue
	45	Magenta
	46	Cyan
	47	White */

		