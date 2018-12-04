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
#include <QMutexLocker>

/***********************
 Includes
*************************/
#include "Terminal.h"
#include <exception>


////////////////////////////////////////////////////////////////////
Terminal::Terminal(int termWidth, int termHeight, bool isGraphicalTerm):
m_screenWidth(termWidth),
m_screenHeight(termHeight),
m_maxHistorySize( 75),
m_scrollRegionStart(0),
m_scrollRegionEnd(termHeight-1),
m_insertPosX(0),
m_insertPosY(0),
mp_translator(NULL),
m_savedCursorPosX(0),
m_savedCursorPosY(0),
m_bConsoleIsDirty(true),
m_bIsCursorVisible(true),
m_bAutoWrapping(true),
m_bIsGraphicalTerm(isGraphicalTerm)
{
	//Create a new character translator
	mp_translator = new CharacterSet();

	//Create our recursive mutex
	mp_mutex = new QMutex(QMutex::Recursive);

	//Create a new screen
	mp_screen = createNewScreenPointerArray(m_screenWidth, m_screenHeight);

	printf("Initial terminal size is %d cols, %d rows\n", m_screenWidth, m_screenHeight);
}

////////////////////////////////////////////////////////////////////
Terminal::~Terminal(void)
{
	if (mp_screen)
		deleteScreen( m_screenWidth, m_screenHeight);

	ConsoleChar **ppRow;
	while ( m_history.size() )
	{
		ppRow = m_history.takeFirst();
		if ( ppRow)
		{
			for (int x = 0; x < m_historyWidth; x++)
				delete ppRow[x];

			delete [] ppRow;
		}
	}

	if (mp_translator)
	{
		delete mp_translator;
		mp_translator = NULL;
	}

	if ( mp_mutex)
	{
		delete mp_mutex;	
		mp_mutex = NULL;
	}
}

////////////////////////////////////////////////////////////////////
RenderInfo*
Terminal::getRenderInfo()
{
	QMutexLocker lock( mp_mutex);
	if ( ! m_bConsoleIsDirty)
		return NULL;

	RenderInfo *pInfo = new RenderInfo();

	if ( m_bIsGraphicalTerm)
	{	//Create a new screen
		pInfo->pScreen = createNewScreenPointerArray(m_screenWidth, m_screenHeight);

		//copy screen contents
		for (int y = 0; y < m_screenHeight; y++)
			for (int x = 0; x < m_screenWidth; x++)
				(* pInfo->pScreen[y][x]) = (* mp_screen[y][x]);
		
		pInfo->termHeight = m_screenHeight;
		pInfo->cursorPosY = m_insertPosY;
	}
	else
	{	//Create a new screen + history
		pInfo->pScreen = createNewScreenPointerArray(m_screenWidth, m_screenHeight+m_history.size() );

		//copy the history
		for (int y = 0; y < m_history.size(); y++)
		{
			for (int x = 0; x < m_screenWidth; x++)
			{
				if ( x < m_historyWidth)
				{
					ConsoleChar **ppRow = m_history[y];
					(* pInfo->pScreen[y][x]) = (* ppRow[x]);
				}
			}
		}

		//copy screen contents
		int offset = m_history.size();
		for (int y = offset; y < m_screenHeight+offset; y++)
			for (int x = 0; x < m_screenWidth; x++)
				(* pInfo->pScreen[y][x]) = (* mp_screen[y-offset][x]);
	
		pInfo->termHeight = m_screenHeight + offset;
		pInfo->cursorPosY = m_insertPosY + offset;
	}

	pInfo->termWidth = m_screenWidth;
	pInfo->visibleTermHeight = m_screenHeight;
	pInfo->cursorPosX = m_insertPosX;
	
	
	m_bConsoleIsDirty = false;
	setEntireConsoleDirty( false); //remove the dirty bits from all CCs

	return pInfo;
}

////////////////////////////////////////////////////////////////////
void
Terminal::setConsoleDirty()
{
	setEntireConsoleDirty(true);
	m_bConsoleIsDirty = true;
}

////////////////////////////////////////////////////////////////////
void
Terminal::setMaxHistorySize(int rows)
{
	if (! m_bIsGraphicalTerm && m_history.size() > rows)
	{
		for (int i = 0; i < m_history.size()-rows; i++)
		{
			delete m_history[0];
			m_history.removeFirst();
		}

		m_maxHistorySize = rows;
	}
}

////////////////////////////////////////////////////////////////////
void
Terminal::setEntireConsoleDirty(bool isDirty)
{
	for (int y = 0; y < m_screenHeight; y++)
	{
		for (int x = 0; x < m_screenWidth; x++)
		{
			if (isDirty)
				mp_screen[y][x]->colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;
			else
				mp_screen[y][x]->colorEffect &= COLOR_EFFECT_REMOVE_CHAR_IS_DIRTY;
		}
	}	
}

////////////////////////////////////////////////////////////////////
void
Terminal::setAutoWrapToNewLine(bool value)
{
	QMutexLocker lock( mp_mutex);
	m_bAutoWrapping = value;
	//if ( value)
	//	printf("Auto line wrapping ENABLED\n");
	//else
	//	printf("Auto line wrapping DISABLED\n");
}

////////////////////////////////////////////////////////////////////
void
Terminal::setScreenSize(int cols, int rows)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	if (cols < 1 || rows < 1)
	{
		printf("OMFG!!! CRASH WARNING!!!\n");
		return;
	}

	//First we resize our history array and copy over what we can
	if ( cols != m_historyWidth)
	{
		for( int i = 0; i < m_history.size(); i++)
		{
			ConsoleChar **ppRow = m_history[i];
			m_history[i] = new ConsoleChar *[cols];

			//Copy over what we can
			int newWidth;
			if ( cols > m_historyWidth)
				newWidth = cols;
			else
				newWidth = m_historyWidth;

			for (int x = 0; x < newWidth; x++)
			{
				if ( x < m_historyWidth && x < cols)
				{
					m_history[i][x] = ppRow[x];
					ppRow[x] = NULL;
				}
				else if ( x < cols)
				{
					m_history[i][x] = new ConsoleChar();
				}
			}
			delete [] ppRow;
		}
		m_historyWidth = cols;
	}

	/* If the new screen size cuts off our insertion point X or Y, then we delete the
	 * whole screen and expect the console to redraw it all.
	 * Else:
	 *   we copy as much of the old screen to our new one and keep the cursor/insertion
	 * position. */	
	if ( rows < m_insertPosY || cols < m_insertPosX)
	{
		//Reset our insertion locations
		m_insertPosX = 0;
		m_insertPosY = 0;

		//delete the old screen
		if (mp_screen)
			deleteScreen( m_screenWidth, m_screenHeight);

		//Set the new screen size
		m_screenWidth = cols;
		m_screenHeight = rows;

		//Create a new crazy screen pointer array
		mp_screen = createNewScreenPointerArray(m_screenWidth, m_screenHeight);
	}
	else
	{
		//Create the new screen array, but don't fill in any elements yet
		ConsoleChar ***pNewScreen = new ConsoleChar** [rows];
		for (int i = 0; i < rows; i++)
			pNewScreen[i] = new ConsoleChar *[cols];

		/*Copy over as much of the old screen as possible and set the other
		 * items to NULLs */
		for (int y = 0; y < rows; y++)
		{
			for (int x = 0; x < cols; x++)
			{
				if (y < m_screenHeight && x < m_screenWidth)
				{
					pNewScreen[y][x] = mp_screen[y][x];
					mp_screen[y][x] = NULL;
				}
				else
					pNewScreen[y][x] = new ConsoleChar();
			}
		}

		//Now delete what's left of the old screen
		if (mp_screen)
			deleteScreen( m_screenWidth, m_screenHeight);

		//Set the new screen size and move the new screen
		m_screenWidth = cols;
		m_screenHeight = rows;
		mp_screen = pNewScreen;
		pNewScreen = NULL;
	}

	//Reset scrolling regions to the entire screen
	m_scrollRegionStart = 0;
	m_scrollRegionEnd = m_screenHeight-1;
}

////////////////////////////////////////////////////////////////////
void
Terminal::insertText( const ConsoleChar &cs, const std::string &text)
{
	try{
		QMutexLocker lock( mp_mutex);
		m_bConsoleIsDirty = true;

		if ( m_insertPosY >= m_screenHeight)
			m_insertPosY = m_screenHeight-1;
		if ( m_insertPosX >= m_screenWidth)
			m_insertPosX = m_screenWidth-1;

		unsigned int i = 0;
		for ( ; i < text.size(); i++)
		{ 
			if (m_insertPosX >= m_screenWidth )
			{
			  //Next insert goes off the end of the screen. do we wrap text to next line?
			  if ( m_bAutoWrapping)
			  {
				  m_insertPosY++;
				  if ( m_insertPosY == m_screenHeight)	//We ran out of room, need to insert a newline
					  insertNewLine();

				  m_insertPosX = 0;
			  }
			  else
				  break;		//Theres no point in going further since we are not autowrapping
			}

			(*mp_screen[m_insertPosY][m_insertPosX]) = cs;
			mp_screen[m_insertPosY][m_insertPosX]->sChar = mp_translator->translate( text[i] );

			//Now we need to increment our insertPosition.
			m_insertPosX++;
		}
	}
	catch (std::exception &e)
	{
		printf("Shit\n");

	}
}

////////////////////////////////////////////////////////////////////
void 
Terminal::insertBackspace(int x)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	for (int i = 0; i < x; i++)
	{
		//Move left, delete the char
		m_insertPosX--;
		if (m_insertPosX < 0)
			m_insertPosX = 0;
		else
		{
			ConsoleChar cc;
			(*mp_screen[m_insertPosY][m_insertPosX]) = cc;
		}
	}

	//Starting from our new insertion point, shift any non-Null character left
	//TODO: Theres a possible optimization here. We currently shift entire line...
	// which isn't needed if theres a bunch of trailing null space
	for (int i = m_insertPosX; i < m_screenWidth -2; i++)
	{
		(*mp_screen[m_insertPosY][i]) = (*mp_screen[m_insertPosY][i+1]);
	}

	//Set the last character on the line tso just a blank with default values
	(*mp_screen[m_insertPosY][m_screenWidth-1]) = ConsoleChar();
}

////////////////////////////////////////////////////////////////////
void
Terminal::deleteCharacters(int n)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	for(int i = 0 ; m_insertPosX+i< m_screenWidth; i++)
	{
		if (m_insertPosX+i+n >= m_screenWidth-1)
			mp_screen[m_insertPosY][m_insertPosX+i]->sChar = CHAR_NULL;
		else
			mp_screen[m_insertPosY][m_insertPosX+i]->sChar = mp_screen[m_insertPosY][m_insertPosX+i+n]->sChar;

		mp_screen[m_insertPosY][m_insertPosX+i]->colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;
	}
}

////////////////////////////////////////////////////////////////////
void
Terminal::insertNewLine(int x)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	//Will the insertion of a newline cause us to scroll?
	if ( m_scrollRegionEnd < m_insertPosY + x)
		scroll(MOVE_UP);
	else
		m_insertPosY += x;

	if (m_insertPosY >= m_screenHeight)
		m_insertPosY = m_screenHeight -1;

	//Now we need to go to the beginning of this new line
//	m_insertPosX = 0;
}

////////////////////////////////////////////////////////////////////
/* When we insert a blank line, we will be moving everything on the current
 * line and the lines below it down. This is similar to scrolling, except we
 * do not move all lines */
void
Terminal::insertBlankLines(int n)
{
	//OPTIMIZE ME -> like in scroll()
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	//Delete the lines that get shifted off by the blank line insertion
	for (int y = m_scrollRegionEnd; y > m_scrollRegionEnd-n; y--)
	{
		for (int x = 0; x < m_screenWidth; x++)
		{
			delete mp_screen[y][x];
			mp_screen[y][x] = NULL;
		}

		delete [] mp_screen[y];
	}

	//Now shift the lines needed
	for (int i = m_scrollRegionEnd; i >= (m_insertPosY + n) ; i--)
		mp_screen[i] = mp_screen[i-n];

	//Create new lines
	for (int y = m_insertPosY; y < (m_insertPosY + n); y++)
	{
		//Make new row
		mp_screen[y] = new ConsoleChar*[m_screenWidth];
		for (int x = 0; x < m_screenWidth; x++)
		{
			//Make each new column item
			mp_screen[y][x] = new ConsoleChar();
		}
	}
}


////////////////////////////////////////////////////////////////////
void
Terminal::getStartCoordsOfNullSpace( int &x, int &y, const int n)
{
	//First walk back to beginning to the end of NULL space
	for (y = m_screenHeight-1; y >= 0; y--)
	{
		for (x = m_screenWidth-1; x >= 0; x--)
		{
			if ( mp_screen[y][x]->sChar != CHAR_NULL)
			{
				

				return;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
/*** Partially OPTIMIZED *****/
void
Terminal::insertSpaces(int n)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	if ( m_bAutoWrapping)
	{
		//Delete n items starting from the bottom right of our screen array
		int x, y, ctr = 0;
		//getStartCoordsOfNullSpace( x, y, n);
		x = m_screenWidth-1;
		y = m_screenHeight-1;
		while ( ctr < n)
		{
			delete mp_screen[y][x];
			mp_screen[y][x] = NULL;
			x--;
			if ( x == -1)
			{
				x = m_screenWidth-1;
				y--;
			}
			ctr++;
		}

		if ( ! m_screenWidth)	//PT note: I think this is where one of our crazy crash bugs came from...
			return;

		//Now shift all elements to make room for n spaces... crazy setup time
		int
			stopX = m_insertPosX,
			stopY = m_insertPosY,
			shiftX = (m_screenWidth-1) - (n % m_screenWidth),		//char column being shifted from. Ex: [y][x] = [shiftX][shiftY]
			shiftY = (m_screenHeight-1) - (n / m_screenWidth); //char row being shifted from.

		stopX--;
		if (stopX == -1)
		{
			stopX = m_screenWidth-1;
			stopY--;
		}
		
		x = m_screenWidth-1,
		y = m_screenHeight-1;
		while ( ! (shiftX == stopX && shiftY == stopY) )
		{
			mp_screen[y][x] = mp_screen[shiftY][shiftX];
			mp_screen[y][x]->colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;
			mp_screen[shiftY][shiftX] = NULL;

			//Move our indexes to next spots
			shiftX--;
			if ( shiftX == -1)
			{
				shiftX = m_screenWidth-1;
				shiftY--;
			}
		
			x--;
			if ( x == -1)
			{
				x = m_screenWidth-1;
				y--;
			}
		}

		//Create the "space "items
		x = m_insertPosX;
		y = m_insertPosY;
		for (ctr = 0; ctr < n; ctr++)
		{
			mp_screen[y][x] = new ConsoleChar(CHAR_NULL);
			x++;
			if ( x == m_screenWidth)
			{
				x = 0;
				y++;
			}
		}
	}
	else
	{
		ConsoleChar *pCC;
		for (int x = m_screenWidth-1; (x-n) >= m_insertPosX; x--)
		{
			pCC = mp_screen[m_insertPosY][x];
			mp_screen[m_insertPosY][x] = mp_screen[m_insertPosY][x-n];
			mp_screen[m_insertPosY][x-n] = pCC;
		}

		//Assumption, the space item will get over-written
	}
}

////////////////////////////////////////////////////////////////////
/* This will erase the characters starting from our insert position
 * going to the left and up*/
void
Terminal::eraseCharacters(int n, ConsoleChar &cs)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	for (int i = 0; i < n; i++)
	{
		// erase (re-paint) the current character and move to the next one
		(*mp_screen[m_insertPosY][m_insertPosX]) = cs;
		mp_screen[m_insertPosY][m_insertPosX]->sChar = CHAR_NULL;

		m_insertPosX++;
		if (m_insertPosX == m_screenWidth)		//Went off right side of screen
		{
			m_insertPosX = 0;
			m_insertPosY++;

			if (m_insertPosY == m_screenHeight)		//Sanity check
				m_insertPosY = m_screenHeight-1;
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Terminal::eraseRequest(EraseType eraseType, ConsoleChar cs)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	int origPosX = m_insertPosX,
		origPosY = m_insertPosY;
	
	switch(eraseType)
	{
	case ERASE_TO_END_OF_LINE:
		while (m_insertPosX < m_screenWidth)
		{
			(*mp_screen[m_insertPosY][m_insertPosX]) = cs;
			m_insertPosX++;
		}
		m_insertPosX = origPosX;
		break;
	case ERASE_TO_BEGINNING_OF_LINE:
		while (m_insertPosX > 0)
		{
			m_insertPosX--;
			(*mp_screen[m_insertPosY][m_insertPosX]) = cs;
		}
		m_insertPosX = origPosX;
		break;
	case ERASE_ENTIRE_LINE_NO_MOVE_CURSOR:
		for (int i = 0; i < m_screenWidth; i++)
			(*mp_screen[m_insertPosY][i]) = cs;
		
		break;
	case ERASE_TO_END_OF_SCREEN:
		//Erase rest of current line
		for (int x = m_insertPosX; x < m_screenWidth; x++)
			(*mp_screen[m_insertPosY][x]) = cs;
		
		m_insertPosY++;  //Move to next row down
		//Erase the rest of the display
		for (int y = m_insertPosY; y < m_screenHeight; y++)
			for (int x = 0; x < m_screenWidth; x++)
				(*mp_screen[y][x]) = cs;
				
		m_insertPosY = origPosY;
		m_insertPosX = origPosX;
		break;
	case ERASE_TO_BEGINNING_OF_SCREEN:
		//Erase rest of current line from cursor to left side
		for (int x = m_insertPosX; x >= 0; x--)
			(*mp_screen[m_insertPosY][x]) = cs;

		m_insertPosY--;
		if ( m_insertPosY < 0)
			break;	//No more to erase

		//Erase the above lines
		for (int y = m_insertPosY; y >= 0; y--)
			for (int x = 0; x < m_screenWidth; x++)
				(*mp_screen[y][x]) = cs;

		m_insertPosX = m_insertPosY = 0;
		break;

	case ERASE_ENTIRE_SCREEN_NO_MOVE_CURSOR:
		for (int y = 0; y < m_screenHeight; y++)
			for (int x = 0; x < m_screenWidth; x++)
				(*mp_screen[y][x]) = cs;

		break;
	default:
		printf("Graphical console doesn't yet handle erase request: %d\n", eraseType);
	}
}

////////////////////////////////////////////////////////////////////
void
Terminal::setScrollingRegion(int startLine, int endLine)
{
	//Locking not needed

	//Sets the scroll region values normalized to a starting index of 0
	m_scrollRegionStart = startLine-1;
	m_scrollRegionEnd = endLine-1;
}

////////////////////////////////////////////////////////////////////
/*NOTE: A user can specify a value of < 0 if they do not wish to change that value.
 * Ex:
 * line = -1, column = 5
 * means we do not change the line, only column is effected  */
void
Terminal::setCursorPosition(int line, int column)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

   //normalize values to start at zero
   if (line > 0)
      line--;
   if (column > 0)
      column--;

   if (line >= 0)
	{
		if ( line >= m_screenHeight)
			m_insertPosY = m_screenHeight-1;
		else
			m_insertPosY = line;
	}

	if (column >= 0)
	{
		if ( line >= m_screenWidth)
			m_insertPosX = m_screenWidth-1;
		else
			m_insertPosX = column;
	}
}

////////////////////////////////////////////////////////////////////
/* <CR>            0015
 * Move the cursor to the left margin of the current line. */
void
Terminal::insertCarriageReturn()
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;
   m_insertPosX = 0;
}

////////////////////////////////////////////////////////////////////
void
Terminal::moveCursor(MoveDirection direction, int n)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

   switch( direction)
   {
   case MOVE_UP:
      m_insertPosY -= n;
      if (m_insertPosY < 0)				//Make sure we didn't go out of bounds
         m_insertPosY = 0;
      break;
   case MOVE_DOWN:
      m_insertPosY += n;
	  if (m_insertPosY >= m_screenHeight )	//Make sure we didn't go out of bounds
         m_insertPosY = m_screenHeight-1;
      break;
   case MOVE_LEFT:
      m_insertPosX -= n;
      if (m_insertPosX < 0)
         m_insertPosX = 0;
      break;
   case MOVE_RIGHT:
      m_insertPosX += n;
	  if (m_insertPosX >= m_screenWidth )
         m_insertPosX = m_screenWidth-1;
      break;
   }
}

////////////////////////////////////////////////////////////////////
void
Terminal::setCursorVisible(bool visible)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;
	m_bIsCursorVisible = visible;
}

////////////////////////////////////////////////////////////////////
void
Terminal::saveCursor()
{
	//Locking not needed

	m_savedCursorPosX = m_insertPosX;
	m_savedCursorPosY = m_insertPosY;
}

////////////////////////////////////////////////////////////////////
void
Terminal::restoreCursor()
{
	//Locking not needed

	m_bConsoleIsDirty = true;
	m_insertPosX = m_savedCursorPosX;
	m_insertPosY = m_savedCursorPosY;
}

////////////////////////////////////////////////////////////////////
/* This function gets used a LOT! So to make it as quick as possible, we
 * never delete rows/items and don't call new. We will only swap pointers.
 * Ex: When scrolling up, the top lines go off the screen. Instead of deleting
 *    them, put them at the bottom of the screen. Then set the lines we placed
 *    at the bottom of the screen to their default values
 *** OPTIMIZED *****/
void
Terminal::scroll(MoveDirection direction, int linesToScroll)
{
	QMutexLocker lock( mp_mutex);
	m_bConsoleIsDirty = true;

	//First, lets mark all CC's that will move as dirty
	for (int y = m_scrollRegionStart; y <= m_scrollRegionEnd; y++)
		for (int x = 0; x < m_screenWidth; x++)
			mp_screen[y][x]->colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;


	ConsoleChar ***pRows = new ConsoleChar **[linesToScroll];
	for (int i = 0; i < linesToScroll;i++)
		pRows[i] = NULL;

	if (direction == MOVE_UP)
	{
		//Step 1: take care of the lines that get shifted off the top
		if ( ! m_bIsGraphicalTerm && m_scrollRegionStart == 0)
		{
			//Move the shifted off lines into our history list
			for (int i = m_scrollRegionStart; i < (linesToScroll + m_scrollRegionStart); i++)
			{
				m_history.append( mp_screen[i] );
				mp_screen[i] = NULL;

				if (m_history.size() > m_maxHistorySize)
				{
					for (int x = 0; x < m_historyWidth; x++)
					{
						delete m_history[0][x];
						m_history[0][x] = NULL;
					}

					delete [] m_history[0];
					m_history.removeFirst();
				}
			}
		}
		else
		{
			//Move the number of top lines to our array so we can re-use them
			for (int i = m_scrollRegionStart; i < (linesToScroll + m_scrollRegionStart); i++)
			{
				pRows[i-m_scrollRegionStart] = mp_screen[i];
				mp_screen[i] = NULL;
			}
		}

		//Step 2: scrolling the rows up
		for (int i = m_scrollRegionStart; i + linesToScroll <= m_scrollRegionEnd; i++)
			mp_screen[i] = mp_screen[i + linesToScroll];

		//Step 3: Place the "saved" rows on the end of the screen, or make new lines to fill the end
		int offset = m_scrollRegionEnd- (linesToScroll - 1);
		for (int y = offset; y <= m_scrollRegionEnd; y++)
		{
			if ( ! m_bIsGraphicalTerm && m_scrollRegionStart == 0)
			{
				mp_screen[y] = new ConsoleChar *[m_screenWidth];
				for (int x = 0; x < m_screenWidth; x++)
					mp_screen[y][x] = new ConsoleChar();
			}
			else
			{
				mp_screen[y] = pRows[y - offset];
				pRows[y - offset] = NULL;

				//Reset each item to defaults
				for (int x = 0; x < m_screenWidth; x++)
					(*mp_screen[y][x]) = ConsoleChar();
			}
		}
	}
	else //direction == MOVE_DOWN
	{
		//Move number of bottom lines to our array
		int offset = m_scrollRegionEnd-linesToScroll + 1;
		for (int i = offset; i <= m_scrollRegionEnd; i++)
		{
			pRows[i-offset] = mp_screen[i];
			mp_screen[i] = NULL;
		}

		//Now scroll the rows down
		for (int i = m_scrollRegionEnd; (i - linesToScroll) >= m_scrollRegionStart; i--)
			mp_screen[i] = mp_screen[i - linesToScroll];

		//Place the "saved" rows on the top of the screen
		offset = m_scrollRegionStart;
		for (int y = offset; y - offset < linesToScroll; y++)
		{
			mp_screen[y] = pRows[y - offset];
			pRows[y - offset] = NULL;

			//Reset each item to defaults
			for (int x = 0; x < m_screenWidth; x++)
				(*mp_screen[y][x]) = ConsoleChar();
		}
	}

	delete [] pRows; //delete our temp pointer array
}

////////////////////////////////////////////////////////////////////
ConsoleChar***
Terminal::createNewScreenPointerArray(int cols, int rows)
{
	ConsoleChar ***pScreen = new ConsoleChar** [rows];
   for (int i = 0; i < rows; i++)
      pScreen[i] = new ConsoleChar *[cols];

	for (int y = 0; y < rows; y++)
		for (int x = 0; x < cols; x++)
			pScreen[y][x] = new ConsoleChar();

	return pScreen;
}

////////////////////////////////////////////////////////////////////
void
Terminal::deleteScreen(int cols, int rows)
{
	if ( mp_screen )
	{
		//Delete individual items
		for (int y = 0; y < rows; y++)
		{
			for (int x = 0; x < cols; x++)
			{
				delete mp_screen[y][x];
				mp_screen[y][x] = NULL;
			}
		}

		//Delete the rows
		for (int i = 0; i < rows; i++)
		{
			delete [] mp_screen[i];
			mp_screen[i] = NULL;
		}

		//Delete the columns
		delete [] mp_screen;
		mp_screen = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
///******** Static function **********
// * Provided for easy destruction outside of the terminal class */
//static void
//deleteScreenPointerArray(ConsoleChar ****pScreen, int cols, int rows)
//{
//	if ( *pScreen )
//	{
//		//Delete individual items
//		for (int y = 0; y < rows; y++)
//		{
//			for (int x = 0; x < cols; x++)
//			{
//				delete (*pScreen)[y][x];
//				(*pScreen)[y][x] = NULL;
//			}
//		}
//
//		//Delete the rows
//		for (int i = 0; i < rows; i++)
//		{
//			delete [] (*pScreen)[i];
//			(*pScreen)[i] = NULL;
//		}
//
//		//Delete the columns
//		delete [] (*pScreen);
//		(*pScreen) = NULL;
//	}
//}
