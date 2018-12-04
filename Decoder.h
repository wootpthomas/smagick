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
#include <QObject>
#include <QMetaType>
#include <QColor>
#include <QMutexLocker>

/***********************
 Other Includes
*************************/
#include <cstring>
#include "DataTypes.h"
#include "ConsoleChar.h"
#include "Terminal.h"

/***********************
 Class Declarations
 **********************/


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
class Decoder :
	public QObject
{
	Q_OBJECT 

public:
	Decoder(Terminal *pMainTerminal, Terminal *pAltTerminal);
	~Decoder(void);

	/* This decodes the incoming text from SSH and displays it appropriately */
	void decode(char *pData, bool deleteCharPtr = true);

	/** THREAD SAFE
	 * Sets the debugging output to print the data, then its Hex representation */
	void toggleDebugPrinting(bool enable);

	/** THREAD SAFE
	 * Returns a RenderInfo item. This should ONLY be called from the DecoderThread!!! */
	RenderInfo* getRenderInfo();

	/** THREAD SAFE
	 * Called from the DecoderThread when some parent object (like our SSHWidget)
	 * wants to know the current state of key modes so it can correctly translate
	 * keypad keypresses into the appropriate translation. */
	bool areKeypadKeysInAppMode();

	/** THREAD SAFE
	 * Called from the DecoderThread when some parent object (like our SSHWidget)
	 * wants to know the current state of key modes so it can correctly translate
	 * arrow keypresses into the appropriate translation. */
	bool areCursorKeysInAppMode();

signals:
	/* Gets emitted when the console has been closed by the user as a
	 * result of them typing in the "exit" command. */
	void exitConsole();

	/* Sent out whenever we recieve a bell character. */
	void playBell();

	/* Signal is emitted when our window title changes. This sometimes contains
	 * the current working directory of the console. */
	void windowTitleChanged(QString newWindowTitle);

private:
	/*********************
	Private Functions
	*********************/
	/* Changes Terminals when requested */
	void switchTerminals(bool useAltTerm = false);

	/* returns the position of the final character relative to mp_data */
	int getFinalCharPos();

	/* Tests for and fixes data with attributes that have been split */
	void fixCastratedData(char **pDecodeStart, char **pStop);

	/* Sets the mp_temp character array to the attribute string and will point the given character
	 * pointer to the final (terminating) character. If it does not exist then it returns false.
	 * Ex: on the string <ESC>[393m
	 * returns true and ppIter now points at 'm' */
	bool findFinalChar(char **ppIter, char finalCharToFind);

	/* Takes in a character pointer and a splitChar. Then it sees how many times the given
	 * string can be split by the <splitChar> and returns a char* array and its size. The
	 * array holds the split strings. */
	void splitStrings(char *pAttr, char splitChar, int &attrListSize);
	
	/* Parses out the 'Ps' from a standard CSI:
	 * <ESC>[Ps m   */
	int getIntAttrValue(const char* pAttr);

	/********************
	 Handler functions
	********************/
	/* Handles all XTerm escape sequences */
	void handleXTermEscapeSequence(char **ppIter);

	/* Handles all standard character attributes. These are in the form:
	 * <ESC>[<something><lowercase letter> */
	void handleStdAttr(char **ppData);

	void handleCharSetAttr(char** ppAttr);

	/* Handles all upper case character attributes. These are in the form:
	 * <ESC><Capitol letter> */
	//void handleUpperCaseLetterAttr(const QString &data, int &parseIndex);
	
	/**** End Handler functions ******/
	/* Decodes and handles the "Send device attributes" request */
	void decodeLowerC_attr(const char* attr);

	/* Decodes XTERM specific move cursor to line number */
	void decodeLowerD_attr(const char* attr);

   /* Decodes the lower case L attribute. I think the only one is telling us
    * to get out of cursorKeysInApp mode */
   void decodeLowerL_attr(const char* attr);

	/* Its purpose is to decode the character coloring attributes and then
	 * set the internal variables mFGColor, m_BGColor, m_colorEffect as neede
	 * so that the next time we tell the world to print text, we know what
	 * coloring attributes to print with */
	void decodeLowerM_attr(const char* attr);

	void decodeLowerH_attr(const char* attr);

   /* decodes the scroll region attribute */
   void decodeLowerR_attr(const char* attr);
   
   /* decodes the move cursor attribute */
   void decodeCapABCDEF_attr(const char*, MoveDirection);

	/* decodes the move to column position */
	void decodeCapG_attr(const char* attr);

   /* Decodes the cursor position attribute */
   void decodeCapH_attr(const char* attr);

	/* Decodes the erase in display/line attribute */
	void decodeCapJK_attr(const char* attr, bool isEraseInDisplay);

	/* Decodes the scroll up/down attribute */
	void decodeCapS_T_attr(const char finalChar, const QString &attr);

	/* Sets the correct color attribute member variables in THIS class */
	void setColorAttr(int colorNum);

	/* Helps us find the position of the given final character */
	int posOfFinalChar(const QString &data, int parseIndex, char finalChar);

	/* Called if we found a color attribute. The fore ground and background colors
	 * are updated with what they should now be for any newly printed text.
	 * Sets these class member vars: m_BGColor, m_FGColor*/
	void setColorValues(QStringList &matches);

	/* Gets the HTML string representation for the color value of the text.
	 * If no value is speficied, gets the default color.*/
	void setForegroundColor(int &color, int colorNum = 0);
	
	/* Gets the HTML string representation for the color value of the background text.
	 * If no value is speficied, gets the default color.*/
	void setBackgroundColor(int &color, int colorNum = 0);

	/* Mainly used for debugging work */
	void printInHex( const QString &data);

	/* Both of these are used only when printing out debugging info */
	void printDebugDisplaySpacing();

	void printToDebugDisplay(char *pData = NULL, bool newLine = true);

	/*********************
	Private Variables
	*********************/
	bool
		m_cursorKeysInAppMode,	//If true, we are in graphical mode and cursor keys operate in graphical mode
		m_keypadKeysInAppMode,	//If true, we are in graphical mode and keypad keys operate in graphical mode
		m_debugPrinting;			//Enables debugging output

	int
		m_dataSize;				//size of the mp_data character array

	char
		*mp_data,				//When decode() is called, this is the main item we operate on
		*mp_last_mp_data,
		*mp_temp,				//Used for findFinalChar() 
		*mp_tempCD,				//temp char array used by the fixCastratedData()
		**mp_tempAttrs;		/* Pointer to a list of character pointers that we'll use as temp variables. To cut
									 * down on memory allocations, we fill this array once on object creation */

	std::string
		m_castratedData,		//Sometimes an attribute will get lopped off and be included in the next data block
		m_peek,					//Shows 4 or so chars before the castrated part. DEBUGGING
		m_newT;					/*Holds our new text as we build it. Because we go into a few private
									 * functions, this is a member variable for convenience */

	/* These are set by the isValidColors function! */
	QString
		m_CWD;					//Holds what we believe is the current working directory

	ConsoleChar
		m_textFormat;			//Holds the current text formatting. We don't use the character here, just the coloring shit

	Terminal
		*mp_mainTerm,			//Main Terminal
		*mp_altTerm,			//Terminal to use for alternate screen buffer
		*mp_currentTerm;		//Pointer to current terminal

	QMutex
		m_mutex;
};
