/***************************************************************************
 *   Copyright (C) 2008 by Paul Thomas, Jeremy Combs                       *
 *   thomaspu@gmail.com, combs.jeremy@gmail.com                            *
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
#include <QHash>
#include <QPixmap>
#include <QFontMetrics>
#include <QFont>
#include <QPainter>

/***********************
 Other Includes
*************************/
#include "ConsoleChar.h"

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
class PixMapHash :
	public QHash<ConsoleChar, QPixmap *>
{
public:
	PixMapHash(QFont UP, QHash<int, QColor> *bgColor, QHash<int, QColor> *fgColor);
	~PixMapHash();

	//int insert(const ConsoleChar &key, const QPixmap *&value);

	QPixmap *value(const ConsoleChar &key);
private:
	QPixmap *generatePixMap(const ConsoleChar &CC, QFontMetrics &fm);

	QHash<int, QColor> *mp_fgColor;
	QHash<int, QColor> *mp_bgColor;
	QFont m_font;
};
