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


/**********************
 * Includes
 *********************/
#include <stdio.h>
#include "ConsoleChar.h"

/**********************
 * Initialize statics
 *********************/
#ifdef CC_MEM_OPTIMIZE
	QMutex * ConsoleChar::mp_staticLock = new QMutex();

	ConsoleChar * ConsoleChar::mp_freeList = NULL;
#endif

////////////////////////////////////////////////////////////////////
ConsoleChar::ConsoleChar(QChar qChar):
sChar(qChar),
BGColor( 0),
FGColor( 0),
blink(false),
colorEffect( )
#ifdef CC_MEM_OPTIMIZE
,
mp_next(NULL)
#endif
{
	colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;
}


#ifdef CC_MEM_OPTIMIZE
////////////////////////////////////////////////////////////////////
void *
ConsoleChar::operator new(unsigned int size)
{
	QMutexLocker lock( mp_staticLock);
	//Check our list and see if we have any freeNodes that we can give out
	if (mp_freeList)
	{
		ConsoleChar *pNew = mp_freeList;
		mp_freeList = mp_freeList->mp_next;
		return pNew;
	}
	//else

	return ::operator new(size);
}

////////////////////////////////////////////////////////////////////
void
ConsoleChar::operator delete(void* pObj)
{
	QMutexLocker lock( mp_staticLock);

	if ( ! pObj)
		return;

	ConsoleChar *pCC = (ConsoleChar*)pObj;
	pCC->mp_next = mp_freeList;
	mp_freeList = pCC;
}

////////////////////////////////////////////////////////////////////
void
ConsoleChar::cleanupFreeList()
{
	if (mp_staticLock)
	{
		delete mp_staticLock;
		mp_staticLock = NULL;
	}

	ConsoleChar *pCC;
	while (mp_freeList)
	{
		pCC = mp_freeList;
		mp_freeList = mp_freeList->mp_next;
		::operator delete(pCC);
	}
}
#endif
