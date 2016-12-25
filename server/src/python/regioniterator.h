/*
 *     Wolfpack Emu (WP)
 * UO Server Emulation Program
 *
 * Copyright 2001-2016 by holders identified in AUTHORS.txt
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Palace - Suite 330, Boston, MA 02111-1307, USA.
 *
 * In addition to that license, if you are running this program or modified
 * versions of it on a public system you HAVE TO make the complete source of
 * the version used by you available or provide people with a location to
 * download it.
 *
 * Wolfpack Homepage: http://developer.berlios.de/projects/wolfpack/
 */

#ifndef __REGIONITERATOR_H__
#define __REGIONITERATOR_H__

#include "utilities.h"
#include "../defines.h"
#include "../mapobjects.h"

/*
	\object itemregioniterator
	\description This object type allows you to iterate over a set of items in a memory efficient way.
	A typical iteration could look like this:

	<code>item = iterator.first
	while item:
	&nbsp;&nbsp;# Access item properties here
	&nbsp;&nbsp;item = iterator.next</code>
*/
typedef struct
{
	PyObject_HEAD;
	MapItemsIterator iter;
} wpRegionIteratorItems;

static void wpItemIteratorDealloc( PyObject* self )
{
	reinterpret_cast<wpRegionIteratorItems*>( self )->iter.~MapItemsIterator();
	PyObject_Del( self );
}

static PyObject* wpRegionIteratorItems_getAttr( wpRegionIteratorItems* self, char* name )
{
	/*
		\rproperty itemregioniterator.first Accessing this property will reset the iterator to the first item and return it.
		If there are no items to iterate over, None is returned.
	*/
	if ( !strcmp( name, "first" ) )
		return PyGetItemObject( self->iter.first() );
	/*
		\rproperty itemregioniterator.next Accessing this property will advance the iterator to the next item and return it.
		At the end of the iteration, None is returned.
	*/
	else if ( !strcmp( name, "next" ) )
		return PyGetItemObject( self->iter.next() );

	Py_RETURN_FALSE;
}

static PyTypeObject wpRegionIteratorItemsType =
{
PyObject_HEAD_INIT( NULL )
0,
"wpItemRegionIterator",
sizeof( wpRegionIteratorItemsType ),
0,
wpItemIteratorDealloc,
0,
( getattrfunc ) wpRegionIteratorItems_getAttr,
0,
0,
0,
0,
0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static PyObject* PyGetItemRegionIterator( unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char map )
{
	// must manually initialize the iterator's state to zero
	wpRegionIteratorItems* returnVal = PyObject_New( wpRegionIteratorItems, &wpRegionIteratorItemsType );
	*reinterpret_cast<void**>( &returnVal->iter ) = NULL;

	returnVal->iter = MapObjects::instance()->listItemsInRect( map, x1, y1, x2, y2 );

	return reinterpret_cast<PyObject*>( returnVal );
}

// Never used? - BtbN
/* static PyObject* PyGetItemRegionIterator( unsigned short xBlock, unsigned short yBlock, unsigned char map )
{
	// must manually initialize the iterator's state to zero
	wpRegionIteratorItems* returnVal = PyObject_New( wpRegionIteratorItems, &wpRegionIteratorItemsType );
	*reinterpret_cast<void**>( &returnVal->iter ) = NULL;

	returnVal->iter = MapObjects::instance()->listItemsInBlock( map, xBlock * 8, yBlock * 8 );

	return reinterpret_cast<PyObject*>( returnVal );
} */

/*
 *	Character Region Iterator
 */
/*
	\object charregioniterator
	\description This object type allows you to iterate over a set of chars in a memory efficient way.
	A typical iteration could look like this:

	<code>char = iterator.first
	while char:
	&nbsp;&nbsp;# Access char properties here
	&nbsp;&nbsp;char = iterator.next</code>
*/
typedef struct
{
	PyObject_HEAD;
	MapCharsIterator iter;
} wpRegionIteratorChars;

static void wpCharIteratorDealloc( PyObject* self )
{
	reinterpret_cast<wpRegionIteratorChars*>( self )->iter.~MapCharsIterator();
	PyObject_Del( self );
}

static PyObject* wpRegionIteratorChars_getAttr( wpRegionIteratorChars* self, char* name )
{
	/*
		\rproperty charregioniterator.first Accessing this property will reset the iterator to the first character and return it.
		If there are no characters to iterate over, None is returned.
	*/
	if ( !strcmp( name, "first" ) )
		return PyGetCharObject( self->iter.first() );
	/*
		\rproperty charregioniterator.next Accessing this property will advance the iterator to the next character and return it.
		At the end of the iteration, None is returned.
	*/
	else if ( !strcmp( name, "next" ) )
		return PyGetCharObject( self->iter.next() );

	Py_RETURN_FALSE;
}

static PyTypeObject wpRegionIteratorCharsType =
{
PyObject_HEAD_INIT( NULL )
0,
"wpCharRegionIterator",
sizeof( wpRegionIteratorCharsType ),
0,
wpCharIteratorDealloc,
0,
( getattrfunc ) wpRegionIteratorChars_getAttr,
0,
0,
0,
0,
0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static PyObject* PyGetCharRegionIterator( unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char map, bool offline = false )
{
	// must manually initialize the iterator's state to zero
	wpRegionIteratorChars* returnVal = PyObject_New( wpRegionIteratorChars, &wpRegionIteratorCharsType );
	*reinterpret_cast<void**>( &returnVal->iter ) = NULL;

	returnVal->iter = MapObjects::instance()->listCharsInRect( map, x1, y1, x2, y2, offline );

	return reinterpret_cast<PyObject*>( returnVal );
}

// Never used? - BtbN
/* static PyObject* PyGetCharRegionIterator( unsigned short xBlock, unsigned short yBlock, unsigned char map, bool offline = false )
{
	// must manually initialize the iterator's state to zero
	wpRegionIteratorChars* returnVal = PyObject_New( wpRegionIteratorChars, &wpRegionIteratorCharsType );
	*reinterpret_cast<void**>( &returnVal->iter ) = NULL;

	returnVal->iter = MapObjects::instance()->listCharsInBlock( map, xBlock * 8, yBlock * 8, offline );

	return reinterpret_cast<PyObject*>( returnVal );
} */

#endif
