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

//http://www.cplusplus.com/doc/tutorial/classes2.html


/***********************
 Qt Includes
*************************/
#include <QFont>
#include <QColor>
#include <QString>
#include <QFontMetrics>
#include <QDataStream>
#include <QMutexLocker>
#include <QMutex>
#include <QPainter>
#include <QSettings>


/***********************
 Other Includes
*************************/
#include "ConsoleChar.h"
#include "PixMapHash.h"
#include "DataTypes.h"


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif



/*************************
 Class Declaration
**************************/
class UserPreferences
{
	//Since the user prefs dialog directly modifies this class, we give it full-spread-eagle access
	friend class UserPreferencesDlg;

public:
	UserPreferences( );
	~UserPreferences(void);

	QColor background(int bgColor = 0) { return getColor(bgColor, true); }

	/** THREAD SAFE
	 * Sets the font type */
	void setFont(const QFont &font);

	/** THREAD SAFE
	 * Gets the font */
	QFont font();

	/* Used to set a specific color */
	void setColor(int, QColor, bool foreground = true);

	/* Get the color for the given colorID */
	QColor getColor(int colorID, bool isBackground);

	/* Gets the cursor color */
	QColor cursorColor() { return m_cursorColor; }

	/* Returns true if we should re-open closed connections */
	bool reOpenPreviousConnections() { return m_bReopenPreviousConnections; }

	void setReOpenPreviousConnections(bool value) { m_bReopenPreviousConnections = value; }

	/* Sets the cursor color */
	void setCursorColor( QColor color) { m_cursorColor = color; }

	/* Returns a pixmap for the given consoleChar */
	QPixmap getPixmapForCharacter(const ConsoleChar *CC);

	/* Clears out the pixmaps from our hash. Allows us to re-create our
	 * pixmap hash using new colors */
	void clearPixmapCache();

	/* Saves the settings for this object */
	void saveSettings();

	/******************
	Operator overloads
	******************/
	QDataStream &operator<<(QDataStream& stream);
	QDataStream &operator>>(QDataStream& stream);

private:
	/* populate/re-populate the color hash */
	void populateColorHashs();

	bool
		m_bReopenPreviousConnections;

	PixMapHash
		*mp_PixMapHash;

	QHash<int, QColor>
		*mp_bgColor,
		*mp_fgColor;

	QFont
		m_font;

	QMutex
		*mp_mutex;

	QColor
		//Foreground colors
		m_fgColor0_39,	//Default foreground color, 39 is also same
		m_fgColor30,
		m_fgColor31,
		m_fgColor32,
		m_fgColor33,
		m_fgColor34,
		m_fgColor35,
		m_fgColor36,
		m_fgColor37,

		//Background colors
		m_bgColor0_49,	//Default background color, 49 is also same
		m_bgColor40,
		m_bgColor41,
		m_bgColor42,
		m_bgColor43,
		m_bgColor44,
		m_bgColor45,
		m_bgColor46,
		m_bgColor47,

		m_cursorColor;

#ifdef _DEBUG
public:
	bool debugToolTipEnabled();
	void toggleDebugToolTip();
	void toggleDebugToolTip(bool);

private:
	bool m_debugToolTipEnabled;
#endif
};
