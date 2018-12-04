/***************************************************************************
 *   Copyright (C) 2008 by Paul Thomas and Jeremy Combs                    *
 *   thomaspu@gmail.com , combs.jeremy@gmail.com                           *
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


#include "StopWatch.h"

////////////////////////////////////////////////////////////////////////////////////
StopWatch::StopWatch(void)
{
	this->time_passed = 0.0;
	this->printFlag = true;
}

////////////////////////////////////////////////////////////////////////////////////
StopWatch::StopWatch(bool autoPrint)
{
	this->time_passed = 0.0;
	this->printFlag = autoPrint;
}

////////////////////////////////////////////////////////////////////////////////////
StopWatch::~StopWatch(void)
{
}

////////////////////////////////////////////////////////////////////////////////////
/* Gets the current system time and stores it as the
time when our StopWatch began timing */
void
StopWatch::start()
{
	this->begin_time = (float) clock();
}

////////////////////////////////////////////////////////////////////////////////////
/* If the StopWatch object is set to display the time
after a sucessful timing took place:
	This will take the current time and do a difference
	against the time when we began and will display the
	amount of time that has passed and records the amount.
Else
	It will not display the amount. */
void
StopWatch::stop()
{
	this->stop_time = (float) clock();
	float clocks_per_sec = (float)CLOCKS_PER_SEC;
	this->time_passed = (this->stop_time - this->begin_time)/clocks_per_sec;
  
	if (this->printFlag)
		print();
}

////////////////////////////////////////////////////////////////////////////////////
void
StopWatch::print()
{
	printf ("Took %.3f secs\n", this->time_passed);
}

////////////////////////////////////////////////////////////////////////////////////
void
StopWatch::autoPrint(bool set)
{
	this->printFlag = set;
}
