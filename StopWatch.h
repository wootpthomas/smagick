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

#pragma once

/* The purpose of this class is to provide a simple, yet effective
 * way to time the execution of code and other items. Its basic
 * functionallity is meant to mimic a Stopwatch. */

#include <ctime>
#include <stdio.h>

class StopWatch
{
public:

	//Default constructors
	StopWatch(void);
	StopWatch(bool autoPrint);

	//destructor
	~StopWatch(void);

	// Starts the timing of the StopWatch
	void start(void);

	//Ends the timing of the StopWatch
	void stop(void);

	//Prints the amount of time for the last timing
	void print(void);

	//Allows us to change the autoPrint setting on the fly
	void autoPrint(bool set);

private:
	// Holds the time when we began last our StopWatch
	//clock_t begin_time;
	float begin_time;

	// Holds the time when we last stopped our StopWatch
	//clock_t stop_time;
	float stop_time;

	// Holds the result of the last timing session
	float time_passed;

	// Flag to see if we should print the time_passed when calling stop
	bool printFlag;
};
