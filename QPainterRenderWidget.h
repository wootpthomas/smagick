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



#include <QScrollBar>
#include <QPainter>
#include <QEvent>
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QBrush>


#include "RenderWidget.h"
#include "UserPreferences.h"


/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


class QPainterRenderWidget :
	public RenderWidget {

	Q_OBJECT
public:
	QPainterRenderWidget(QWidget *parent, 	UserPreferences * const pUserPrefs);
	~QPainterRenderWidget();

	void renderScreen(RenderInfo* );

	QString getSelection();

	void setActive() { setFocus(); }

protected:
	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);

	/* We over-ride the scroll wheel event so that we can tell our render
	 * widget to scroll the view accordingly */
	void wheelEvent ( QWheelEvent * e );

public slots:
	/* When the user moves the scrollbar slider, this essentially just
	 * causes us do do a repaint event. Our paint event is smart enough
	 * to paint the appropriate area based on the scrollbar's slider
	 * position */
	void slotScrollBarSliderMoved(int x);

private:

	void calculateCords();

	//Adjusts the scrollbar size according to the new screen array stuff
	void adjustScrollbar();

	//QTimer *blinkTimer;

	QScrollBar
		*mp_scrollBar;

	UserPreferences
		* const mp_userPrefs; //Thread-safe user preferences object
	
	int
		*rowLookupTable,
		*colLookupTable,
		m_fontHeight,
		m_fontWidth,
		selectionPoint1x,
		selectionPoint1y,
		selectionPoint2x,
		selectionPoint2y;

	bool
		amDraging,
		m_bDontUseDirtyBits;	/*If set, the user just moved the scrollbar to the bottom, so
									 * we need to render all CCs */

	QPixmap
		*mp_screenPixmap;		/* Pointer to the pixmap we render onto. We draw this onto the widget
									 * and when we are ready to display to the user, we then draw this pixmap
									 * onto our widget */
};
