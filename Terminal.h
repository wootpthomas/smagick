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
#include <QChar>
#include <QMutex>
#include <cstring>


/***********************
 Includes
*************************/
#include "CharacterSet.h"
#include "ConsoleChar.h"
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
 **********************/
class Terminal
{
public:
	Terminal(int termWidth, int termHeight, bool isGraphicalTerm = false);
	~Terminal(void);

	/** THREAD SAFE
	 * Returns a RenderInfo objet so that our renderer has all the info it needs
	 * to draw the screen */
	RenderInfo* getRenderInfo();

	/******* ONLY TO BE CALLED FROM DECODER OBJECT!! ***********
	 * Called when the decoder switches from/to graphical mode which changes
	 * consoles. We force the m_bConsoleIsDirty flag to be true so that the next
	 * time the current SSHWidget asks for renderInfo, we give it a valid object
	 * even if the console really isn't dirty. Needed for console type switch */
	void setConsoleDirty();

	/* Sets the maximum number of lines that we keep track of for history.
	 * If this is not set, we use the default size of 75 rows. */
	void setMaxHistorySize(int rows);

	/** THREAD SAFE
	 * Enable/disable autotextwrapping */
	void setAutoWrapToNewLine(bool value);

	/** THREAD SAFE
	 * Changes the terminal screen size */
	void setScreenSize(int cols, int rows);

   /** THREAD SAFE
	 * Inserts text into our screen with the specified colorSettings */
	void insertText( const ConsoleChar &cs, const std::string &text);

	/** THREAD SAFE
	 * Inserts x number of backspaces */
	void insertBackspace(int n = 1);

	/** THREAD SAFE
	 * Deletes the specified number of characters */
	void deleteCharacters(int n = 1);

	/** THREAD SAFE
	 * Inserts x number of newlines */
	void insertNewLine(int n = 1);

	/** THREAD SAFE
	 * inserts the given number of blank lines into our console */
	void insertBlankLines(int n);

	/** THREAD SAFE
	 * Inserts the given number of spaces */
	void insertSpaces(int n);

	/** THREAD SAFE
	 * erases the given number of characters */
	void eraseCharacters(int n, ConsoleChar &cs);

	/** THREAD SAFE
	 * This covers any and all erasing. Erases on a line or
	 * on the entire screen */
	void eraseRequest(EraseType, ConsoleChar);

	/** THREAD SAFE
	 * Sets up a scrolling region */
	void setScrollingRegion(int startLine, int endLine);

   /** THREAD SAFE
	 * Moves the cursor position */
   void setCursorPosition(int line, int column);

	/** THREAD SAFE
	 * Moves the cursor to the very left edge of the screen */
   void insertCarriageReturn();

   /** THREAD SAFE
	 * Moves the cursor in the specified direction N times */
   void moveCursor(MoveDirection direction, int n = 1);

	/** THREAD SAFE
	 * Show/hide the cursor */
	void setCursorVisible(bool value);

	/** THREAD SAFE
	 * Saves the cursor position for future use */
	void saveCursor();

	/** THREAD SAFE
	 * Restores a saved cursor position */
	void restoreCursor();

	/** THREAD SAFE
	 * Scrolls the view up/down by the given number of lines */
	void scroll(MoveDirection direction, int linesToScroll = 1);

	/** THREAD SAFE
	 * Changes the drawing character set */
	void changeCharSet(CharSet charSet) { mp_translator->changeCharSet(charSet); }

	///* Deletes a ConsoleChar*** pointer array. We'll make this static so that
	// * the reciever of any screenPointerArray can easily delete their stuff
	// * and not be too blown away. */
	//static void deleteScreenPointerArray(ConsoleChar ****pScreen, int cols, int rows);


private:

	/**********************
    Private Functions
   **********************/
	/* Returns a pointer to our pointer array for a new screen */
	ConsoleChar*** createNewScreenPointerArray(int cols, int rows);

	/* Deletes the screen */
	void deleteScreen(int cols, int rows);

	/* Used inside insertSpaces() to help optimize CC shifts */
	void getStartCoordsOfNullSpace( int &x, int &y, const int n = 0);

	/* Used to set the entire console screen's CC dirty or clean. We normally
	 * set all CCs to clean after sending a renderinfo object out. Quite useful
	 * for when we switch from graphical to text mode and we need to make sure that
	 * the render widget repaints all characters. */
	void setEntireConsoleDirty(bool isDirty);

	/**********************
    Private Variables
   **********************/
	QString
		m_defaultTextColor,			//Specifies the default color name for text
		m_defaultTextBGColor;			//Specifies the default color name for text background

   ConsoleChar
      ***mp_screen;		/* Points to (the array) a list of pointers, and each pointer in the
							    * list (row) points to a list of pointers (columns). */
	QList<ConsoleChar **>
		m_history;			/* If this is the non-graphical terminal (see m_bIsGraphicalTerm) then
								 * anytime we have a line that goes off the top of the screen, we push
								 * it onto our history list. Then when we get in a renderInfo request
								 * we will build a screen with the history + our current screen and send
								 * that over to our renderWidget.
								 * This history list holds a pointer to rows of CCs. The rows are
								 * m_historyWidth wide. */

	QMutex
		*mp_mutex;			/* This mutex is setup to be recursive, because this class has methods
								 * which will do a mutex lock and then call another public method which
								 * will try and re-lock the mutex.
								 * This mutex is really only used to protect reading/writing to/from the
								 * mp_screen array. Functions that do not modify mp_screen do not need
								 * to lock. */

   int
      m_screenWidth,
      m_screenHeight,
		m_historyWidth,		//Specifies how long each row is in our history list
		m_maxHistorySize,		//Specifies how many lines of history we will keep
      m_insertPosX,			//Insertion point for new text (column)
      m_insertPosY,			//Insertion point for new text (row)
		m_scrollRegionStart,	//Start of the scrolling region
		m_scrollRegionEnd,	//End of the scrolling region
		m_savedCursorPosX,	//Stores the last saved X position of our cursor
		m_savedCursorPosY;	//Stores the last saved Y position of our cursor

	CharacterSet
		*mp_translator;	//Holds the type of character set we are drawing

	bool
		m_bConsoleIsDirty,	//Flag to tell if we do need to do a rendering
		m_bIsCursorVisible,
		m_bAutoWrapping,		//Flag that lets us know when inserted text should auto-wrap
		m_bIsGraphicalTerm;	/* If set to true then we don't need to keep any sort of terminal
									 * history. If false, then we keep terminal history and when a 
									 * renderInfo object is requested, we will send over a screen array
									 * that contains history appended with the current screen */
};
