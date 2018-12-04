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
#include <QThread>
#include <QQueue>
#include <QMutex>



/***********************
 Forward Class Declarations
***********************/
class Decoder;
class Terminal;
class QSemaphore;


/***********************
 Includes
***********************/
//#include "DataTypes.h"
#include "ConsoleChar.h"



/* the Decoder Thread holds both the
 *   Decoder
 *   Terminal
 */
class DecoderThread :
	public QThread
{
public:
	DecoderThread(int termWidth, int termHeight);
	~DecoderThread(void);

	/* Returns a pointer to our decoder specifically for signal/slot connections */
	const QObject* getDecoderPtr() { return (QObject*)mp_decoder; }

	/* Gets our decoder all ready for work */
	void run();

	/** THREAD SAFE
	 * Adds on an item to be decoded in our queue */
	void addToQueue(char *);

	/** THREAD SAFE
	 * Changes the size of the screen */
	void adjustScreenSize(int columns, int rows);

	/** THREAD SAFE
	 * Shuts down this thread */
	void quit();

	/** THREAD SAFE
	 * Toggles debugging on and off */
	void toggleDebugPrinting(bool value);

	/** THREAD SAFE
	 * Blocks and only returns true when this thread is ready to work */
	void isReadyToWork();

	/** THREAD SAFE
	 * Gets a RenderInfo object from the Decoder. Cause the Decoder knows which terminal
	 * is the active one. */
	RenderInfo* getRenderInfo();

	/** THREAD SAFE
	 * Reports back the status of app mode for the keypad keys. */
	bool areKeypadKeysInAppMode();
	
	/** THREAD SAFE
	 * Reports back the status of app mode for the cursor keys. */
	bool areCursorKeysInAppMode();

private:
	Decoder
		*mp_decoder;	//Decodes data from the SSHConnection

	Terminal				//Holds the data representing the terminal display
		*mp_mainTerm,	//Text terminal
		*mp_altTerm;	//Alternate terminal

	QQueue<char*>
		m_queue;			//Holds the list of char* to work on

	QMutex
		m_mutex,			//Locker to read/write other data
		m_queueMutex;	//Locker to read/write from queue

	QSemaphore
		*mp_semaWhore;	//

	int
		m_termCols,
		m_termRows;

	bool
		m_bKeepRunning,		//Helps us figure out when to exit
		m_bAdjustScreenSize;	//Set when we need to update our terminal sizes
};
