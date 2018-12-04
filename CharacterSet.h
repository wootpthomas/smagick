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
***********************/
#include <QChar>


/***********************
 Includes
***********************/
#include "DataTypes.h"


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/***********************
 Class Definition
***********************/
class CharacterSet
{
public:
	/* Constructor sets the default character set to G0 */
	CharacterSet(CharSet charSet = CHAR_SET_G0);

	~CharacterSet(void);

	/* Changes the current character set */
	void changeCharSet(CharSet newCharSet) { m_currentCharSet = newCharSet; }

	/*Takes a character and returns a QChar representing the current
	 * character set we are drawing in */
	QChar translate(const QChar c) const;

private:
	CharSet
		m_currentCharSet;		//Holds the current character set we draw in
};
