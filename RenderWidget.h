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
#include <QWidget>


/***********************
 Includes
***********************/
#include "DataTypes.h"
#include "ConsoleChar.h"


class RenderWidget :
	public QWidget
{
public:
	RenderWidget(QWidget *parent);
	~RenderWidget(void);

	/* This is expected to be implemented in the inheriting class.
	 * Gets called when the GUI wants this widget to draw the screen.*/
	virtual void renderScreen(RenderInfo* ) =0;

	/* When the user changes tabs, we need to make sure that the display
	 * widget has focus. For most widgets, we just need to have them call
	 * setFocus() on the widget that the users sees. */
	virtual void setActive() =0;

protected:
	void keyPressEvent(QKeyEvent *pEvent);

	RenderInfo
		*mp_screen;
};
