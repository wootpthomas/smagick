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
***********************/
#include <QMutexLocker>
#include <QSemaphore>

/***********************
 Includes
***********************/
#include "DecoderThread.h"
#include "Decoder.h"
#include "Terminal.h"

////////////////////////////////////////////////////////////////////
DecoderThread::DecoderThread(int termWidth, int termHeight):
m_bKeepRunning(true),
m_bAdjustScreenSize(false),
m_termCols(termWidth),
m_termRows(termHeight),
mp_mainTerm(NULL),
mp_altTerm(NULL),
mp_decoder(NULL)
{
	mp_semaWhore = new QSemaphore(1);
	mp_semaWhore->acquire(1);		//Acquire a lock. We release it when we are ready to process shit
}

////////////////////////////////////////////////////////////////////
DecoderThread::~DecoderThread(void)
{
	delete mp_semaWhore;
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::run()
{
	mp_mainTerm = new Terminal( m_termCols, m_termRows);	//Text terminal
	mp_altTerm = new Terminal( m_termCols, m_termRows);	//Text terminal
	mp_decoder = new Decoder(mp_mainTerm, mp_altTerm);

	mp_semaWhore->release(1);			//Release the lock. Now readyToWork() is blocking
	printf("decoder thread IS ready!\n");


	char *pData = NULL;
	
	//Now that all objects are created and we are ready to decode and stuff
	//lets get our little event loop running
	while (1)
	{
		m_mutex.lock();
		if (! m_bKeepRunning)
		{
			m_mutex.unlock();
			break;
		}

		if ( m_bAdjustScreenSize)
		{
			mp_mainTerm->setScreenSize( m_termCols, m_termRows);
			mp_altTerm->setScreenSize( m_termCols, m_termRows);
			m_bAdjustScreenSize = false;
		}
		
		m_mutex.unlock();

		//Get a lock on the queue
		m_queueMutex.lock();
		if (m_queue.size() )
		{
			pData = m_queue.dequeue();
		}
		m_queueMutex.unlock();

		//Now lets check and see if we have data to decode
		if (pData)
		{
			mp_decoder->decode( pData);
			pData = NULL;
		}
		else
			this->usleep(500);	//Sleep for 1/2 a msec
	}

	//Cleanup
	delete mp_mainTerm;
	mp_mainTerm = NULL;

	delete mp_altTerm;
	mp_altTerm = NULL;

	delete mp_decoder;
	mp_decoder = NULL;

	//Delete any pending items
	while ( m_queue.size() )
	{
		pData = m_queue.dequeue();
		delete [] pData;
	}
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::addToQueue(char *pData)
{
	QMutexLocker locker( &m_queueMutex);
	m_queue.enqueue( pData);

#if defined(_DEBUG)
	if ( m_queue.size() > 10)
		printf("%d items in queue\n", m_queue.size() );
#endif
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::adjustScreenSize(int columns, int rows)
{
	QMutexLocker locker( &m_mutex);
	m_bAdjustScreenSize = true;
	m_termCols = columns;
	m_termRows = rows;
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::quit()
{
	QMutexLocker locker( &m_mutex);
	m_bKeepRunning = false;
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::toggleDebugPrinting(bool value)
{
	QMutexLocker locker( &m_mutex);
	mp_decoder->toggleDebugPrinting(value);
}

////////////////////////////////////////////////////////////////////
void
DecoderThread::isReadyToWork()
{
	mp_semaWhore->acquire(1);
}

////////////////////////////////////////////////////////////////////
RenderInfo* 
DecoderThread::getRenderInfo()
{
	QMutexLocker locker( &m_mutex);
	if (mp_decoder)
		return mp_decoder->getRenderInfo();
	else
		return NULL;
}

////////////////////////////////////////////////////////////////////
bool
DecoderThread::areKeypadKeysInAppMode()
{
	QMutexLocker locker( &m_mutex);
	if (mp_decoder)
		return mp_decoder->areKeypadKeysInAppMode();

	return false;
}
	
////////////////////////////////////////////////////////////////////
bool
DecoderThread::areCursorKeysInAppMode()
{
	QMutexLocker locker( &m_mutex);
	if (mp_decoder)
		return mp_decoder->areCursorKeysInAppMode();

	return false;
}
