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
 Other Includes
*************************/
#include "CharacterSet.h"

////////////////////////////////////////////////////////////////////
CharacterSet::CharacterSet(CharSet charSet):
m_currentCharSet(charSet)
{
	//TODO: Add a check to make sure the charSet is valid
}

////////////////////////////////////////////////////////////////////
CharacterSet::~CharacterSet(void)
{

}

////////////////////////////////////////////////////////////////////
/*         If the Special Graphics set is selected, the graphics for ASCII codes
0137 through 0176 will be replaced according to the following table (see the
SCS control sequence).


Octal    ASCII      Special              Octal    ASCII     Special
Code    graphic     graphic              code    graphic    graphic
-------------------------------------------------------------------------------
0137      _         Blank                0157       o       Horiz Line - scan 1
0140      \         Diamond              0160       p       Horiz Line - scan 3
0141      a         Checkerboard         0161       q       Horiz Line - scan 5
0142      b         Digraph: HT          0162       r       Horiz Line - scan 7
0143      c         Digraph: FF          0163       s       Horiz Line - scan 9
0144      d         Digraph: CR          0164       t       Left "T" (|-)
0145      e         Digraph: LF          0165       u       Right "T" (-|)
0146      f         Degree Symbol        0166       v       Bottom "T" (|_)
0147      g         +/- Symbol           0167       w       Top "T" (T)
0150      h         Digraph: NL          0170       x       Vertical Bar (|)
0151      i         Digraph: VT          0171       y       Less/Equal (<_)
0152      j         Lower-right corner   0172       z       Grtr/Egual (>_)
0153      k         Upper-right corner   0173       {       Pi symbol
0154      l         Upper-left corner    0174       |       Not equal (=/)
0155      m         Lower-left corner    0175       }       UK pound symbol
0156      n         Crossing lines (+)   0176       ~       Centered dot

horizontal lines are
152
158
169
196
252

NOTE 1: Codes 0152-0156 and 0164-0170 are used to draw rectangular grids" each
piece of this set is contiguous with other so the lines formed will be
unbroken.

NOTE 2: Codes 0157-0163 give better vertical resolution than dashes and
underlines when drawing graphs; using these segments, 120 x 132 resolution may
be obtained in 132 column mode with the Advanced Video Option installed. */
QChar
CharacterSet::translate(const QChar c) const
{
	switch(m_currentCharSet)
	{
	case CHAR_SET_G0:
		return c;
		break;
	case CHAR_SET_G1:
	case CHAR_SET_LINE_as_G0:
		/* the characters should be one of these: klmnopqrstuvw */
		switch(c.toAscii() )
		{
		case '_':
			return QChar(32);
		case '\\':
			return QChar(9830);
		case 'g':
			return QChar(176);
		case 'h':
			return QChar(241);
		case 'j':
			return QChar(9496);
		case 'k':
			return QChar(9488);
		case 'l':
			return QChar(9484);
		case 'm':
			return QChar(9492);
		case 'n':
			return QChar(9532);
		case 'o':
			return QChar(196);
		case 'p':
			return QChar(196);
		case 'q':
			return QChar(9472);
		case 'r':
			return QChar(196);
		case 's':
			return QChar(219);
		case 't':
			return QChar(9500);
		case 'u':
			return QChar(9508);
		case 'v':
			return QChar(9524);
		case 'w':
			return QChar(9516);
		case 'x':
			return QChar(9474);
		case 'y':
			return QChar(219);
		case 'z':
			return QChar(219);
		}
		//If we got here, we got a character we didn;t expect
		printf("Unexpected character: %c for charset G1\n", c.toAscii() );
		break;
	case CHAR_SET_UK_as_G0:
		printf("Unhandled character set: uk as G0\n");
		break;
	case CHAR_SET_UK_as_G1:
		printf("Unhandled character set: uk as G1\n" );
		break;
	case CHAR_SET_US_as_G0:
		return c;
		break;
	case CHAR_SET_US_as_G1:
		printf("Unhandled character set: us as G1\n");
		break;
	
		printf("Unhandled character set: line as G0\n" );
		break;
	case CHAR_SET_LINE_as_G1:
		printf("Unhandled character set: line as G1\n" );
		break;
	default:
		printf("Unhandled character set: %c\n", c.toAscii() );
		break;
	}
	return QChar(219);
}

