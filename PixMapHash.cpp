#include "PixMapHash.h"

PixMapHash::PixMapHash(QFont font, QHash<int, QColor> *bgColor, QHash<int, QColor> *fgColor)
{
	m_font = font;
	mp_fgColor = fgColor;
	mp_bgColor = bgColor;
}

PixMapHash::~PixMapHash() {
	QPixmap *tempPixmapPointer;
	QHash<ConsoleChar, QPixmap *>::iterator pixMapIterator = this->begin();
	while( pixMapIterator != this->constEnd() )
	{
		tempPixmapPointer = pixMapIterator.value();
		pixMapIterator = this->erase(pixMapIterator);
		delete tempPixmapPointer;
	}
}

//int PixMapHash::insert(const ConsoleChar &key, const QPixmap *&value) {
//	return 0;
//}

QPixmap *PixMapHash::value(const ConsoleChar &key)
 {
	QPixmap *pPM = QHash<ConsoleChar, QPixmap *>::value(key);
	if( pPM )
		return pPM;
	else
	{
		QFontMetrics fm(m_font);
		QPixmap *pTemp = generatePixMap(key, fm);
		QHash<ConsoleChar, QPixmap *>::insert(key, pTemp);
		return pTemp;
	}
}

QPixmap *
PixMapHash::generatePixMap(const ConsoleChar &CC, QFontMetrics &fm)
{
	QSize size;
	size.setHeight(fm.height());
	size.setWidth(fm.maxWidth());
	QPixmap *charPixmap = new QPixmap(size);
	
	//set the font attributes
	QFont tempFont = m_font;
	tempFont.setBold(CC.colorEffect & COLOR_EFFECT_BRIGHT);
	tempFont.setUnderline(CC.colorEffect & COLOR_EFFECT_UNDERLINE);

	//setup the Colors
	QColor bgColor = mp_bgColor->value(CC.BGColor);
	QColor fgColor = mp_fgColor->value(CC.FGColor);
	if(CC.colorEffect & COLOR_EFFECT_DIM)
	{
		bgColor = bgColor.darker();
		fgColor = fgColor.darker();
	}
	if(CC.colorEffect & COLOR_EFFECT_REVERSE)
	{
		QColor temp = bgColor;
		bgColor = fgColor;
		fgColor = temp;
	}

	//fill the background
	charPixmap->fill(bgColor);

	//check if we need to draw the characters
	if(CC.sChar != 0 && CC.sChar != ' ') {
		//setup the painter
		QPainter painter(charPixmap);	
		painter.setPen(fgColor);
		painter.setFont(tempFont);

		//draw the character
		painter.drawText(0 , fm.ascent(), QString(CC.sChar));

		//finalize the painting
		painter.end();
	}

	return charPixmap;	//this is a completed pixmap
}