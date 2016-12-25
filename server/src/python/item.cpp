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

#include "engine.h"

#include "../items.h"
#include "../muls/tilecache.h"
#include "../scriptmanager.h"
#include "../multi.h"
#include "../basechar.h"
#include "../singleton.h"

#include "utilities.h"
#include "pycontent.h"
#include "tempeffect.h"
#include "objectcache.h"

/*
	\object item
	\inherit object
	\description This object represents an item in the wolfpack world.
*/
struct wpItem
{
	PyObject_HEAD;
	P_ITEM pItem;
};

// Note: Must be of a different type to cause more then 1 template instanciation
class cItemObjectCache : public cObjectCache<wpItem, 50>
{
};

typedef Singleton<cItemObjectCache> ItemCache;

// Forward Declarations
static PyObject* wpItem_getAttr( wpItem* self, char* name );
static int wpItem_setAttr( wpItem* self, char* name, PyObject* value );
int wpItem_compare( PyObject* a, PyObject* b );
long wpItem_hash( wpItem* self )
{
	return self->pItem->serial();
}

// Return a string representation for an item object.
static PyObject* wpItem_str( wpItem* object )
{
	return PyString_FromFormat( "0x%x", object->pItem->serial() );
}

/*!
	The typedef for Wolfpack Python items
*/
static PyTypeObject wpItemType =
{
PyObject_HEAD_INIT( &PyType_Type )
0,
"wpitem",
sizeof( wpItemType ),
0,
wpDealloc,
0,
( getattrfunc ) wpItem_getAttr,
( setattrfunc ) wpItem_setAttr,
wpItem_compare,
0,
0,
0,
0,
( hashfunc ) wpItem_hash,
0,
( reprfunc ) wpItem_str,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,0,0,0,0,0,0,0,0,
};

PyObject* PyGetItemObject( P_ITEM item )
{
	if ( item == NULL )
	{
		Py_RETURN_NONE;
	}
	else
	{
		//	wpItem *returnVal = ItemCache::instance()->allocObj( &wpItemType );
		wpItem* returnVal = PyObject_New( wpItem, &wpItemType );
		returnVal->pItem = item;
		return ( PyObject * ) returnVal;
	}
}

// Method declarations

/*!
	Resends the item to all clients in range
*/
/*
	\method item.update
	\description Resend the item.
*/
static PyObject* wpItem_update( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	self->pItem->update();

	Py_RETURN_NONE;
}

/*!
	Removes the item
*/
/*
	\method item.delete
	\description Deletes the item.
*/
static PyObject* wpItem_delete( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	self->pItem->remove();

	Py_RETURN_NONE;
}

/*!
	Moves the item to the specified location
*/
/*
	\method item.moveto
	\description Change the position of this item.
	\param pos The coord object representing the new position.
*/
/*
	\method item.moveto
	\description Change the position of this item.
	\param x The new x coordinate of this item.
	\param y The new y coordinate of this item.
	\param z Defaults to the current z position of the item.
	The new z coordinate of this item.
	\param map Defaults to the current map the item is on.
	The new map coordinate of this item.
*/
static PyObject* wpItem_moveto( wpItem* self, PyObject* args )
{
	// Gather parameters
	Coord pos = self->pItem->pos();
	uchar noRemove = 1; // otherwise it NEVER gets moved to the surface

	if ( PyTuple_Size( args ) == 1 )
	{
		if ( !PyArg_ParseTuple( args, "O&|b:item.moveto(coord, [noremove=1])", &PyConvertCoord, &pos, &noRemove ) )
		{
			return 0;
		}

		if (pos.isInternalMap()) {
			PyErr_SetString( PyExc_RuntimeError, "Moving to the internal map using item.moveto() is not supported." );
			return 0;
		}

		self->pItem->moveTo( pos );
	}
	else
	{
		int z = 0;
		if ( !PyArg_ParseTuple( args, "HH|iBB:item.moveto(x, y, [z], [map])", &pos.x, &pos.y, &z, &pos.map, &noRemove ) )
		{
			return 0;
		}
		pos.z = ( signed char ) z;

		if (pos.isInternalMap()) {
			PyErr_SetString( PyExc_RuntimeError, "Moving to the internal map using item.moveto() is not supported." );
			return 0;
		}

		self->pItem->moveTo( pos );
	}

	Py_RETURN_NONE;
}

/*
	\method item.removefromview
	\description Remove the item from all clients who can currently see it.
*/
static PyObject* wpItem_removefromview( wpItem* self, PyObject* args )
{
	int k = 1;
	if ( !PyArg_ParseTuple( args, "|i:item.removefromview( clean )", &k ) )
		return 0;
	self->pItem->removeFromView( k != 0 ? true : false );
	Py_RETURN_NONE;
}

/*!
	Plays a soundeffect originating from the item
*/
/*
	\method item.soundeffect
	\description Play a soundeffect originating from the item.
	\param sound The id of the soundeffect.
*/
static PyObject* wpItem_soundeffect( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( PyTuple_Size( args ) < 1 || !PyInt_Check( PyTuple_GetItem( args, 0 ) ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	self->pItem->soundEffect( PyInt_AsLong( PyTuple_GetItem( args, 0 ) ) );

	Py_RETURN_NONE;
}

/*!
	Returns the distance towards a given object or position
*/
/*
	\method item.distanceto
	\description Measure the distance between the item and another object.
	\param object The target object. May be another character, item or a coord
	object.
	\return The distance in tiles towards the given target.
*/
/*
	\method item.distanceto
	\description Measure the distance between the item and a coordinate.
	\param x The x component of the target coordinate.
	\param y The y component of the target coordinate.
	\return The distance in tiles towards the given coordinate.
*/
static PyObject* wpItem_distanceto( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		return PyInt_FromLong( -1 );

	// Probably an object
	if ( PyTuple_Size( args ) == 1 )
	{
		PyObject* pObj = PyTuple_GetItem( args, 0 );

		if ( checkWpCoord( PyTuple_GetItem( args, 0 ) ) )
			return PyInt_FromLong( self->pItem->pos().distance( getWpCoord( pObj ) ) );

		// Item
		P_ITEM pItem = getWpItem( pObj );
		if ( pItem )
			return PyInt_FromLong( pItem->dist( self->pItem ) );

		P_CHAR pChar = getWpChar( pObj );
		if ( pChar )
			return PyInt_FromLong( pChar->dist( self->pItem ) );
	}
	else if ( PyTuple_Size( args ) >= 2 ) // Min 2
	{
		Coord pos = self->pItem->pos();

		if ( !PyInt_Check( PyTuple_GetItem( args, 0 ) ) || !PyInt_Check( PyTuple_GetItem( args, 1 ) ) )
			return PyInt_FromLong( -1 );

		pos.x = PyInt_AsLong( PyTuple_GetItem( args, 0 ) );
		pos.y = PyInt_AsLong( PyTuple_GetItem( args, 1 ) );

		return PyInt_FromLong( self->pItem->pos().distance( pos ) );
	}

	PyErr_BadArgument();
	return NULL;
}

/*
	\method item.useresource
	\description Consumes a given amount of a resource.
	\param amount The amount to consume.
	\param itemid The item id of the object to consume.
	\param color Defaults to 0.
	The color of the object to consume
	\return How many items have not been deleted.
*/
static PyObject* wpItem_useresource( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( PyTuple_Size( args ) < 2 || !PyInt_Check( PyTuple_GetItem( args, 0 ) ) || !PyInt_Check( PyTuple_GetItem( args, 1 ) ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	quint32 amount = PyInt_AsLong( PyTuple_GetItem( args, 0 ) );
	quint16 id = PyInt_AsLong( PyTuple_GetItem( args, 1 ) );
	quint16 color = 0;

	if ( PyTuple_Size( args ) > 2 && PyInt_Check( PyTuple_GetItem( args, 2 ) ) )
		color = PyInt_AsLong( PyTuple_GetItem( args, 2 ) );

	quint32 deleted = 0;
	deleted = self->pItem->deleteAmount( amount, id, color );

	return PyInt_FromLong( deleted );
}

/*
	\method item.countresource
	\description Returns the amount of a given resource.
	\param itemid The item id of the resource to count.
	\param color Defaults to 0.
	The color of the items to count.
	\return The amount of item-id
*/
static PyObject* wpItem_countresource( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( PyTuple_Size( args ) < 1 || !PyInt_Check( PyTuple_GetItem( args, 0 ) ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	quint16 id = PyInt_AsLong( PyTuple_GetItem( args, 0 ) );
	qint16 color = -1;

	if ( PyTuple_Size( args ) > 1 && PyInt_Check( PyTuple_GetItem( args, 1 ) ) )
		color = PyInt_AsLong( PyTuple_GetItem( args, 1 ) );

	unsigned int avail = 0;
	avail = self->pItem->countItems( id, color );

	return PyLong_FromLong( avail );
}

/*
	\method item.gettag
	\description Returns the value of a custom tag. Three types of tag types: String, Int and Float.
	\param name The name of the tag.
	\return Returns the value of the given tag name.
*/
static PyObject* wpItem_gettag( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
	{
		Py_RETURN_NONE;
	}

	if ( PyTuple_Size( args ) < 1 || !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	QString key = PyString_AsString( PyTuple_GetItem( args, 0 ) );
	cVariant value = self->pItem->getTag( key );

	switch( value.type() )
	{
	case cVariant::StringType:	return QString2Python( value.toString() );
	case cVariant::IntType:		return PyInt_FromLong( value.asInt() );
	case cVariant::DoubleType:	return PyFloat_FromDouble( value.asDouble() );
	default: break;
	}
	Py_RETURN_NONE;
}

/*!
	Sets a custom tag
*/
/*
	\method item.settag
	\description Set a value for the given tag name. Three value types: String, Int and Float.
	\param name The name of the tag
	\param value The value of the tag.
*/
static PyObject* wpItem_settag( wpItem* self, PyObject* args )
{
	if ( self->pItem->free )
		Py_RETURN_FALSE;

	char* key;
	PyObject* object;

	if ( !PyArg_ParseTuple( args, "sO:item.settag( name, value )", &key, &object ) )
		return 0;

	if ( PyString_Check( object ) || PyUnicode_Check( object ) )
	{
		self->pItem->setTag( key, cVariant( boost::python::extract<QString>( object ) ) );
	}
	else if ( PyInt_Check( object ) )
	{
		self->pItem->setTag( key, cVariant( ( int ) PyInt_AsLong( object ) ) );
	}
	else if ( PyLong_Check( object ) )
	{
		self->pItem->setTag( key, cVariant( ( int ) PyLong_AsLong( object ) ) );
	}
	else if ( PyFloat_Check( object ) )
	{
		self->pItem->setTag( key, cVariant( ( double ) PyFloat_AsDouble( object ) ) );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "You passed an unknown object type to char.settag." );
		return 0;
	}

	Py_RETURN_NONE;
}

/*!
	Checks if a certain tag exists
*/
/*
	\method item.hastag
	\description Returns the given tag name exists.
	\param name
	\return Returns true or false if the tag exists.
*/
static PyObject* wpItem_hastag( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	char* pKey = 0;
	if ( !PyArg_ParseTuple( args, "s:item.hastag( key )", &pKey ) )
		return 0;

	if ( self->pItem->hasTag( pKey ) )
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

/*!
	Deletes a given tag
*/
/*
	\method item.deltag
	\description Deletes the tag under a given name.
	\param name The name of the tag that should be deleted.
*/
static PyObject* wpItem_deltag( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	QString key = PyString_AsString( PyTuple_GetItem( args, 0 ) );
	self->pItem->removeTag( key );

	Py_RETURN_NONE;
}
/*
	\method item.ischar
	\description Returns whether the item is a character.
	\return Returns true or false.
*/
static PyObject* wpItem_ischar( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	Q_UNUSED( self );
	Py_RETURN_FALSE;
}
/*
	\method item.isitem
	\description Returns whether the item is an item.
	\return Returns true or false.
*/
static PyObject* wpItem_isitem( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	Q_UNUSED( self );
	Py_RETURN_TRUE;
}

/*!
	Shows a moving effect moving toward a given object or coordinate.
*/
/*
	\method item.movingeffect
	\description Shows a moving effect moving toward a given object or coordinate.
	\param id The id of the moving object.
	\param target Can be an item, character or pos.
	\param fixedDirection Set a fixed direction of the moving id.
	\param explodes True or false for exploding at the end, no damage.
	\param speed Speed at which the id moves. Default is 10.
	\param hue Hue of the moving id
	\param renderMode Unknown
*/
static PyObject* wpItem_movingeffect( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( ( !checkArgObject( 1 ) && !checkArgCoord( 1 ) ) || !checkArgInt( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	quint16 id = getArgInt( 0 );

	cUObject* object = getArgChar( 1 );
	if ( !object )
		object = getArgItem( 1 );

	Coord pos;

	if ( checkArgCoord( 1 ) )
		pos = getArgCoord( 1 );

	// Optional Arguments
	bool fixedDirection = true;
	bool explodes = false;
	quint8 speed = 10;
	quint16 hue = 0;
	quint16 renderMode = 0;

	if ( checkArgInt( 2 ) )
		fixedDirection = getArgInt( 2 ) != 0;

	if ( checkArgInt( 3 ) )
		explodes = getArgInt( 3 ) != 0;

	if ( checkArgInt( 4 ) )
		speed = getArgInt( 4 );

	if ( checkArgInt( 5 ) )
		hue = getArgInt( 5 );

	if ( checkArgInt( 6 ) )
		renderMode = getArgInt( 6 );

	if ( object )
		self->pItem->effect( id, object, fixedDirection, explodes, speed, hue, renderMode );
	else
		self->pItem->effect( id, pos, fixedDirection, explodes, speed, hue, renderMode );

	Py_RETURN_NONE;
}

/*!
	Adds a temp effect to this item.
*/
/*
	\method item.addtimer
	\description Set a delayed timer for a script function to execute.
	\param expiretime Timer interval, in milliseconds.
	\param expirecallback The function that should be called
	when the effect expires. The prototype for this function is:
	<code>def expire_callback(item, args):
		&nbsp;&nbsp;pass</code>
	Item is the item the effect was applied to. Args is the list of custom arguments you passed to addtimer.
	\param arguments A list of arguments that should be passed on to the effect.
		Please note that you should only pass on strings, integers and floats because
		they are the only objects that can be saved to the worldfile. If you want to
		pass on items or characters, please pass the serial instead and use the
		findchar and finditem functions in the wolfpack library.
	\param serializable Defaults to false.
		If this is true, the effect will be saved to the worldfile. Otherwise the effect will be lost when the server is
		shutdown or crashes.
	\return Returns None.
*/
static PyObject* wpItem_addtimer( wpItem* self, PyObject* args )
{
	quint32 expiretime;
	PyObject* function;
	PyObject* arguments;
	uchar persistent = 0;

	if ( !PyArg_ParseTuple( args, "iOO!|B:item.addtimer", &expiretime, &function, &PyList_Type, &arguments, &persistent ) )
		return 0;

	PythonFunction* toCall = 0;
	if ( !PyCallable_Check( function ) )
	{
		QString func = boost::python::extract<QString>( function );
		if ( func.isNull() )
		{
			PyErr_SetString( PyExc_TypeError, "Bad argument on addtimer callback type" );
			return 0;
		}
		Console::instance()->log( LOG_WARNING, tr("Using deprecated string as callback identifier [%1]").arg(func) );
		toCall = new PythonFunction( func );

		if ( !toCall->isValid() )
		{
			PyErr_Format(PyExc_RuntimeError, "The function callback you specified was invalid: %s.", func.toLatin1().constData());
			return 0;
		}
	}
	else
		toCall = new PythonFunction( function );

	if ( !toCall->isValid() )
	{
		PyErr_SetString(PyExc_RuntimeError, "The function callback you specified was invalid.");
		return 0;
	}

	PyObject* py_args = PyList_AsTuple( arguments );
	cPythonEffect* effect = new cPythonEffect( toCall, py_args );
	Py_DECREF(py_args);

	// Should we save this effect?
	effect->setSerializable( persistent != 0 );

	effect->setDest( self->pItem->serial() );
	effect->setExpiretime_ms( expiretime );
	Timers::instance()->insert( effect );

	Py_RETURN_NONE;
}

/*!
	Gets the outmost item this item is contained in.
*/
/*
	\method item.getoutmostitem
	\description Returns the outmost item this item is contained in.
*/
static PyObject* wpItem_getoutmostitem( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	return PyGetItemObject( self->pItem->getOutmostItem() );
}

/*!
	Gets the outmost character this item is contained in.
*/
/*
	\method item.getoutmostchar
	\description Returns the outmost character this item is contained in.
*/
static PyObject* wpItem_getoutmostchar( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	return PyGetCharObject( self->pItem->getOutmostChar() );
}

/*!
	Returns the item's name
*/
/*
	\method item.getname
	\description Returns the name of the object.
	\return Returns the object's name.
*/
static PyObject* wpItem_getname( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem )
		Py_RETURN_FALSE;

	return QString2Python( self->pItem->getName( true ) );
}

/*!
	Adds an item to this container.
*/
/*
	\method item.additem
	\description Adds an item to the container.
	\param item Item to add.
	\param randomPos Gives the item a random position in the pack. Defaults to true
	\param handleWeight Defaults to true.
	\param autostack Autostacks the item. Defaults to true
*/
static PyObject* wpItem_additem( wpItem* self, PyObject* args )
{
	if ( !self->pItem || self->pItem->free )
		Py_RETURN_FALSE;

	if ( !checkArgItem( 0 ) )
	{
		PyErr_BadArgument();
		return 0;
	}

	P_ITEM pItem = getArgItem( 0 );

	if ( pItem->free )
		Py_RETURN_FALSE;

	// Secondary Parameters
	bool randomPos = true;
	bool handleWeight = true;
	bool autoStack = true;

	if ( checkArgInt( 1 ) )
		randomPos = getArgInt( 1 ) != 0;

	if ( checkArgInt( 2 ) )
		handleWeight = getArgInt( 2 ) != 0;

	if ( checkArgInt( 3 ) )
		autoStack = getArgInt( 3 ) != 0;

	self->pItem->addItem( pItem, randomPos, handleWeight, false, autoStack );

	Py_RETURN_NONE;
}

/*!
	Amount of items inside this container
*/
/*
	\method item.countitem
	\return Returns the amount of items in a container.
*/
static PyObject* wpItem_countItem( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( !self->pItem || self->pItem->free )
	{
		PyErr_BadArgument();
		return 0;
	}

	return PyInt_FromLong( self->pItem->content().count() );
}

/*
	\method item.lightning
	\description Zaps the object with a lightning bolt!
	\param hue The hue value of the lightning.
*/
static PyObject* wpItem_lightning( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	unsigned short hue = 0;

	if ( !PyArg_ParseTuple( args, "|h:item.lightning( [hue] )", &hue ) )
		return 0;

	self->pItem->lightning( hue );

	Py_RETURN_NONE;
}

/*
	\method item.resendtooltip
	\description Resends the tooltip of the object.
*/
static PyObject* wpItem_resendtooltip( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );

	if ( !self->pItem->free )
		self->pItem->resendTooltip();

	Py_RETURN_NONE;
}

/*
	\method item.dupe
	\description Creatues a dupe of the item
*/
static PyObject* wpItem_dupe( wpItem* self, PyObject* args )
{
	bool dupeContent = false;

	if ( checkArgInt( 0 ) )
		dupeContent = getArgInt( 0 ) != 0;

	if ( !self->pItem->free )
	{
		P_ITEM item = self->pItem->dupe( dupeContent );
		return item->getPyObject();
	}

	Py_RETURN_NONE;
}

/*
	\method item.isblessed
	\description Determines if an item is blessed or not.
	\return True, False
*/
static PyObject* wpItem_isblessed( wpItem* self, PyObject* args )
{
	Q_UNUSED( args );
	if ( self->pItem->free )
	{
		return 0;
	}
	return self->pItem->newbie() ? PyTrue() : PyFalse();
}

/*
	\method item.canstack
	\description Determines if the item is stackable.
	\return True, False
*/
static PyObject* wpItem_canstack( wpItem* self, PyObject* args )
{
	P_ITEM other;
	if ( !PyArg_ParseTuple( args, "O&:item.canstack(other)", &PyConvertItem, &other ) )
	{
		return 0;
	}

	return self->pItem->canStack( other ) ? PyTrue() : PyFalse();
}

/*
	\method item.countitems
	\description Counts items recursively in a container by matching their baseids
	against a given list of strings.
	\param baseids A list of strings the baseids of all items will be matched against.
	\return Amount of matching items.
*/
static PyObject* wpItem_countitems( wpItem* self, PyObject* args )
{
	PyObject* list;
	if ( !PyArg_ParseTuple( args, "O!:item.countitems(baseids)", &PyList_Type, &list ) )
	{
		return 0;
	}

	QStringList baseids;

	for ( int i = 0; i < PyList_Size( list ); ++i )
	{
		PyObject* item = PyList_GetItem( list, i );
		baseids.append( boost::python::extract<QString>( item ) );
	}

	return PyInt_FromLong( self->pItem->countItems( baseids ) );
}

/*
	\method item.removeitems
	\description Removes items recursively from a container by matching their baseids
	against a given list of strings.
	\param baseids A list of strings the baseids of the found items will be matched against.
	\param amount Amount of items to remove.
	\return The amount of items that still would need to be removed. If the requested amount could be
	statisfied, the return value is 0.
*/
static PyObject* wpItem_removeitems( wpItem* self, PyObject* args )
{
	PyObject* list;
	unsigned int amount;
	if ( !PyArg_ParseTuple( args, "O!I:item.removeitems(baseids, amount)", &PyList_Type, &list, &amount ) )
	{
		return 0;
	}

	QStringList baseids;

	for ( int i = 0; i < PyList_Size( list ); ++i )
	{
		PyObject* item = PyList_GetItem( list, i );
		baseids.append( boost::python::extract<QString>( item ) );
	}

	return PyInt_FromLong( self->pItem->removeItems( baseids, amount ) );
}

/*
	\method item.removescript
	\description Remove a python script from the script chain for this object.
	\param script The id of the python script you want to remove from the script chain.
*/
static PyObject* wpItem_removescript( wpItem* self, PyObject* args )
{
	char* script;
	if ( !PyArg_ParseTuple( args, "s:item.removescript(name)", &script ) )
	{
		return 0;
	}
	self->pItem->removeScript( script );
	Py_RETURN_NONE;
}

/*
	\method item.addscript
	\description Add a pythonscript to the script chain of this object.
	Does nothing if the object already has that script.
	\param script The id of the python script you want to add to the script chain.
*/
static PyObject* wpItem_addscript( wpItem* self, PyObject* args )
{
	char* script;
	if ( !PyArg_ParseTuple( args, "s:item.addscript(name)", &script ) )
	{
		return 0;
	}

	cPythonScript* pscript = ScriptManager::instance()->find( script );

	if ( !pscript )
	{
		PyErr_Format( PyExc_RuntimeError, "No such script: %s", script );
		return 0;
	}

	self->pItem->addScript( pscript );
	Py_RETURN_NONE;
}

/*
	\method item.hasscript
	\description Check if this object has a python script in its script chain.
	\param script The id of the python script you are looking for.
	\return True of the script is in the chain. False otherwise.
*/
static PyObject* wpItem_hasscript( wpItem* self, PyObject* args )
{
	char* script;
	if ( !PyArg_ParseTuple( args, "s:item.hasscript(name)", &script ) )
	{
		return 0;
	}

	if ( self->pItem->hasScript( script ) )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

/*
	\method item.say
	\description Let the item say a text visible for everyone in range.
	\param text The text the item should say.
	\param color Defaults to 0x3b2.
	The color for the text.
*/
/*
	\method item.say
	\description Let the item say a localized text message.
	\param clilocid The id of the localizd message the item should say.
	\param params Defaults to an empty string.
	The parameters that should be parsed into the localized message.
	\param affix Defaults to an empty string.
	Text that should be appended or prepended (see the prepend parameter) to the
	localized message.
	\param prepend Defaults to false.
	If this boolean parameter is set to true, the affix is prepended rather than
	appended.
	\param color Defaults to 0x3b2.
	The color of the message.
	\param socket Defaults to None.
	If a socket object is given here, the message will only be seen by the given socket.
*/
static PyObject* wpItem_say( wpItem* self, PyObject* args, PyObject* keywds )
{
	if ( !checkArgStr( 0 ) )
	{
		uint id;
		char* clilocargs = 0;
		char* affix = 0;
		char prepend;
		uint color = 0x3b2;
		cUOSocket* socket = 0;

		static char* kwlist[] =
		{
		"clilocid",
		"args",
		"affix",
		"prepend",
		"color",
		"socket",
		NULL
		};

		if ( !PyArg_ParseTupleAndKeywords( args, keywds, "i|ssbiO&:char.say( clilocid, [args], [affix], [prepend], [color], [socket] )", kwlist, &id, &clilocargs, &affix, &prepend, &color, &PyConvertSocket, &socket ) )
			return 0;

		self->pItem->talk( id, clilocargs, affix, prepend, color, socket );
	}
	else
	{
		ushort color = 0x3b2;

		if ( checkArgInt( 1 ) )
			color = getArgInt( 1 );

		self->pItem->talk( getArgStr( 0 ), color );
	}

	Py_RETURN_NONE;
}

/*
	\method item.callevent
	\description Call a python event chain for this object. Ignore global hooks.
	\param event The id of the event you want to call. See <library id="wolfpack.consts">wolfpack.consts</library> for constants.
	\param args A tuple of arguments you want to pass to this event handler.
	\return The result of the first handling event.
*/
static PyObject* wpItem_callevent( wpItem* self, PyObject* args )
{
	unsigned int event;
	PyObject* eventargs;

	if ( !PyArg_ParseTuple( args, "IO!:item.callevent(event, args)", &event, &PyTuple_Type, &eventargs ) )
	{
		return 0;
	}

	PyObject *result = self->pItem->callEvent( ( ePythonEvent ) event, eventargs );

	if ( !result )
	{
		result = Py_None;
		Py_INCREF( result );
	}

	return result;
}

/*
	\method item.effect
	\description Show an effect that moves along with the item.
	\param id The effect item id.
	\param speed Defaults to 5.
	This is the animation speed of the effect.
	\param duration Defaults to 10.
	This is how long the effect should be visible.
	\param hue Defaults to 0.
	This is the color for the effect.
	\param rendermode Defaults to 0.
	This is a special rendermode for the effect.
	Valid values are unknown.
*/
static PyObject* wpItem_effect( wpItem* self, PyObject* args )
{
	quint16 id;
	// Optional Arguments
	quint8 speed = 5;
	quint8 duration = 10;
	quint16 hue = 0;
	quint16 renderMode = 0;

	if ( !PyArg_ParseTuple( args, "H|BBHH:char.effect(id, [speed], [duration], [hue], [rendermode])", &id, &speed, &duration, &hue, &renderMode ) )
	{
		return 0;
	}

	self->pItem->pos().effect( id, speed, duration, hue, renderMode );

	Py_RETURN_NONE;
}

/*
	\method item.getintproperty
	\description Get an integer property from the items definition.
	\param name The name of the property. This name is not case sensitive.
	\param default The default value that is returned if this property doesnt
	exist. Defaults to 0.
	\return The property value or the given default value.
*/
static PyObject* wpItem_getintproperty( wpItem* self, PyObject* args )
{
	unsigned int def = 0;
	PyObject *pyname;

	if ( !PyArg_ParseTuple( args, "O|i:item.getintproperty(name, def)", &pyname, &def ) )
	{
		return 0;
	}

	QString name = boost::python::extract<QString>( pyname );
	if ( self->pItem->basedef() )
	{
		return PyInt_FromLong( self->pItem->basedef()->getIntProperty( name, def ) );
	}
	else
	{
		return PyInt_FromLong( def );
	}
}

/*
	\method item.getstrproperty
	\description Get a string property from the items definition.
	\param name The name of the property. This name is not case sensitive.
	\param default The default value that is returned if this property doesnt
	exist. Defaults to an empty string.
	\return The property value or the given default value.
*/
static PyObject* wpItem_getstrproperty( wpItem* self, PyObject* args )
{
	PyObject *pydef = Py_None;
	PyObject *pyname;

	if ( !PyArg_ParseTuple( args, "O|O:item.getstrproperty(name, def)", &pyname, &pydef ) )
	{
		return 0;
	}

	QString name = boost::python::extract<QString>( pyname );

	QString def = QString::null;
	if ( pydef != Py_None )
	{
		def = boost::python::extract<QString>( pydef );
	}

	if ( self->pItem->basedef() )
	{
		return QString2Python( self->pItem->basedef()->getStrProperty( name, def ) );
	}
	else
	{
		if ( pydef != Py_None )
		{
			Py_INCREF( pydef );
			return pydef;
		}
		else
		{
			return QString2Python( "" );
		}
	}
}

/*
	\method item.hasstrproperty
	\description Checks if the definition of this item has a string property with the given name.
	\param name The name of the property. This name is not case sensitive.
	\return True if the definition has the property, False otherwise.
*/
static PyObject* wpItem_hasstrproperty( wpItem* self, PyObject* args )
{
	PyObject *pyname;

	if ( !PyArg_ParseTuple( args, "O:item.hasstrproperty(name)", &pyname ) )
	{
		return 0;
	}

	QString name = boost::python::extract<QString>( pyname );

	if ( self->pItem->basedef() && self->pItem->basedef()->hasStrProperty( name ) )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

/*
	\method item.hasintproperty
	\description Checks if the definition of this item has an integer property with the given name.
	\param name The name of the property. This name is not case sensitive.
	\return True if the definition has the property, False otherwise.
*/
static PyObject* wpItem_hasintproperty( wpItem* self, PyObject* args )
{
	PyObject *pyname;

	if ( !PyArg_ParseTuple( args, "O:item.hasintproperty(name)", &pyname ) )
	{
		return 0;
	}

	QString name = boost::python::extract<QString>( pyname );

	if ( self->pItem->basedef() && self->pItem->basedef()->hasIntProperty( name ) )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

static PyMethodDef wpItemMethods[] =
{
{ "additem",			( getattrofunc ) wpItem_additem, METH_VARARGS, "Adds an item to this container." },
{ "countitem",			( getattrofunc ) wpItem_countItem, METH_VARARGS, "Counts how many items are inside this container." },
{ "countitems",			( getattrofunc ) wpItem_countitems, METH_VARARGS, "Counts the items inside of this container based on a list of baseids." },
{ "removeitems",		( getattrofunc ) wpItem_removeitems, METH_VARARGS, "Removes items inside of this container based on a list of baseids." },
{ "update",				( getattrofunc ) wpItem_update, METH_VARARGS, "Sends the item to all clients in range." },
{ "removefromview",		( getattrofunc ) wpItem_removefromview, METH_VARARGS, "Removes the item from the view of all in-range clients." },
{ "delete",				( getattrofunc ) wpItem_delete, METH_VARARGS, "Deletes the item and the underlying reference." },
{ "moveto",				( getattrofunc ) wpItem_moveto, METH_VARARGS, "Moves the item to the specified location." },
{ "soundeffect",		( getattrofunc ) wpItem_soundeffect, METH_VARARGS, "Sends a soundeffect to the surrounding sockets." },
{ "distanceto",			( getattrofunc ) wpItem_distanceto, METH_VARARGS, "Distance to another object or a given position." },
{ "canstack",			( getattrofunc ) wpItem_canstack, METH_VARARGS, "Sees if the item can be stacked on another item." },
{ "useresource",		( getattrofunc ) wpItem_useresource, METH_VARARGS, "Consumes a given resource from within the current item." },
{ "countresource",		( getattrofunc ) wpItem_countresource, METH_VARARGS, "Returns the amount of a given resource available in this container." },
{ "addtimer",			( getattrofunc ) wpItem_addtimer, METH_VARARGS, "Attaches a timer to this object." },
{ "getoutmostchar",		( getattrofunc ) wpItem_getoutmostchar, METH_VARARGS, "Get the outmost character." },
{ "getoutmostitem",		( getattrofunc ) wpItem_getoutmostitem, METH_VARARGS, "Get the outmost item." },
{ "getname",			( getattrofunc ) wpItem_getname, METH_VARARGS, "Get item name." },
{ "lightning",			( getattrofunc ) wpItem_lightning, METH_VARARGS, 0 },
{ "resendtooltip",		( getattrofunc ) wpItem_resendtooltip, METH_VARARGS, 0 },
{ "dupe",				( getattrofunc ) wpItem_dupe, METH_VARARGS, 0 },
{ "say",				( getattrofunc ) wpItem_say, METH_VARARGS | METH_KEYWORDS, 0 },
{ "effect",				( getattrofunc ) wpItem_effect, METH_VARARGS, 0 },

// Definition Properties
{ "getintproperty", ( getattrofunc ) wpItem_getintproperty,		METH_VARARGS, 0 },
{ "getstrproperty", ( getattrofunc ) wpItem_getstrproperty,		METH_VARARGS, 0 },
{ "hasintproperty", ( getattrofunc ) wpItem_hasintproperty,		METH_VARARGS, 0 },
{ "hasstrproperty", ( getattrofunc ) wpItem_hasstrproperty,		METH_VARARGS, 0 },

// Event handling
{ "callevent",			( getattrofunc ) wpItem_callevent, METH_VARARGS, 0 },
{ "addscript",			( getattrofunc ) wpItem_addscript,			METH_VARARGS, 0},
{ "removescript",		( getattrofunc ) wpItem_removescript,		METH_VARARGS, 0},
{ "hasscript",			( getattrofunc ) wpItem_hasscript,			METH_VARARGS, 0},

// Effects
{ "movingeffect",		( getattrofunc ) wpItem_movingeffect, METH_VARARGS, "Shows a moving effect moving toward a given object or coordinate." },

// Tag System
{ "gettag",				( getattrofunc ) wpItem_gettag, METH_VARARGS, "Gets a tag assigned to a specific item." },
{ "settag",				( getattrofunc ) wpItem_settag, METH_VARARGS, "Sets a tag assigned to a specific item." },
{ "hastag",				( getattrofunc ) wpItem_hastag, METH_VARARGS, "Checks if a certain item has the specified tag." },
{ "deltag",				( getattrofunc ) wpItem_deltag, METH_VARARGS, "Deletes the specified tag." },

// Is*? Functions
{ "isitem",				( getattrofunc ) wpItem_isitem, METH_VARARGS, "Is this an item." },
{ "ischar",				( getattrofunc ) wpItem_ischar, METH_VARARGS, "Is this a char." },
{ "isblessed",			( getattrofunc ) wpItem_isblessed, METH_VARARGS, "Is this item blessed(newbie) "},
{ NULL, NULL, 0, NULL }
};

// Getters + Setters

static PyObject* wpItem_getAttr( wpItem* self, char* name )
{
	// Special Python things
	/*
		\rproperty item.content A tuple of all items inside of the container.
	*/
	if ( !strcmp( "content", name ) )
	{
		const ContainerContent &content = self->pItem->content();
		PyObject* list = PyTuple_New( content.count() );
		unsigned int i = 0;
		for ( ContainerIterator it( content ); !it.atEnd(); ++it )
			PyTuple_SetItem( list, i++, PyGetItemObject( *it ) );
		return list;
	}
	/*
		\rproperty item.tags A tuple of all tag names the object currently has.
	*/
	else if ( !strcmp( "tags", name ) )
	{
		// Return a list with the keynames
		QStringList tags = self->pItem->getTags();
		PyObject *list = PyTuple_New(tags.count());
		unsigned int i = 0;
		for ( QStringList::iterator it = tags.begin(); it != tags.end(); ++it )
		{
			PyTuple_SetItem(list, i++, QString2Python(*it));
		}

		return list;
	}
	/*
		\rproperty item.objects If the item is a multi object, this is a list of objects that are within
		the multi. If it's not a multi, this property is None.
	*/
	else if ( !strcmp( "objects", name ) )
	{
		cMulti* multi = dynamic_cast<cMulti*>( self->pItem );

		if ( !multi )
		{
			Py_RETURN_NONE;
		}

		const QList<cUObject*>& objects = multi->getObjects();
		PyObject* tuple = PyTuple_New( objects.count() );
		QList<cUObject*>::const_iterator it( objects.begin() );
		unsigned int i = 0;
		for ( ; it != objects.end(); ++it )
		{
			PyTuple_SetItem( tuple, i++, ( *it )->getPyObject() );
		}
		return tuple;
	}
	/*
		\rproperty item.scripts Returns a list of all script names the object has.
	*/
	else if ( !strcmp( "scripts", name ) )
	{
		QList<QByteArray> events = self->pItem->scriptList().split( ',' );
		if ( self->pItem->basedef() )
		{
			const QList<cPythonScript*> &list = self->pItem->basedef()->baseScripts();
			QList<cPythonScript*>::const_iterator it( list.begin() );
			while ( it != list.end() )
			{
				events.append( ( *it )->name() );
				++it;
			}
		}

		PyObject* list = PyTuple_New( events.count() );
		for ( int i = 0; i < events.count(); ++i )
			PyTuple_SetItem( list, i, PyString_FromString( events[i] ) );
		return list;
	}
	else
	{
		PyObject* result = self->pItem->getProperty( name );
		if ( result )
		{
			return result;
		}
	}

	return Py_FindMethod( wpItemMethods, ( PyObject * ) self, name );
}

static int wpItem_setAttr( wpItem* self, char* name, PyObject* value )
{
	/*
		\rproperty item.container Returns the serial of the container this item is in. Returns 0 if no container.
	*/
	if ( !strcmp( "container", name ) )
	{
		if ( checkWpItem( value ) )
		{
			P_ITEM pCont = getWpItem( value );
			if ( pCont )
				pCont->addItem( self->pItem );
		}
		else if ( checkWpChar( value ) )
		{
			P_CHAR pCont = getWpChar( value );
			if ( pCont )
			{
				tile_st tile = TileCache::instance()->getTile( self->pItem->id() );
				if ( tile.layer )
				{
					if ( pCont->atLayer( ( cBaseChar::enLayer ) tile.layer ) )
						pCont->atLayer( ( cBaseChar::enLayer ) tile.layer )->toBackpack( pCont );
					pCont->addItem( ( cBaseChar::enLayer ) tile.layer, self->pItem );
				}
			}
		}
		else
		{
			self->pItem->removeFromCont();
			self->pItem->moveTo( self->pItem->pos() );
		}
	}
	else
	{
		cVariant val;
		if ( PyString_Check( value ) || PyUnicode_Check( value ) )
			val = cVariant( boost::python::extract<QString>( value ) );
		else if ( PyInt_Check( value ) )
			val = cVariant( PyInt_AsLong( value ) );
		else if ( PyLong_Check( value ) )
			val = cVariant( PyLong_AsLong( value ) );
		else if ( checkWpItem( value ) )
			val = cVariant( getWpItem( value ) );
		else if ( checkWpChar( value ) )
			val = cVariant( getWpChar( value ) );
		else if ( checkWpCoord( value ) )
			val = cVariant( getWpCoord( value ) );
		else if ( PyFloat_Check( value ) )
			val = cVariant( PyFloat_AsDouble( value ) );
		else if ( value == Py_True )
			val = cVariant( 1 ); // True
		else if ( value == Py_False )
			val = cVariant( 0 ); // false

		//if( !val.isValid() )
		//{
		//	if( value->ob_type )
		//		PyErr_Format( PyExc_TypeError, "Unsupported object type: %s", value->ob_type->tp_name );
		//	else
		//		PyErr_Format( PyExc_TypeError, "Unknown object type" );
		//	return 0;
		//}

		stError * error = self->pItem->setProperty( name, val );

		if ( error )
		{
			PyErr_Format( PyExc_TypeError, "Error while setting attribute '%s': %s", name, error->text.toLatin1().constData() );
			delete error;
			return -1;
		}
	}

	return 0;
}

P_ITEM getWpItem( PyObject* pObj )
{
	if ( pObj->ob_type != &wpItemType )
		return 0;

	wpItem* item = ( wpItem* ) ( pObj );
	return item->pItem;
}

bool checkWpItem( PyObject* pObj )
{
	if ( pObj->ob_type != &wpItemType )
		return false;
	else
		return true;
}

int wpItem_compare( PyObject* a, PyObject* b )
{
	// Both have to be characters
	if ( a->ob_type != &wpItemType || b->ob_type != &wpItemType )
		return -1;

	P_ITEM pA = getWpItem( a );
	P_ITEM pB = getWpItem( b );

	return !( pA == pB );
}

int PyConvertItem( PyObject* object, P_ITEM* item )
{
	if ( object->ob_type != &wpItemType )
	{
		PyErr_BadArgument();
		return 0;
	}

	*item = ( ( wpItem * ) object )->pItem;
	return 1;
}
