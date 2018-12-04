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

/**********************
 * Qt Includes
 *********************/
#include <QMutex>
#include <QChar>
#include <QHash>


/**********************
 * Includes
 *********************/



/**********************
 * #Defines
 *********************/
/* Our ColorEffect will essentially just be an unsigned char */
#define COLOR_EFFECT_DEFAULT			0	//Normal			0	Reset all attributes
#define COLOR_EFFECT_BRIGHT			1	//Bold				1	Bright
#define COLOR_EFFECT_DIM				2	//Dark colors		2	Dim
#define COLOR_EFFECT_UNDERLINE		4	//underscore		4	Underscore
#define COLOR_EFFECT_BLINK				8	//blinky blinky!	5	Blink
#define COLOR_EFFECT_REVERSE			16	//backwards			7	Reverse
#define COLOR_EFFECT_CHAR_IS_DIRTY	32	//If true, this CC has changed since last render

//These are provided for convenience. AND them to remove each bit
#define COLOR_EFFECT_REMOVE_BRIGHT			62
#define COLOR_EFFECT_REMOVE_DIM				61
#define COLOR_EFFECT_REMOVE_UNDERLINE		59
#define COLOR_EFFECT_REMOVE_BLINK			55
#define COLOR_EFFECT_REMOVE_REVERSE			47
#define COLOR_EFFECT_REMOVE_CHAR_IS_DIRTY	31

/* A console character is an item that will represent a character on the
 * console. It holds the character to display along with a colorSettings
 * item that tells us how to draw the character in terms of color, bold,
 * normal, underlined, reverse, dim, etc. */
class ConsoleChar
{
public:

	ConsoleChar( QChar qChar = NULL);


	QChar
		sChar;         //Screen character

	unsigned char
		colorEffect;	//Each bit tells us to bold, darken, underline.... this text
	
	//Color settings stuff
	int
		BGColor,		//Background color
		FGColor;		//Foreground (text) color
	
	bool
		blink;

	/************ Memory Optimization*************
	* Since our app will create and destroy many of these ConsoleChar (CC) instances
	* this cause an excessive amount of calls to new/delete. To quicken construction
	* and desctruction, we will re-use CC items by use of a static list
	* which holds "deleted" items. Anytime a new CC object is needed, we see if we
	* can grab one from our list. If not, we new' another CC object. Anytime we delete
	* a CC object, we just add it on the beginning of our list */
#ifdef CC_MEM_OPTIMIZE
	/**********************
	* Overloaded Functions
	**********************/
	/** THREAD SAFE
	 * This will see if we have any previous nodes that we can re-use. If not, we just
	 * "new" another one. */
	static void* operator new(unsigned int size);

	/** THREAD SAFE
	 * Adds the deleted node onto the list to be later recycled. */
	static void operator delete(void*);

	/* Meant to be called when the application is getting ready to close. All nodes
	 * that are contained in the recycle list are finally deleted */
	static void cleanupFreeList();


	/**********************
	* Overloaded Variables
	**********************/
	ConsoleChar
		*mp_next;
	
	static ConsoleChar
		*mp_freeList;		/* Holds a list of pointers to deleted CC objects. These will be reused
								 * Whenever the user calls "new" for a CC object. We will just give them
								 * an object from our list */
	static QMutex
		*mp_staticLock;
#endif
};

inline bool operator==(const ConsoleChar &CC1, const ConsoleChar &CC2)
{
	return (CC1.BGColor == CC2.BGColor && CC1.colorEffect == CC2.colorEffect && CC1.FGColor == CC2.FGColor && CC1.sChar == CC2.sChar);
}

inline uint qHash(const ConsoleChar &key)
{
	uint temp = key.BGColor;
	temp <<= 5;
	temp |= key.FGColor;
	temp <<= 8;
	temp |= key.colorEffect;
	temp <<= 8;
	temp |= key.sChar.toAscii();
	return temp;
}


/* When our SSHWidget wants to do a rendering, it gets a RenderInfo object and
 * sends it to our renderWidget so it can draw the screen. */
struct RenderInfo{
	ConsoleChar
		***pScreen;			/* Pointer to a 2D array of CC pointers that represents
								 * our console screen AND its history. The beginning of the
								 * screen is history and the bottom <visibleTermHeight> lines
								 * is the actual console screen. Its up to the display widget
								 * to correctly show the proper viewport of the pScreen based
								 * upon the scrollbar of the Render widget */
	int
		termWidth,			//Size of the terminal width
		termHeight,			//size of the terminal height + history
		visibleTermHeight,//Start of the viewable part of the terminal
		cursorPosX,			//cursor column location
		cursorPosY;			//cursor row location

	RenderInfo(){
		pScreen = NULL;
		termWidth = 0;
		termHeight = 0;
		visibleTermHeight = 0;
		cursorPosX = 0;
		cursorPosY = 0;
	}
	
	~RenderInfo(){
		if (pScreen)
		{
			//Terminal::deleteScreenPointerArray( &pScreen, termWidth, termHeight);
			//Delete individual items
			for (int y = 0; y < termHeight; y++)
			{
				for (int x = 0; x < termWidth; x++)
				{
					delete pScreen[y][x];
					pScreen[y][x] = NULL;
				}
			}
	
			//Delete the rows
			for (int i = 0; i < termHeight; i++)
			{
				delete [] pScreen[i];
				pScreen[i] = NULL;
			}
	
			//Delete the columns
			delete [] pScreen;
			pScreen = NULL;
		}
	}
};
