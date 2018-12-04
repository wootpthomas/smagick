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
*************************/
#include <QApplication>
#include <QTextEdit>
#include <QTextCursor>
#include <QString>
#include <QScrollBar>
#include <QRegExp>


/***********************
 Includes
*************************/
#include "Decoder.h"
#include "SSHConnection.h"

#ifdef _DEBUG
	#include "StopWatch.h"
#endif


#define TEMP_ARRAY_SIZE 16			//Lets us handle something with 16 attributes: <ESC>[1;2;3;4;5;6;....16m
#define TEMP_VAR_SIZE	128		//Each attribute length max size

/***********************
 Useful Docs
*************************/
// http://www.xfree.org/current/ctlseqs.html
// http://www.mit.edu/afs/sipb/project/outland/doc/rxvt/html/refer.html


////////////////////////////////////////////////////////////////////
Decoder::Decoder(Terminal *pTerminal, Terminal *pAltTerminal):
m_cursorKeysInAppMode(false),
m_keypadKeysInAppMode(false),
mp_data(NULL),
mp_last_mp_data(NULL),
m_dataSize(0),
mp_mainTerm(pTerminal),
mp_altTerm( pAltTerminal),
mp_currentTerm(pTerminal)
#ifdef _DEBUG
,
m_debugPrinting(true)
#else
,
m_debugPrinting(false)
#endif
{
	//Set our m_newT string to reserve a min of 64 chars
	m_newT.reserve( 64);

	mp_tempAttrs = new char *[TEMP_ARRAY_SIZE];
	for (int i = 0; i < TEMP_ARRAY_SIZE; i++)
		mp_tempAttrs[i] = new char [TEMP_VAR_SIZE];

	mp_temp = new char[TEMP_VAR_SIZE];
	mp_tempCD = new char[TEMP_VAR_SIZE];
}

////////////////////////////////////////////////////////////////////
Decoder::~Decoder(void)
{
	for (int i = 0; i < TEMP_ARRAY_SIZE; i++)
		delete [] mp_tempAttrs[i];
	delete [] mp_tempAttrs;

	delete [] mp_temp;
	delete [] mp_tempCD;
}

////////////////////////////////////////////////////////////////////
/* Returns an int which tells us how many chars we need to use from mp_data
 * to properly terminate our attribute */
int
Decoder::getFinalCharPos()
{
	if ( m_castratedData[0] != CHAR_ESC)
		printf("Problem found!\n");

	std::string attr;
	attr.append( m_castratedData);
	unsigned i = 0;
	char *pIter = mp_data;
	while (i < strlen(mp_data))
	{
		attr.append(pIter, 1);
	
		int result;
		switch( attr[1] )
		{
			case CHAR_SPACE:	/* These all require 3 chars if properly terminated*/
			case '#':
			case '%':
			case '(':
			case ')':
			case '*':
			case '+':
				result = 3 - m_castratedData.size();
				if ( result < 0)
					printf("oh shit\n");
				return result;
			case '7':		/* These all are terminating characters */
			case '8':
			case '=':
			case '>':
			case 'F':
			case 'c':
			case 'l':
			case 'm':
			case 'n':
			case 'o':
			case '|':
			case '}':
			case '~':
				return 1;
			case '[':
				//If we didn;t match any of those, look for a final character in the range of [a-zA-Z@]
				for (unsigned int i = 1; i < attr.length(); i++)
				{
					if ( (attr[i] >= 'a' && attr[i] <= 'z') ||
						(attr[i] >= 'A' && attr[i] <= 'Z') || attr[i] == '@' )
						return i - m_castratedData.size() + 1;		//properly terminated
				}
				break;
			case ']':
				for (unsigned int i = 1; i < attr.length(); i++)
				{
					if ( attr[i] == CHAR_BELL )
					{
						int value = i - m_castratedData.size() + 1;
						return value;		//properly terminated
					}
				}
				break;
			default:
				/* So at this point, LibSSH2 lost data, the best we can do is see if the last
				 * char added is a <ESC> then we can at least get to a workable attribute */
				//printf("Oh man, we gots problemz\n");
				if ( *pIter == CHAR_ESC)
					return attr.size() - m_castratedData.size() -1;

		}

		i++;		//Increment our counters
		pIter++;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////
void
Decoder::fixCastratedData(char **pDecodeStart, char **pStop)
{
	if ( m_castratedData.size() )
	{
		/* Last time decode ran, we had an attribute that was split
		 * so now we'll build the rest of the attribute and send it through
		 * decode. Find the terminating character. */
		int size = getFinalCharPos();
		/* Make sure our decoder skips over the first few chars that are from last time */
		*pDecodeStart += size * sizeof(char);

		m_castratedData.append( mp_data, size);
		memcpy( mp_tempCD, m_castratedData.c_str(), m_castratedData.size() );
		mp_tempCD[m_castratedData.size()] = 0x0;
		
		char *pTemp = mp_data;
		m_castratedData = "";
		
		decode(mp_tempCD, false);
		mp_data = pTemp;
	}

	//Now we need to see if the current data chopped off any attribute
	//Now to find the last escape char and then see if its been correctly terminated
	char *pEsc = mp_data + ( strlen(mp_data) * sizeof(char));
	pEsc--;
	while( *pEsc != NULL && pEsc != mp_data)
	{
		if ( *pEsc == CHAR_ESC)
			break;
		else
			pEsc--;
	}

	if (pEsc == mp_data)
		return;			//Doesn't contain the <ESC> char
	//else		contains the ESC char

	char *pIter = pEsc;
	int ctr = 1;
	pIter++;		
	if ( *pIter == NULL)  //Does it contain a 2nd char
	{								//nope!
		*pStop = pEsc;		//Tell our decoder to stop processing when it hits this particular ESC sequence
		m_castratedData = *(--pIter);
		return;
	}
	
	//Looking at 2nd char: <ESC><tChar>
	switch( *pIter )
	{
	case CHAR_SPACE:	/* These all require <ESC><char><char> to properly terminated*/
	case '#':
	case '%':
	case '(':
	case ')':
	case '*':
	case '+':
		pIter++;
		ctr++;
		if ( *pIter == NULL)  //Does it contain a 3rd char?
		{								//nope!
			*pStop = pEsc;		//Tell our decoder to stop processing when it hits this particular ESC sequence
			m_castratedData.append(pEsc, ctr);
			return;
		}		
		return;  //Yes, contains a 2nd char... properly terminated
	case '7':
	case '8':
	case '=':
	case '>':
	case 'F':
	case 'c':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case '|':
	case '}':
	case '~':
		return;
	case '[':	//Has a terminating char a-zA-Z, @

		//Look for a final character in the range of [a-zA-Z@]
		while ( *pIter != NULL)
		{
			if ( (*pIter >= 'a' && *pIter <= 'z') ||
				(  *pIter >= 'A' && *pIter <= 'Z') || *pIter == '@' )
				return;		//properly terminated
			else
			{
				pIter++;
				ctr++;
			}
		}

		*pStop = pEsc;		//Tell our decoder to stop processing when it hits this particular ESC sequence
		m_castratedData.append(pEsc, ctr);
		break;
	case ']':
		//Look for a final character in the range of [a-zA-Z@]
		while ( *pIter != NULL)
		{
			if ( *pIter == CHAR_BELL )
				return;		//properly terminated
			else
			{
				pIter++;
				ctr++;
			}
		}

		*pStop = pEsc;		//Tell our decoder to stop processing when it hits this particular ESC sequence
		m_castratedData.append(pEsc, ctr);
		break;

	default:
		printf("Could Not find terminating char!!!!!!\n");
	}
}

////////////////////////////////////////////////////////////////////
void
Decoder::decode(char *pData, bool deleteCharPtr)
{
	/* For ease of passing less parameters, lets set some member variables */
	mp_data = pData;
	m_dataSize = strlen(pData);

#ifdef _DEBUG
	//StopWatch sw;
	//sw.start();
	//printf("****Starting decode. Size: %d...", m_dataSize );
#endif

	char *pIter2 = NULL;
	char *pIter = pData;
	char *pStop = NULL;

	//if (m_debugPrinting) 
	//{
	//	printDebugDisplaySpacing();
	//	printToDebugDisplay();
	//}

	/* If the last time an attribute was cut off, this will fix it. It will also adjust
	 * the start of our parsing position if we used some of the beginning chars of this
	 * current mp_data string.*/
	fixCastratedData( &pIter, &pStop );

	//Initialize member variables for start of decoding
	m_newT.clear();

	for( ; *pIter != NULL; pIter++)
	{
		switch ( *pIter)
		{
			case CHAR_ENQUIRY: 
				/* Return Terminal Status . Default response is the terminal name,
				 * e.g., "xterm", but may be overridden by a resource answerbackString. */
				break;
			case CHAR_BELL:
				emit playBell();
				break;
			case CHAR_BACKSPACE:
				//insertTextIfNeeded();			//Print any text that we might have
				// We just move the cursor left
				mp_currentTerm->moveCursor(MOVE_LEFT);
				break;
			case CHAR_HORIZONTAL_TAB:
				break;
			case CHAR_VERTICAL_TAB:
				break;
			case CHAR_NEW_PAGE:
			case CHAR_NEW_LINE:
				//insertTextIfNeeded();		//Print any text that we might have
				mp_currentTerm->insertNewLine();
				break;
			case CHAR_CARRIAGE_RETURN:
				//insertTextIfNeeded();		//Print any text that we might have
				mp_currentTerm->insertCarriageReturn();
				break;
			case CHAR_SHIFT_OUT:
				//insertTextIfNeeded();			//Print any text that we might have
				mp_currentTerm->changeCharSet( CHAR_SET_G1);
				break;
			case CHAR_SHIFT_IN:
				//insertTextIfNeeded();			//Print any text that we might have
				mp_currentTerm->changeCharSet( CHAR_SET_G0);
				break;
			case CHAR_ESC:
				if ( pStop == pIter)
					break;			//Stop processing, this attribute is broken
				pIter2 = pIter;
				pIter2++;

				if ( *pIter2 == NULL )		//Is it safe to look 1 char ahead?
				{
					printf("We encountered the end of the string when we expected an attribute value!\n");
					break;
				}

				//BugFix - libssh2 sometimes breaks attributes and we get double <ESC>'s. Test and fix if found
				if ( *pIter2 == CHAR_ESC)
				{
					pIter2++;
					if ( *pIter2 == NULL)
						break;
				}

				//Move our stuff to the next character
				pIter++;
				switch ( *pIter )
				{
					case '[':
						handleStdAttr( &pIter);
						continue;
					case ']':
						handleXTermEscapeSequence( &pIter);
						continue;
					case '(':
					case ')':
					case CHAR_SHIFT_OUT:
					case CHAR_SHIFT_IN:
						handleCharSetAttr( &pIter);
						continue;
					case '=':  //<ESC>=
						this->m_keypadKeysInAppMode = true;
						continue;
					case '>':  //<ESC>>
						this->m_keypadKeysInAppMode = false;
						continue;
					case '#':
					//	handleLineAttr(data, i);
					//	break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
						printf("Unhandled attribute: <ESC>%c\n", *pIter);
						break;
					case '7':
						mp_currentTerm->saveCursor();
						break;
					case '8':
						mp_currentTerm->restoreCursor();
						break;
					case '9':
						printf("Unhandled attribute: <ESC>%c\n", *pIter);
						break;
					//case 'A'-'Z':
					case 'D':
						mp_currentTerm->scroll(MOVE_UP, 1);
						break;
					case 'M':
						mp_currentTerm->scroll(MOVE_DOWN, 1);
						break;
					default:
						printf("Unhandled attribute: <ESC>%c\n", *pIter);
						break;
				}
				break;
			case 'Ì':		//Skip this garbage
				break;
			default:  /* We have plaintext, just tack it on... */
				/* Lets be smart about appending text. If we hit text, lets iterate forward
				 * and see how much text we can append. The more we append in one go, the faster
				 * it will be overall */
				char *pIterPT = pIter + sizeof(char);		//Start our looking one ahead
				int plainTNum = 1;
				while ( *pIterPT && (*pIterPT > 31 && *pIterPT < 127))
				{
					pIterPT++;
					plainTNum++;
				}

				//Cool, we now know that we can append plainTNum characters starting from pIter. append them
				m_newT.append( pIter, plainTNum);

				//Move our main iterator to the end of what we appended
				pIter = --pIterPT;

				//Now since we grabbed all the printable text, lets print it
				mp_currentTerm->insertText( m_textFormat, m_newT );
				m_newT = "";	//redneck quick clearing ;p

				break;
		}

		if ( pStop == pIter)
			break;			//Stop processing, this attribute is broken
	}

#ifdef _DEBUG
	//sw.stop();
#endif

	if (mp_data && deleteCharPtr)
	{
		if (deleteCharPtr)
		{
			delete [] mp_last_mp_data;
			mp_last_mp_data = mp_data;
			mp_data = NULL;
			m_dataSize = 0;

			//delete [] mp_data;
			//mp_data = NULL;
			//m_dataSize = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////
void
Decoder::toggleDebugPrinting(bool enable)
{
	QMutexLocker lock( &m_mutex);
	m_debugPrinting = enable;
}

////////////////////////////////////////////////////////////////////
RenderInfo* 
Decoder::getRenderInfo()
{
	QMutexLocker lock( &m_mutex);
	if (mp_currentTerm)
		return mp_currentTerm->getRenderInfo();
	return NULL;
}

////////////////////////////////////////////////////////////////////
bool
Decoder::areKeypadKeysInAppMode()
{
	QMutexLocker lock( &m_mutex);
	return m_keypadKeysInAppMode;
}

////////////////////////////////////////////////////////////////////
bool
Decoder::areCursorKeysInAppMode()
{
	QMutexLocker lock( &m_mutex);
	return m_cursorKeysInAppMode;
}

////////////////////////////////////////////////////////////////////
void
Decoder::switchTerminals(bool useAltTerm)
{
	if (useAltTerm)
	{
		mp_currentTerm = mp_altTerm;
	}
	else
	{
		mp_currentTerm = mp_mainTerm;
	}

	//Mark both console's as dirty to force our RenderWidget to redraw everything
	mp_altTerm->setConsoleDirty();
	mp_mainTerm->setConsoleDirty();
}

////////////////////////////////////////////////////////////////////
void
Decoder::printDebugDisplaySpacing()
{
	printf("\n---------------------------------------------------\n");
}

////////////////////////////////////////////////////////////////////
void
Decoder::printToDebugDisplay(char * pData, bool newLine)
{
	if ( ! pData)
		pData = mp_data;
	if ( ! pData)
		return;

	char *pIter = pData;
	do {
		switch ( *pIter ){
			case CHAR_NULL:						printf("<NULL>");	break;
			case CHAR_START_OF_HEADING:		printf("<START OF HEADING>");	break;
			case CHAR_START_OF_TEXT:			printf("<START OF TEXT>");	break;
			case CHAR_END_OF_TEXT:				printf("<END OF TEXT>");	break;
			case CHAR_END_OF_TRANSMISSION:	printf("<END OF TRANSMISSION>");	break;
			case CHAR_ENQUIRY:					printf("<END OF TRANSMISSION>");	break;
			case CHAR_ACKNOWLEDGE:				printf("<ACKNOWLEDGE>");	break;
			case CHAR_BELL:						printf("<BELL>");	break;
			case CHAR_BACKSPACE:					printf("<BACKSPACE>");	break;
			case CHAR_HORIZONTAL_TAB:			printf("<HORIZONTAL TAB>"); break;
			case CHAR_NEW_LINE:					printf("<NEW LINE>"); break;
			case CHAR_VERTICAL_TAB:				printf("<VERTICAL TAB>");	break;
			case CHAR_NEW_PAGE:					printf("<NEW PAGE>");		break;
			case CHAR_CARRIAGE_RETURN:			printf("<CARRIAGE RETURN>");	break;
			case CHAR_SHIFT_OUT:					printf("<SHIFT OUT>");		break;
			case CHAR_SHIFT_IN:					printf("<SHIFT IN>");	break;
			case CHAR_DATA_LINK_ESC:			printf("<DATA LINE ESC>");		break;
			case CHAR_DEVICE_CONTROL_1:		printf("<DEVICE CONTROL 1>");		break;
			case CHAR_DEVICE_CONTROL_2:		printf("<DEVICE CONTROL 2>");	break;
			case CHAR_DEVICE_CONTROL_3:		printf("<DEVICE CONTROL 3>");	break;
			case CHAR_DEVICE_CONTROL_4:		printf("<DEVICE CONTROL 4>");	break;
			case CHAR_ESC:							printf("<ESC>");	break;
			case CHAR_SPACE:						printf(".");		break;
			default:									printf( "%c", *pIter );
		}

		pIter++;		//Move to next items
	} while ( *pIter != NULL);

	if ( newLine)
		printf("\n");
}

////////////////////////////////////////////////////////////////////
bool
Decoder::findFinalChar(char **ppIter, char finalCharToFind)
{
	char
		*pFC = (*ppIter);
	
	int
		attrSize = -1;

	do{
		pFC++;
		attrSize++;

		if ( *pFC == finalCharToFind)
			break;

	} while ( *pFC != NULL);

	if (attrSize)			//hmmm this IF is a possible crash location.. ;p PT
	{
		char *pSource = *ppIter;
		pSource++;
		memcpy( mp_temp, pSource, attrSize);
		mp_temp[attrSize] = 0x0;	//Null terminate the string
		*ppIter = pFC;		//Move our main decode() iterator to our final char
	}
	else
		memset(mp_temp, 0x0, TEMP_VAR_SIZE);

	return attrSize;
}

////////////////////////////////////////////////////////////////////
// Make this thing return the size of the attributes sucked out
void
Decoder::splitStrings(char *pAttr, char splitChar, int &attrListSize)
{
	if (pAttr)
	{
		//Figure out how
		attrListSize = 0;
		char *pIter = pAttr;
		int size;
		
		while ( *pAttr)
		{
			size = 0;
			while ( *pIter )
			{
				if ( *pIter == splitChar )
				{
					pIter++;
					break;
				}
				size++;
				pIter++;
			}

			if (size > TEMP_VAR_SIZE)
				size = TEMP_VAR_SIZE-1;

			//Clear the temp char array befre we copy the new data into it
			memset(mp_tempAttrs[attrListSize], 0x0, TEMP_VAR_SIZE);

			memcpy(mp_tempAttrs[attrListSize], pAttr, size);
			pAttr = pIter;
			attrListSize++;
		}
	}
}

////////////////////////////////////////////////////////////////////
/* Parses out the 'Ps' from a standard CSI:
 * <ESC>[Ps m   */
int
Decoder::getIntAttrValue(const char* pAttr)
{
	int attrValue = 1;
	if ( pAttr )
	{
		attrValue = atoi(pAttr);
		if ( ! attrValue)
			attrValue = 1;
	}
	return attrValue;
}

////////////////////////////////////////////////////////////////////
/*   ESC ] Ps;Pt BEL		Set XTerm Parameters
 *Ps = 0			    Change Icon Name and Window Title to Pt
 *Ps = 1			    Change Icon Name to Pt
 *Ps = 2			    Change Window Title to Pt
 *Ps = 10			    menuBar command Pt (rxvt compile-time option)
 *Ps = 20			    Change default background pixmap to Pt (rxvt compile-time option) 
 *Ps = 39			    Change default foreground color to Pt (rxvt compile-time option)
 *Ps = 46			    Change Log File to Pt(normally disabled by a compile-time option) unimplemented
 *Ps = 49			    Change default background color to Pt (rxvt compile-time option)
 *Ps = 50
    Set Font to Pt, with the following special values of Pt (rxvt)
    #+n : change up n font(s)
    #-n : change down n font(s)

          if n is missing or 0, a value of 1 is used

    empty : change to font0
    #n : change to fontName 
*/
////////////////////////////////////////////////////////////////////
void
Decoder::handleXTermEscapeSequence(char **ppIter)
{
	char *pSendStr = NULL;

	//Let's scan down and see if we can find the final character (BELL)
	if ( ! findFinalChar(ppIter, CHAR_BELL) )
	{
		printf("Count Not find terminating char!\n");
		return;
	}
	
	//Lets create our list of char* (strings). Each string is an attribute
	int attrListSize = 0;
	splitStrings(mp_temp, ';', attrListSize);

	if (attrListSize == 2)
	{
		int value = atoi( mp_tempAttrs[0] );
		switch(value)
		{
			case 0:	//Change Icon Name and Window Title to Pt
				emit windowTitleChanged( QString( mp_tempAttrs[1])  );
				break;
			case 1:	//Change Icon Name to Pt
				printf("Unhandled XTerm value in escape sequence: %d\n", value);
				break;
			case 2:	//Change Window Title to Pt
				emit windowTitleChanged( QString(mp_tempAttrs[1]) );
				break;
			case 3:	//Set X property on top-level window. Pt should be in the form "prop=value", or just "prop" to delete the property
			case 4:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 46:
			case 50:
			default:
				printf("Unhandled XTerm value in escape sequence: %d\n", value);
		}
	}
}

////////////////////////////////////////////////////////////////////
/* Standard attributes come in the form:
 * ESC [ <anything>
 * for Example:
 *	ESC [ m
 *	ESC [ 5 m
 *	ESC [ pt ; pb r
 *	ESC [ ? 1 l		<- last character is the letter l
 *	ESC [ ? 1 9 h
 *	ESC [ 0 1 ; 3 4 m
 * These so called "standard attributes" as I've named them will always have
 * a letter as the final character */
void
Decoder::handleStdAttr(char **ppIter)
{
	//Find the position of the final char( pFC is pointer to final char). Setup our iterator shiz
	int attrSize = 0;
	char *pFC = (*ppIter);
	pFC++;			//Move our pointer past the '['
	while ( *pFC )
	{
		if ( ( (*pFC) >= 'a' && (*pFC) <='z') ||
			(  (*pFC) >= 'A' && (*pFC) <='Z') ||
			(*pFC) == '@')
		{
			break;
		}

		pFC++;			//Move our pointer to next character
		attrSize++;		//Increase the attribute size
	}

	if ( ! *pFC)	
	{
		printf("Could not find terminating char in handleStdAttr()!\n");
		return;
	}
	
	char *pSource = (*ppIter);  //pSource points to the '['
	if (attrSize)
	{
		pSource++;
		memcpy( mp_temp, pSource, attrSize);
		mp_temp[attrSize] = NULL;		//NULL terminate the string
	}
	else
		mp_temp[0] = NULL;

	/* Before we jump into things, since we have pulled out the attribute and have the pFC 
	 * (terminating/final character), lets move the ppIter so our main decode()
	 * function doesn't see any of this. */
	(*ppIter) = pFC;

	switch( *pFC )
	{
		case 'd':
			decodeLowerD_attr(mp_temp);
			break;
		case 'c':
			decodeLowerC_attr(mp_temp);
			break;
		case 'f':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'l':
			decodeLowerL_attr(mp_temp);
			break;
		case 'g':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'h':
			decodeLowerH_attr(mp_temp);
         break;
		case 'i':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'm':		//ooo! Parsing font color attributes
			decodeLowerM_attr(mp_temp);
			break;
		case 'n':
		case 'q':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'r':
         decodeLowerR_attr(mp_temp);
         break;
		case 'y':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'A':
         decodeCapABCDEF_attr(mp_temp, MOVE_UP);
         break;
		case 'B':
         decodeCapABCDEF_attr(mp_temp, MOVE_DOWN);
         break;
		case 'C':
         decodeCapABCDEF_attr(mp_temp, MOVE_RIGHT);
         break;
		case 'D':
         decodeCapABCDEF_attr(mp_temp, MOVE_LEFT);
         break;
		case 'E':
         decodeCapABCDEF_attr(mp_temp, MOVE_NEXT_LINE);
         break;
		case 'F':
         decodeCapABCDEF_attr(mp_temp, MOVE_PREVIOUS_LINE);
         break;
		case 'G':
			decodeCapG_attr(mp_temp);
			break;
		case 'H':
         decodeCapH_attr(mp_temp);
         break;
		case 'I':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'J':
			decodeCapJK_attr(mp_temp, true);
			break;
		case 'K':
			decodeCapJK_attr(mp_temp, false);
			break;
		case 'L':
			mp_currentTerm->insertBlankLines( getIntAttrValue(mp_temp));
			break;
		case 'M':
		case 'N':
		case 'O':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'P':
			mp_currentTerm->deleteCharacters( getIntAttrValue( mp_temp) );
			break;
		case 'Q':
		case 'R':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'S':
			decodeCapS_T_attr('S', mp_temp);
			break;
		case 'T':
			decodeCapS_T_attr('T', mp_temp);
			break;
		case 'U':
		case 'V':
		case 'W':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case 'X':
			mp_currentTerm->eraseCharacters( getIntAttrValue(mp_temp), m_textFormat);
			break;
		case 'Y':
		case 'Z':
			printf("Attribute not yet supported: %c\n", *pFC);
			break;
		case '@':
			mp_currentTerm->insertSpaces( getIntAttrValue(mp_temp) );
			break;
		default:
			printf("Attribute not in switch statment: %c\n", *pFC);
	}
}

////////////////////////////////////////////////////////////////////
/***********CSI P s c**************
Send Device Attributes (Primary DA)
	P s = 0 or omitted -> request attributes from terminal. The response depends on the decTerminalID resource setting.
-> CSI ? 1 ; 2 c (‘‘VT100 with Advanced Video Option’’)
-> CSI ? 1 ; 0 c (‘‘VT101 with No Options’’)
-> CSI ? 6 c (‘‘VT102’’)
-> CSI ? 6 0 ; 1 ; 2 ; 6 ; 8 ; 9 ; 1 5 ; c (‘‘VT220’’)
The VT100-style response parameters do not mean anything by themselves. VT220 parameters do, telling the host what features the terminal supports:
-> 1 132-columns
-> 2 Printer
-> 6 Selective erase
-> 8 User-defined keys
-> 9 National replacement character sets
-> 1 5 Technical characters
-> 2 2 ANSI color, e.g., VT525
-> 2 9 ANSI text locator (i.e., DEC Locator mode)

*********CSI > P s c**********
Send Device Attributes (Secondary DA) 	
P s = 0 or omitted -> request the terminal’s identification code. The response
depends on the decTerminalID resource setting. It should apply only to VT220
and up, but xterm extends this to VT100.

-> CSI > P p ; P v ; P c c
where P p denotes the terminal type
-> 0 (‘‘VT100’’)
-> 1 (‘‘VT220’’)
and P v is the firmware version (for xterm, this is the XFree86 patch number, starting with 95). In a DEC terminal, P c indicates the ROM cartridge registration number and is always zero.  */
void
Decoder::decodeLowerC_attr(const char *pAttr)
{
	char *pIter = const_cast<char*>(pAttr);
	if ( *pIter == '>')
	{
		pIter++;
		//The server is requesting the terminals ID code
		printf("Unhandled 'c' attribute: %s\n", pIter );

	}
	else
	{
		printf("Unhandled 'c' attribute: %s\n", pIter );
	}
}

////////////////////////////////////////////////////////////////////
/* XTERM SPECIFIC
 * ESC [ Ps d
 *  Cursor to Line Ps (VPA)  */
void
Decoder::decodeLowerD_attr(const char *pAttr)
{
	int row = atoi( pAttr);
	if ( row)
		mp_currentTerm->setCursorPosition(row, -1);	//Move to row, do not change columns
}

////////////////////////////////////////////////////////////////////
/****** CSI P m h 		Set Mode (SM) 	******
P s = 2 -> Keyboard Action Mode (AM)
P s = 4 -> Insert Mode (IRM)
P s = 12 -> Send/receive (SRM)
P s = 20 -> Automatic Newline (LNM)

***** CSI ? P m h *****
P s = 1 -> Application Cursor Keys (DECCKM)
P s = 2 -> Designate USASCII for character sets G0-G3 (DECANM), and set VT100 mode.
P s = 3 -> 132 Column Mode (DECCOLM)
P s = 4 -> Smooth (Slow) Scroll (DECSCLM)
P s = 5 -> Reverse Video (DECSCNM)   (black characters on white screen mode)
P s = 6 -> Origin Mode (DECOM)
P s = 7 -> Wraparound Mode (DECAWM)	(auto wrap to new line)
P s = 8 -> Auto-repeat Keys (DECARM)	(keyboard auto repeat mode on)
P s = 9 -> Send Mouse X & Y on button press. See the section Mouse Tracking.
P s = 10 -> Show toolbar (rxvt)
P s = 12 -> Start Blinking Cursor (att610)
P s = 18 -> Print form feed (DECPFF)	(print form feed on)
P s = 19 -> Set print extent to full screen (DECPEX)	(print whole screen)
P s = 25 -> Show Cursor (DECTCEM)
P s = 30 -> Show scrollbar (rxvt).
P s = 35 -> Enable font-shifting functions (rxvt).
P s = 38 -> Enter Tektronix Mode (DECTEK)
P s = 40 -> Allow 80 -> 132 Mode
P s = 41 -> more(1) fix (see curses resource)
P s = 42 -> Enable Nation Replacement Character sets (DECNRCM)
P s = 44 -> Turn On Margin Bell
P s = 45 -> Reverse-wraparound Mode
P s = 46 -> Start Logging (normally disabled by a compile-time option)
P s = 47 -> Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
P s = 66 -> Application keypad (DECNKM)
P s = 67 -> Backarrow key sends backspace (DECBKM)
******** Tracking ************
P s = 1000 -> Send Mouse X & Y on button press and release. See the section Mouse Tracking.
P s = 1001 -> Use Hilite Mouse Tracking.
P s = 1002 -> Use Cell Motion Mouse Tracking.
P s = 1003 -> Use All Motion Mouse Tracking.
P s = 1010 -> Scroll to bottom on tty output (rxvt).
P s = 1011 -> Scroll to bottom on key press (rxvt).
P s = 1035 -> Enable special modifiers for Alt and NumLock keys.
P s = 1036 -> Send ESC when Meta modifies a key (enables the metaSendsEscape resource).
P s = 1037 -> Send DEL from the editing-keypad Delete key
P s = 1047 -> Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
P s = 1048 -> Save cursor as in DECSC (unless disabled by the titeInhibit resource)
P s = 1049 -> Save cursor as in DECSC and use Alternate Screen Buffer, clearing it first (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.
P s = 1051 -> Set Sun function-key mode.
P s = 1052 -> Set HP function-key mode.
P s = 1053 -> Set SCO function-key mode.
P s = 1060 -> Set legacy keyboard emulation (X11R6).
P s = 1061 -> Set Sun/PC keyboard emulation of VT220 keyboard.
P s = 2004 -> Set bracketed paste mode. */
void
Decoder::decodeLowerH_attr(const char * pAttr)
{
	char *pIter = const_cast<char*>(pAttr);
	if ( ! *pIter)
		return;

	if ( *pIter == '?')
	{
		pIter++;		//Move over the '?'
		//this->spl

		int attrListSize = 0;
		splitStrings(pIter, ';', attrListSize);
		for (int i = 0; i < attrListSize; i++)
		{
			/// fix me
			int value = atoi( mp_tempAttrs[i] );

			if ( ! value)
			{
				printf("Unexpected 'h' attr: %s", mp_tempAttrs[i] );
				continue;
			}

			switch(value)
			{
			case 1:
				m_cursorKeysInAppMode = true;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				printf("Recieved unhandled 'h' attr: %d\n", value);
				break;
			case 7: //Autowrap to newline ENABLED
				mp_mainTerm->setAutoWrapToNewLine(true);
				mp_altTerm->setAutoWrapToNewLine(true);
				break;
			case 8:
			case 9:
			case 10:
			case 12:
			case 18:
			case 19:
				printf("Recieved unhandled 'h' attr: %d\n", value);
				break;
			case 25:
				mp_currentTerm->setCursorVisible(true);
				break;
			case 30:
			case 35:
			case 38:
			case 40:
			case 41:
			case 42:
			case 43:
			case 45:
			case 46:
			case 47:
			case 66:
			case 67:
			case 1000:
			case 1001:
			case 1002:
			case 1003:
			case 1010:
			case 1011:
			case 1035:
			case 1036:
			case 1037:
			case 1047:
			case 1048:
				printf("Recieved unhandled 'h' attr: %d\n", value);
				break;
			case 1049:
				switchTerminals(true);
				break;
			case 1051:
			case 1052:
			case 1053:
			case 1060:
			case 1061:
			case 2004:
			default:
				printf("Recieved unhandled 'h' attr: %d\n", value);
			}
		}
	}
	else
	{
		int attrListSize = 0;
		splitStrings(pIter, ';', attrListSize);
		for (int i = 0; i < attrListSize; i++)
		{
			int value = 0;
			if ( mp_tempAttrs[i] )
				value = atoi( mp_tempAttrs[i] );

			if ( ! value)
			{
				printf("Unexpected 'h' attr: %s", mp_tempAttrs[i] );
				continue;
			}

			switch(value)
			{
			case 2:
			case 4:
			case 12:
			case 20:
			default:
				printf("Unhandled 'h' attribute: %d\n", value);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
/********CSI P m l**********
 Reset Mode (RM) 	
P s = 2 -> Keyboard Action Mode (AM)
P s = 4 -> Replace Mode (IRM)
P s = 12 -> Send/receive (SRM)
P s = 20 -> Normal Linefeed (LNM)
**********CSI ? P m l******
 DEC Private Mode Reset (DECRST) 	
P s = 1 -> Normal Cursor Keys (DECCKM)
P s = 2 -> Designate VT52 mode (DECANM).
P s = 3 -> 80 Column Mode (DECCOLM)
P s = 4 -> Jump (Fast) Scroll (DECSCLM)
P s = 5 -> Normal Video (DECSCNM)
P s = 6 -> Normal Cursor Mode (DECOM)
P s = 7 -> No Wraparound Mode (DECAWM)
P s = 8 -> No Auto-repeat Keys (DECARM)
P s = 9 -> Don’t Send Mouse X & Y on button press
P s = 10 -> Hide toolbar (rxvt)
P s = 12 -> Stop Blinking Cursor (att610)
P s = 18 -> Don’t print form feed (DECPFF)
P s = 19 -> Limit print to scrolling region (DECPEX)
P s = 25 -> Hide Cursor (DECTCEM)
P s = 30 -> Don’t show scrollbar (rxvt).
P s = 35 -> Disable font-shifting functions (rxvt).
P s = 40 -> Disallow 80 -> 132 Mode
P s = 41 -> No more(1) fix (see curses resource)
P s = 42 -> Disable Nation Replacement Character sets (DECNRCM)
P s = 44 -> Turn Off Margin Bell
P s = 45 -> No Reverse-wraparound Mode
P s = 46 -> Stop Logging (normally disabled by a compile-time option)
P s = 47 -> Use Normal Screen Buffer
P s = 66 -> Numeric keypad (DECNKM)
P s = 67 -> Backarrow key sends delete (DECBKM)
P s = 1000 -> Don’t Send Mouse X & Y on button press and release. See the section Mouse Tracking.
P s = 1001 -> Don’t Use Hilite Mouse Tracking
P s = 1002 -> Don’t Use Cell Motion Mouse Tracking
P s = 1003 -> Don’t Use All Motion Mouse Tracking
P s = 1010 -> Don’t scroll to bottom on tty output (rxvt).
P s = 1011 -> Don’t scroll to bottom on key press (rxvt).
P s = 1035 -> Disable special modifiers for Alt and NumLock keys.
P s = 1036 -> Don’t send ESC when Meta modifies a key (disables the metaSendsEscape resource).
P s = 1037 -> Send VT220 Remove from the editing-keypad Delete key
P s = 1047 -> Use Normal Screen Buffer, clearing screen first if in the Alternate Screen (unless disabled by the titeInhibit resource)
P s = 1048 -> Restore cursor as in DECRC (unless disabled by the titeInhibit resource)
P s = 1049 -> Use Normal Screen Buffer and restore cursor as in DECRC (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.
P s = 1051 -> Reset Sun function-key mode.
P s = 1052 -> Reset HP function-key mode.
P s = 1053 -> Reset SCO function-key mode.
P s = 1060 -> Reset legacy keyboard emulation (X11R6).
P s = 1061 -> Reset Sun/PC keyboard emulation of VT220 keyboard.
P s = 2004 -> Reset bracketed paste mode.  */
void
Decoder::decodeLowerL_attr(const char *pAttr)
{
	char *pIter = const_cast<char*>(pAttr);
	if ( ! *pIter)
		return;

	if ( *pIter == '?')
	{
		pIter++;		//Move over the '?'
		int attrListSize = 0;
		splitStrings(pIter, ';', attrListSize);
		for (int i = 0; i < attrListSize; i++)
		{
			int value = 0;
			if ( mp_tempAttrs[i] )
				value = atoi( mp_tempAttrs[i] );

			if ( ! value)
			{
				printf("Unexpected 'l' attr: %s\n", mp_tempAttrs[i] );
				continue;
			}

			switch(value)
			{
			case 1:
				m_cursorKeysInAppMode = false;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				printf("Recieved unhandled 'l' attr: %d\n", value);
				break;
			case 7:  //autowrap to newline DISABLED
				mp_mainTerm->setAutoWrapToNewLine(false);
				mp_altTerm->setAutoWrapToNewLine(false);
				break;
			case 8:
			case 9:
			case 10:
			case 12:
			case 18:
			case 19:
				printf("Recieved unhandled 'l' attr: %d\n", value);
				break;
			case 25:
				mp_currentTerm->setCursorVisible(false);
				break;
			case 30:
			case 35:
			case 40:
			case 41:
			case 42:
			case 44:
			case 45:
			case 46:
			case 47:
			case 66:
			case 67:
			case 1000:
			case 1001:
			case 1002:
			case 1003:
			case 1010:
			case 1011:
			case 1035:
			case 1036:
			case 1037:
			case 1047:
			case 1048:
				printf("Recieved unhandled 'l' attr: %d\n", value);
				break;
			case 1049:
				switchTerminals(false);
				break;
			case 1051:
			case 1052:
			case 1053:
			case 1060:
			case 1061:
			case 2004:
			default:
				printf("Recieved unhandled 'l' attr: %d\n", value);
			}
		}
	}
	else
	{
		int attrListSize = 0;
		splitStrings(pIter, ';', attrListSize);
		for (int i = 0; i < attrListSize; i++)
		{
			int value = 0;
			if ( mp_tempAttrs[i])
				value = atoi( mp_tempAttrs[i] );

			if ( ! value)
			{
				printf("Unexpected 'l' attr: %s\n", mp_tempAttrs[i] );
				continue;
			}

			switch(value)
			{
			case 2:
			case 4:
			case 12:
			case 20:
			default:
				printf("Unhandled 'l' attribute: %d\n", value);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
/* CSI P m m
 *	Character Attributes (SGR) 	
 *	P s = 0 -> Normal (default)
 ********************************
P s = 1 -> Bold
P s = 4 -> Underlined
P s = 5 -> Blink (appears as Bold)
P s = 7 -> Inverse
P s = 8 -> Invisible, i.e., hidden (VT300)
P s = 22 -> Normal (neither bold nor faint)
P s = 24 -> Not underlined
P s = 25 -> Steady (not blinking)
P s = 27 -> Positive (not inverse)
P s = 28 -> Visible, i.e., not hidden (VT300)
P s = 30 -> Set foreground color to Black
P s = 31 -> Set foreground color to Red
P s = 32 -> Set foreground color to Green
P s = 33 -> Set foreground color to Yellow
P s = 34 -> Set foreground color to Blue
P s = 35 -> Set foreground color to Magenta
P s = 36 -> Set foreground color to Cyan
P s = 37 -> Set foreground color to White
P s = 39 -> Set foreground color to default (original)
P s = 40 -> Set background color to Black
P s = 41 -> Set background color to Red
P s = 42 -> Set background color to Green
P s = 43 -> Set background color to Yellow
P s = 44 -> Set background color to Blue
P s = 45 -> Set background color to Magenta
P s = 46 -> Set background color to Cyan
P s = 47 -> Set background color to White
P s = 49 -> Set background color to default (original). If 16-color
	support is compiled, the following apply. Assume that xterm’s
	resources are set so that the ISO color codes are the first 8 of
	a set of 16. Then the aixterm colors are the bright versions of the ISO colors:
P s = 90 -> Set foreground color to Black
P s = 91 -> Set foreground color to Red
P s = 92 -> Set foreground color to Green
P s = 93 -> Set foreground color to Yellow
P s = 94 -> Set foreground color to Blue
P s = 95 -> Set foreground color to Magenta
P s = 96 -> Set foreground color to Cyan
P s = 97 -> Set foreground color to White
P s = 100 -> Set background color to Black
P s = 101 -> Set background color to Red
P s = 102 -> Set background color to Green
P s = 103 -> Set background color to Yellow
P s = 104 -> Set background color to Blue
P s = 105 -> Set background color to Magenta
P s = 106 -> Set background color to Cyan
P s = 107 -> Set background color to White If xterm is compiled with the 16-color support disabled, it supports the following, from rxvt:
P s = 100 -> Set foreground and background color to default If 88- or 256-color support is compiled, the following apply.
P s = 38 ; 5 ; P s -> Set foreground color to the second P s
P s = 48 ; 5 ; P s -> Set background color to the second P s */
#ifndef _DEBUG
	inline
#endif
void
Decoder::decodeLowerM_attr(const char* pAttr)
{
	char *pIter = const_cast<char*>(pAttr);
	if ( pIter && strlen(pIter) )
	{
		int attrListSize = 0;
		splitStrings(pIter, ';', attrListSize);

		for (int i = 0; i < attrListSize; i++)
		{
			int attrValue = 0;
			if ( mp_tempAttrs[i] )
				attrValue = atoi( mp_tempAttrs[i] );
			
			setColorAttr(attrValue);
		}
	}
	else		//Reset color attributes to their default values
	{			// we must have gotten ESC[m */
		m_textFormat.colorEffect = COLOR_EFFECT_DEFAULT | COLOR_EFFECT_CHAR_IS_DIRTY;
		m_textFormat.BGColor = 0;
		m_textFormat.FGColor = 0;
	}
}

////////////////////////////////////////////////////////////////////
void
Decoder::handleCharSetAttr(char** ppAttr)
{
	if ( ! **ppAttr)
		return;

	if ( **ppAttr == '(')
	{
		(*ppAttr)++;
		if ( ! (**ppAttr) )
			return;

		switch( **ppAttr )
		{
		case 'A':
			mp_currentTerm->changeCharSet(CHAR_SET_UK_as_G0);
			return;
		case 'B':
			mp_currentTerm->changeCharSet(CHAR_SET_US_as_G0);
			return;
		case '0':
			mp_currentTerm->changeCharSet(CHAR_SET_LINE_as_G0);
			return;
		}
	}
	else if ( **ppAttr == ')')
	{
		(*ppAttr)++;
		if ( ! (**ppAttr) )
			return;

		switch( **ppAttr)
		{
		case 'A':
			mp_currentTerm->changeCharSet(CHAR_SET_UK_as_G1);
			return;
		case 'B':
			mp_currentTerm->changeCharSet(CHAR_SET_US_as_G1);
			return;
		case '0':
			mp_currentTerm->changeCharSet(CHAR_SET_LINE_as_G1);
			return;
		}
	}
	else if ( **ppAttr == CHAR_SHIFT_IN)
	{
		mp_currentTerm->changeCharSet(CHAR_SET_LINE_as_G0);
		return;
	}
	else if ( **ppAttr == CHAR_SHIFT_OUT)
	{
		mp_currentTerm->changeCharSet(CHAR_SET_LINE_as_G1);
		return;
	}

	////If we get here, It wasn't any of the attributes
	printf("Unexpected attribute: %c\n", **ppAttr );
}

//////////////////////////////////////////////////////////////////////
//void
//Decoder::handleUpperCaseLetterAttr(const QString &data)
//{
//	char testChar = data[parseIndex].toLatin1();
//	switch(testChar)
//	{
//	case 'D':
//		/* Index:
//		 *	This sequence causes the cursor to move downward one line without
//		 *	changing the column.  If the cursor is at the bottom margin, a scroll
//		 *	up is performed.  Format Effector. */
//		mp_currentTerm->moveCursor(MOVE_DOWN, 1);
//		break;
//
//	case 'E':
//		/* Next Line:
//		 *	This causes the cursor to move to the first position of the next line
//		 *	down.  If the cursor is on the bottom line, a scroll is performed.
//		 *	Format Effector. */
//
//	case 'H': 
//		/* Horizontal Tab Set:
//		 * Set a tab stop at the current cursor position.  Format Effector. */
//		printf("Unhandled upper case attr: %c\n", data[parseIndex].toAscii() );
//		break;
//
//	case 'M':
//		/* Reverse Index:
//		 * Move the cursor up one line without changing columns.  If the cursor is
//		 * on the top line, a scroll down is performed. */
//		mp_currentTerm->moveCursor(MOVE_UP, 1);
//		break;
//
//	case 'N':
//		/* select G2 set for next character only */
//		//setG2ForNextCharOnly();
//		//break;
//
//	case 'O':
//		/* select G3 set for next character only */
//		//setG3ForNextCharOnly();
//		printf("Unhandled upper case attr: %c\n", data[parseIndex].toLatin1() );
//		break;
//	default:
//		printf("Unknown upper case attr: %c\n", data[parseIndex].toLatin1() );
//	}
//}

////////////////////////////////////////////////////////////////////
int
Decoder::posOfFinalChar(const QString &data, int parseIndex, char finalChar)
{
	for (int i = parseIndex; i < data.length(); i++)
	{
		if (data[i] == finalChar)
			return i;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////
/* The <ESC>[ {Pn} ; {Pn} r 
 * is for setting the top and bottom margins that define a scrolling
 * region. The first parameter is the line number of the first line in
 * the scrolling region; the second parameter is the line number of the
 * bottom line of the scrolling region.  Default is the entire screen (no
 * margins).  The minimum region allowed is two lines, i.e., the top line
 * must be less than the bottom.  The cursor is placed in the home position */
void
Decoder::decodeLowerR_attr(const char *pAttr)
{
	char *pIter = const_cast<char*>(pAttr);
	if ( ! *pIter)
		return;

	int attrListSize = 0;
	splitStrings(pIter, ';', attrListSize);
	
   if (attrListSize == 2)
   {
      int firstLine = 0, lastLine = 0;
		if (mp_tempAttrs[0])
			firstLine = atoi( mp_tempAttrs[0]);
      if ( ! firstLine)
         return;
		if (mp_tempAttrs[1])
			lastLine = atoi( mp_tempAttrs[1]);
      if (! lastLine)
         return;

      mp_currentTerm->setScrollingRegion(firstLine, lastLine);
   }
   else
      printf("Scroll region attribute not handled: %s\n", pIter );
}

////////////////////////////////////////////////////////////////////
/* <ESC>[1A  move the cursor up by N lines. default = 1 
 * <ESC>[1B  move the cursor down by N lines. default = 1
 * <ESC>[1C  move the cursor right by N lines. default = 1
 * <ESC>[1D  move the cursor left by N lines. default = 1
 * <ESC>[1E  move the cursor Next line by N times. default = 1
 * <ESC>[1F  move the cursor previous line by N lines. default = 1*/
void
Decoder::decodeCapABCDEF_attr(const char* pAttr, MoveDirection md)
{
	if ( (! pAttr) || (! *pAttr) )
      mp_currentTerm->moveCursor(md, 1);
   else
   {
      int val = atoi(pAttr);
      if (val)
         mp_currentTerm->moveCursor(md, val);
      else
         printf("Bad 'move cursor': %s\n", pAttr);
   }
}

////////////////////////////////////////////////////////////////////
void
Decoder::decodeCapG_attr(const char* pAttr)
{
	int column = atoi(pAttr);
	if (column)
		mp_currentTerm->setCursorPosition( -1, column);	//Move to column, do not change rows
}

////////////////////////////////////////////////////////////////////
void
Decoder::decodeCapH_attr(const char* pAttr)
{
	int
		attrListSize = 0,
		line = 1,
		column = 1;
	char *pIter = const_cast<char*>(pAttr);
	if ( pIter)
		splitStrings(pIter, ';', attrListSize);

   if ( attrListSize)       //Default: ESC[H
	{
		if (attrListSize == 2)
		{
			if ( mp_tempAttrs[0] )
				line = atoi( mp_tempAttrs[0]);
			if ( ! line)
				return;
			if ( mp_tempAttrs[1] )
				column = atoi(mp_tempAttrs[1]);
			if (! column)
				return;
		}
		else
		{
			printf("Cursor position attribute not handled: %s\n", pIter);
			return;
		}
	}

	mp_currentTerm->setCursorPosition(line, column);
}

////////////////////////////////////////////////////////////////////
void 
Decoder::decodeCapJK_attr(const char* pAttr, bool isEraseInDisplay)
{
	int attrValue = 0;
	if ( pAttr )
		attrValue = atoi( pAttr);

	switch (attrValue)
	{
	case 0:
		if (isEraseInDisplay)
			mp_currentTerm->eraseRequest(ERASE_TO_END_OF_SCREEN, m_textFormat);
		else
			mp_currentTerm->eraseRequest(ERASE_TO_END_OF_LINE, m_textFormat);
		break;
	case 1:
		if (isEraseInDisplay)
			mp_currentTerm->eraseRequest(ERASE_TO_BEGINNING_OF_SCREEN, m_textFormat);
		else
			mp_currentTerm->eraseRequest(ERASE_TO_BEGINNING_OF_LINE, m_textFormat);
		break;
	case 2:
		if (isEraseInDisplay)
			mp_currentTerm->eraseRequest(ERASE_ENTIRE_SCREEN_NO_MOVE_CURSOR, m_textFormat);
		else
			mp_currentTerm->eraseRequest(ERASE_ENTIRE_LINE_NO_MOVE_CURSOR, m_textFormat);
		break;
	default:
		if (isEraseInDisplay)
			printf("Unhandled EraseInDisplay attribute (J) value: %d", attrValue);
		else
			printf("Unhandled EraseInLine attribute (K) value: %d", attrValue);
	}
}



////////////////////////////////////////////////////////////////////
/* CSI P s S 	Scroll up P s lines (default = 1) (SU) 	
 * CSI P s T 	Scroll down P s lines (default = 1) (SD) */
void
Decoder::decodeCapS_T_attr(const char finalChar, const QString &attr)
{
	int attrValue = 1;
	if (attr.length() )
	{
		bool ok;
		attrValue = attr.toInt(&ok);
		if ( !ok)
			attrValue = 1;
	}

	if (finalChar == 'S')
		mp_currentTerm->scroll(MOVE_UP, attrValue);
	else //finalChar == 'T'
		mp_currentTerm->scroll(MOVE_DOWN, attrValue);
}

////////////////////////////////////////////////////////////////////
/*	0	Reset all attributes
	1	Bright		//Let this be bold
	2	Dim			//Let this be the "dark" colors
	4	Underscore	
	5	Blink
	7	Reverse
	8	Hidden */
void
Decoder::setColorAttr(int colorNum)
{
	if (colorNum < 28)
	{
		switch (colorNum)
		{
		case 0:		//Must have got ESC[00m, ESC[m, ....
			m_textFormat.colorEffect = COLOR_EFFECT_DEFAULT;
			m_textFormat.FGColor = 0;
			m_textFormat.BGColor = 0;
			break;
		case 1:
			m_textFormat.colorEffect |= COLOR_EFFECT_BRIGHT;
			break;
		case 2:
			m_textFormat.colorEffect |= COLOR_EFFECT_DIM;
			break;
		case 4:
			m_textFormat.colorEffect |= COLOR_EFFECT_UNDERLINE;
			break;
		case 5:
			m_textFormat.blink = true;
			break;
		case 7:
			m_textFormat.colorEffect |= COLOR_EFFECT_REVERSE;
			break;
		case 8:		//We don't support the hidden attribute
			//m_textFormat.colorEffect |= COLOR_EFFECT_HIDDEN;
			break;
		case 24:
			m_textFormat.colorEffect &= COLOR_EFFECT_REMOVE_UNDERLINE;	//Turn OFF underlining
			break;
		case 25:
			m_textFormat.blink = false;
			break;
		case 27:
			m_textFormat.colorEffect &= COLOR_EFFECT_REMOVE_REVERSE;	//Turn OFF reversing
			break;
		default:
			printf("Unhandled color attribute: %d\n", colorNum);
			m_textFormat.colorEffect = COLOR_EFFECT_DEFAULT;
		}
	}
	else if (colorNum < 40)
		m_textFormat.FGColor = colorNum;
	else // ( (colorNum >= 40 && colorNum <=47) || colorNum == 49)
		m_textFormat.BGColor = colorNum;

	//Make sure anything that uses this console character is labeled as dirty
	m_textFormat.colorEffect |= COLOR_EFFECT_CHAR_IS_DIRTY;
}


////////////////////////////////////////////////////////////////////
