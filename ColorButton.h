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
#include <QPushButton>
#include <QString>

/***********************
 Other Includes
*************************/



/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif



class ColorButton:
	public QPushButton
{
	Q_OBJECT

public:
	ColorButton(QWidget *pParent): QPushButton(pParent)
	{
		connect(this, SIGNAL(clicked()),
			this, SLOT(slotClicked()));
	}

	/* Sets the button color to the specified QColor */
	void setButtonColor(QColor &color)
	{
		QString bgColor = "background-color: %1;";
		bgColor = bgColor.arg( color.name() );
		setStyleSheet( bgColor);
		m_color = color;
	}

	//Returns the QColor of the button
	QColor & buttonColor() {	return m_color; }

signals:
	void getNewColor(ColorButton *);

private slots:
	void slotClicked() {	emit getNewColor(this); }

private:
	QColor m_color;
};