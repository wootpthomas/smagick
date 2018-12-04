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

/*******************
 Qt Includes
*******************/
#include <QScrollBar>
#include <QPaintEngine>

/*******************
 Other Includes
*******************/
#include "QPainterRenderWidget.h"

#ifdef _DEBUG
	#include "StopWatch.h"
#endif




#define SCROLLBARWIDTH 20

QPainterRenderWidget::QPainterRenderWidget(QWidget *parent, UserPreferences * const pUserPrefs) :
RenderWidget(parent),
mp_userPrefs(pUserPrefs),
rowLookupTable(0),
colLookupTable(0),
m_fontHeight(0),
m_fontWidth(0),
selectionPoint1x(0),
selectionPoint1y(0),
selectionPoint2x(0),
selectionPoint2y(0),
amDraging(false),
mp_screenPixmap(NULL),
m_bDontUseDirtyBits(false)
{
	this->setMouseTracking(true);

	mp_scrollBar = new QScrollBar(Qt::Vertical, this);
	connect( mp_scrollBar, SIGNAL(valueChanged(int) ),
		this, SLOT(slotScrollBarSliderMoved(int) ));
	connect( mp_scrollBar, SIGNAL(sliderMoved(int)),
		this, SLOT(slotScrollBarSliderMoved(int) ));
	mp_scrollBar->setMinimum( 0 );
	mp_scrollBar->setValue( mp_scrollBar->maximum() );

	//Set the widget background color to the same color in our userPreferences
	setAutoFillBackground(true);
	QPalette pal = this->palette();
	pal.setColor(QPalette::Window, mp_userPrefs->getColor(49, true) );
	setPalette( pal);
}

QPainterRenderWidget::~QPainterRenderWidget() {
	if (mp_screen){
		delete mp_screen;
		mp_screen = NULL;
	}

	if ( mp_screenPixmap)
	{
		delete mp_screenPixmap;
		mp_screenPixmap = NULL;
	}

	if (rowLookupTable)
	{
		delete [] rowLookupTable;
		rowLookupTable = NULL;	
	}

	if (colLookupTable)
	{
		delete [] colLookupTable;
		colLookupTable = NULL;	
	}
}

void QPainterRenderWidget::renderScreen(RenderInfo *newScreen) {

	if(!mp_screen) {
		mp_screen = newScreen;
		calculateCords();

		if ( ! mp_screenPixmap)
		{
			QFontMetrics fm(mp_userPrefs->font());
			mp_screenPixmap = new QPixmap(
				mp_screen->termWidth * m_fontWidth,
				mp_screen->termHeight * m_fontHeight);
		}
	}
	else
	{
		//Set our flag for if we need to recalculate our coordinates
		bool reCalculateCords = mp_screen->termHeight != newScreen->termHeight || mp_screen->termWidth != newScreen->termWidth;

		//Move to the new screen
		delete mp_screen;
		mp_screen = newScreen;

		if (reCalculateCords)
		{
			//reCalculate coordinates for the current screen
			calculateCords();

			QPixmap *pOldPM = mp_screenPixmap;
			mp_screenPixmap = new QPixmap(
				mp_screen->termWidth * m_fontWidth,
				mp_screen->termHeight * m_fontHeight );
			
			//Let's get our new copy ready. First, fill with default background color
			mp_screenPixmap->fill( mp_userPrefs->getColor(49, true) );

			//Now copy what we can from the old pixmap onto the new one
			QPainter painter( mp_screenPixmap);
			painter.drawPixmap(0, 0, *pOldPM);

			delete pOldPM;
		}
	}

	//New renderInfo clears any selections we had
	selectionPoint1x = selectionPoint1y = 0;
	selectionPoint2x = selectionPoint2y = 0;

	//Now let's resize the scrollbar stuff according to the screen
	adjustScrollbar();

	repaint();
}

void QPainterRenderWidget::adjustScrollbar()
{
	if ( mp_screen->termHeight == mp_screen->visibleTermHeight)
	{
		mp_scrollBar->setEnabled(false);
		mp_scrollBar->setMaximum( 0 );
		mp_scrollBar->setPageStep( 0 );
	}
	else
	{
		mp_scrollBar->setEnabled(true);
		mp_scrollBar->setMaximum( mp_screen->termHeight - mp_screen->visibleTermHeight);
		mp_scrollBar->setPageStep( mp_screen->termHeight / 10);
		mp_scrollBar->setSingleStep( 1);
		mp_scrollBar->setValue( mp_scrollBar->maximum() );
	}
}

void QPainterRenderWidget::slotScrollBarSliderMoved(int x)
{
	if ( mp_scrollBar->value() == mp_scrollBar->maximum())
		m_bDontUseDirtyBits = true;	//Set a flag to redraw entire screen

	repaint();
}

QString QPainterRenderWidget::getSelection() {
	QString currSelection;
	int startX = selectionPoint1x;
	int startY = selectionPoint1y;
	int stopX = selectionPoint2x;
	int stopY = selectionPoint2y;
	int tempX;
	int tempY;

	//nothing is selected return empty
	if( stopY == startY && stopX == startX )
		return "";

	if( stopY < startY ) {
		tempY = startY;
		startY = stopY;
		stopY = tempY;

		tempX = startX;
		startX = stopX;
		stopX = tempX;
	}
	if( stopY == startY && stopX < startX ) {
		tempY = startY;
		startY = stopY;
		stopY = tempY;

		tempX = startX;
		startX = stopX;
		stopX = tempX;
	}

	int offset = mp_screen->termHeight - (mp_screen->termHeight - mp_scrollBar->value());
	QString spaceString = "";
	for( int y = startY; y <= stopY; y++ ) {
		for( int x = ((y == startY)?startX:0); x <= ((y==stopY)?stopX:mp_screen->termWidth-1); x++ ) {
			if( mp_screen->pScreen[y+offset][x]->sChar == 0 ) {
				spaceString += " ";
			} else {
				//if we have spaces insert them and reset the space string
				if( !spaceString.isEmpty() ){
					currSelection += spaceString;
					spaceString.clear();
				}
				currSelection += mp_screen->pScreen[y+offset][x]->sChar;
			}
		}
		spaceString.clear();
		if( y != stopY )
			currSelection += "\n";
	}
	return currSelection;
}

void QPainterRenderWidget::calculateCords() {
	if(mp_screen) {
		QFontMetrics fm(mp_userPrefs->font());
		
		m_fontHeight = fm.height();
		m_fontWidth = fm.width(" ");
		//fontDescent = fm.descent();
		//fontAscent = fm.ascent();

		delete[] rowLookupTable;
		rowLookupTable = new int[mp_screen->termHeight];
		for(int i = 0; i < mp_screen->termHeight; i++ ) {
			rowLookupTable[i] = (i * m_fontHeight);
		}

		delete[] colLookupTable;
		colLookupTable = new int[mp_screen->termWidth];
		for(int i = 0; i < mp_screen->termWidth; i++) {
			colLookupTable[i] = ((i) * m_fontWidth);
		}
	}
}

void QPainterRenderWidget::paintEvent(QPaintEvent* paintEvent) {	

	
//#ifdef _DEBUG
//	StopWatch s(true);
//	printf("starting render...");
//	s.start();
//#endif	

	//do simple check of pointer safety
	if(mp_screen && rowLookupTable && colLookupTable) {
		QPainter pixmapPainter(mp_screenPixmap);

		int savedPMRenders = 0;
		int
			offset = 0,
			min = mp_scrollBar->minimum(),
			max = mp_scrollBar->maximum(),
			value = mp_scrollBar->value();

		offset = mp_screen->termHeight - (mp_screen->termHeight - mp_scrollBar->value());
		for( int y = 0 ; y < mp_screen->visibleTermHeight; y++ )
		{
			for( int x = 0 ; x < mp_screen->termWidth; x++ )
			{
				if ( m_bDontUseDirtyBits || mp_scrollBar->value() != mp_scrollBar->maximum() ||
					mp_screen->pScreen[y+offset][x]->colorEffect & COLOR_EFFECT_CHAR_IS_DIRTY)
				{
					pixmapPainter.drawPixmap(
						colLookupTable[x],
						rowLookupTable[y],
						mp_userPrefs->getPixmapForCharacter(mp_screen->pScreen[y+offset][x]));
				}
				else
					savedPMRenders++;
			}
		}
		m_bDontUseDirtyBits = false;

		int totalPM = mp_screen->visibleTermHeight * mp_screen->termWidth;
		printf("Rendered %d of %d pixmaps\n", totalPM-savedPMRenders, totalPM);

		//Now that our offscreen pixmap has the data, draw it onto the widget
		QPainter painter(this);
		int height = mp_screen->termHeight * m_fontHeight;
		int width = mp_screen->termWidth * m_fontWidth;
		painter.drawPixmap(0, 0, *mp_screenPixmap);

		//draw Cursor
		QColor cursorColor = mp_userPrefs->cursorColor();
		cursorColor.setAlpha(150);
		painter.fillRect(
			colLookupTable[mp_screen->cursorPosX],
			rowLookupTable[mp_screen->cursorPosY - offset],
			m_fontWidth,
			m_fontHeight,
			QBrush(cursorColor));
		
		if(selectionPoint1x != selectionPoint2x || selectionPoint1y != selectionPoint2y) {
			//set color to highlight with
			cursorColor.setRgb(0,0,255,150);
			
			if(selectionPoint1y < selectionPoint2y) {
				//selection Point 1 is above selection Point 2      This will be easy 
				//draw the first block
				painter.fillRect(colLookupTable[selectionPoint1x],
					rowLookupTable[selectionPoint1y],
					this->width() - colLookupTable[selectionPoint1x],
					m_fontHeight,
					QBrush(cursorColor));

				//fill the between blocks
				if(selectionPoint1y+1 != selectionPoint2y) {
					painter.fillRect(0, rowLookupTable[selectionPoint1y+1], this->width(), (rowLookupTable[selectionPoint2y-1] - rowLookupTable[selectionPoint1y+1]) + m_fontHeight, QBrush(cursorColor));
				}
				painter.fillRect(0, rowLookupTable[selectionPoint2y], colLookupTable[selectionPoint2x] + m_fontWidth, m_fontHeight, QBrush(cursorColor));

			} else if( selectionPoint1y > selectionPoint2y ) {
				//selection point 2 is above selection point 1 
				//draw the first block
				painter.fillRect(colLookupTable[selectionPoint2x], rowLookupTable[selectionPoint2y], this->width() - colLookupTable[selectionPoint2x], m_fontHeight, QBrush(cursorColor));
				//fill the between blocks
				if(selectionPoint2y+1 != selectionPoint1y) {
					//TODO: this is wrong
					painter.fillRect(0, rowLookupTable[selectionPoint2y+1], this->width(), (rowLookupTable[selectionPoint1y-1] - rowLookupTable[selectionPoint2y+1]) + m_fontHeight, QBrush(cursorColor));
				}
				painter.fillRect(0, rowLookupTable[selectionPoint1y], colLookupTable[selectionPoint1x] + m_fontWidth, m_fontHeight, QBrush(cursorColor));

			} else {
				//just a single line
				if( selectionPoint1x < selectionPoint2x ) {
					//point 1 on left of point 2
					painter.fillRect(colLookupTable[selectionPoint1x], rowLookupTable[selectionPoint1y], (colLookupTable[selectionPoint2x] - colLookupTable[selectionPoint1x]) + m_fontWidth, m_fontHeight, QBrush(cursorColor));
				} else {
					painter.fillRect(colLookupTable[selectionPoint2x], rowLookupTable[selectionPoint2y], (colLookupTable[selectionPoint1x] - colLookupTable[selectionPoint2x]) + m_fontWidth, m_fontHeight, QBrush(cursorColor));
				}
			}
		}  // end of painting selection junk
		painter.end();
	}

//#ifdef _DEBUG
//	s.stop();
//	/*double elapsedTime = s..getElapsedTime();
//	printf("...Render Complete!!\nthe render took %f seconds.\n", elapsedTime);*/
//#endif
	
}

void QPainterRenderWidget::resizeEvent(QResizeEvent *resizeEvent) {
	mp_scrollBar->setGeometry(this->width() - SCROLLBARWIDTH, 0, SCROLLBARWIDTH, this->height());
}

void QPainterRenderWidget::mousePressEvent(QMouseEvent *e) {
	if(mp_screen && e->button() == Qt::LeftButton) {
		QPoint xy = e->pos();
		
		int charx = xy.rx() / m_fontWidth;
		int chary = xy.ry() / m_fontHeight;
		if( charx >= mp_screen->termWidth)
			charx = mp_screen->termWidth-1;
		if( chary >= mp_screen->termHeight)
			chary = mp_screen->termHeight-1;
		selectionPoint1x = charx;
		selectionPoint1y = chary;
		selectionPoint2x = charx;
		selectionPoint2y = chary;
		amDraging = true;
		this->update();
	}
}

void QPainterRenderWidget::mouseReleaseEvent(QMouseEvent *e) {
	if(mp_screen && e->button() == Qt::LeftButton) {
		QPoint xy = e->pos();
		
		int charx = xy.rx() / m_fontWidth;
		int chary = xy.ry() / m_fontHeight;
		if( charx >= mp_screen->termWidth)
			charx = mp_screen->termWidth-1;
		if( chary >= mp_screen->termHeight)
			chary = mp_screen->termHeight-1;
		selectionPoint2x = charx;
		selectionPoint2y = chary;
		amDraging = false;
		this->update();
	}
}

void QPainterRenderWidget::mouseMoveEvent(QMouseEvent *e) {
	if(mp_screen) {
			//get the current location information and store the location as char cordenates
			QPoint xy = e->pos();

			int charx = xy.rx() / m_fontWidth;
			int chary = xy.ry() / m_fontHeight;
			if(charx >= mp_screen->termWidth) 
				charx = mp_screen->termWidth-1;
			if(charx < 0)
				charx = 0;
			if(chary >= mp_screen->termHeight)
				chary = mp_screen->termHeight-1;
			if(chary < 0)
				chary = 0;

			if( amDraging ) {
				/* what to do if the left mouse button is down while we move
				 * we need to draw the selection box from selectionPoint1x, selectionPoint1y to the current location
				 * so here is what we are going to do...
				 * set selectionPoint2x, selectionPoint2y to the current locaion and let the paint event do it!!!!  (sweat huh) */
				selectionPoint2x = charx;
				selectionPoint2y = chary;
				this->update();			
			}

#ifdef _DEBUG
		if(mp_userPrefs->debugToolTipEnabled()) {
			int character = (int)mp_screen->pScreen[chary][charx]->sChar.toLatin1();
			QString myTooltip = "Character: ";
				   myTooltip += QString::number(character);
				   myTooltip += " '";
				   myTooltip += mp_screen->pScreen[chary][charx]->sChar;
				   myTooltip += "'\n BG Color: " + QString::number(mp_screen->pScreen[chary][charx]->BGColor);
				   myTooltip += "\n FG Color: " + QString::number(mp_screen->pScreen[chary][charx]->FGColor);
			uchar options = mp_screen->pScreen[chary][charx]->colorEffect;
			if( options & COLOR_EFFECT_BRIGHT )
				myTooltip += "\n BRIGHT";
			if( options & COLOR_EFFECT_DIM )
				myTooltip += "\n DIM";
			if( options & COLOR_EFFECT_REVERSE )
				myTooltip += "\n REVERSE";
			if( options & COLOR_EFFECT_UNDERLINE )
				myTooltip += "\n UNDERLINE";
			this->setToolTip(myTooltip);
		

		} else {
			this->setToolTip("");
		}
#endif
	}
}

void QPainterRenderWidget::wheelEvent ( QWheelEvent * pEvent )
{
	int numDegrees = pEvent->delta() / 8;
	int numSteps = numDegrees / 15;

	if (pEvent->orientation() == Qt::Vertical)
	{
		int scrollValue = mp_scrollBar->value() - (numSteps * mp_scrollBar->pageStep());
		if ( scrollValue < 0)
			mp_scrollBar->setValue( 0);
		else if ( scrollValue > mp_scrollBar->maximum() )
			mp_scrollBar->setValue( mp_scrollBar->maximum() );
		else
			mp_scrollBar->setValue( scrollValue );
	}
	
	pEvent->accept();
}