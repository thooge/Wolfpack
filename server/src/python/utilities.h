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

#if !defined( __UTILITIES_H__ )
#define __UTILITIES_H__

#include <boost/python.hpp>

#include "engine.h"
#include "pyerrors.h"
#include <QString>
#include <QList>
#include <QByteArray>

#include "../typedefs.h"

class cUOSocket;
class cItem;
class cBaseChar;
class Coord;
class cAccount;
class cTerritory;
class cUOTxTooltipList;
class cMulti;
class AbstractAI;

typedef cItem* P_ITEM;
typedef cMulti* P_MULTI;

#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#endif

/*!
	Things commonly used in other python-definition
	source-files.
*/
inline PyObject* PyFalse()
{
	Py_INCREF( Py_False );
	return Py_False;
}

inline PyObject* PyTrue()
{
	Py_INCREF( Py_True );
	return Py_True;
}

#define PyHasMethod(a) if( codeModule == NULL ) return false; if( !PyObject_HasAttrString( codeModule, a ) ) return false;

void wpDealloc( PyObject* self );

PyObject* PyGetTooltipObject( cUOTxTooltipList* );

int PyConvertObject( PyObject* object, cUObject** uobject );

bool checkWpSocket( PyObject* object );
PyObject* PyGetSocketObject( cUOSocket* );
cUOSocket* getWpSocket( PyObject* object );
int PyConvertSocket( PyObject* object, cUOSocket** sock );

bool checkWpCoord( PyObject* object );
int PyConvertCoord( PyObject* object, Coord* pos );
PyObject* PyGetCoordObject( const Coord& coord );
Coord getWpCoord( PyObject* object );

bool checkWpItem( PyObject* object );
PyObject* PyGetItemObject( P_ITEM );
P_ITEM getWpItem( PyObject* );
int PyConvertItem( PyObject*, P_ITEM* item );

PyObject* PyGetObjectObject( cUObject* );

bool checkWpChar( PyObject* object );
PyObject* PyGetCharObject( P_CHAR );
P_CHAR getWpChar( PyObject* );
int PyConvertChar( PyObject* object, P_CHAR* character );
int PyConvertPlayer( PyObject* object, P_PLAYER* player );

bool checkWpAccount( PyObject* object );
PyObject* PyGetAccountObject( cAccount* );
cAccount* getWpAccount( PyObject* );

bool checkWpRegion( PyObject* object );
PyObject* PyGetRegionObject( cTerritory* );
cTerritory* getWpRegion( PyObject* );

bool checkWpMulti( PyObject* object );
PyObject* PyGetMultiObject( P_MULTI );
P_MULTI getWpMulti( PyObject* );

bool checkWpAI( PyObject* object );
PyObject* PyGetAIObject( AbstractAI* );
AbstractAI* getWpAI( PyObject* );

// Argument checks
#define checkArgObject( id ) ( PyTuple_Size( args ) > id && ( checkWpItem( PyTuple_GetItem( args, id ) ) || checkWpChar( PyTuple_GetItem( args, id ) ) ) )
#define checkArgChar( id ) ( PyTuple_Size( args ) > id && checkWpChar( PyTuple_GetItem( args, id ) ) )
#define checkArgItem( id ) ( PyTuple_Size( args ) > id && checkWpItem( PyTuple_GetItem( args, id ) ) )
#define checkArgCoord( id ) ( PyTuple_Size( args ) > id && checkWpCoord( PyTuple_GetItem( args, id ) ) )
#define getArgCoord( id ) getWpCoord( PyTuple_GetItem( args, id ) )
#define getArgItem( id ) getWpItem( PyTuple_GetItem( args, id ) )
#define getArgChar( id ) getWpChar( PyTuple_GetItem( args, id ) )
#define checkArgInt( id ) ( PyTuple_Size( args ) > id && PyInt_Check( PyTuple_GetItem( args, id ) ) )
#define getArgInt( id ) PyInt_AsLong( PyTuple_GetItem( args, id ) )
#define checkArgStr( id ) ( PyTuple_Size( args ) > id && ( PyString_Check( PyTuple_GetItem( args, id ) ) || PyUnicode_Check( PyTuple_GetItem( args, id ) ) ) )
#define checkArgUnicode( id ) ( PyTuple_Size( args ) > id && PyUnicode_Check( PyTuple_GetItem( args, id ) ) )
#define getArgStr( id ) (boost::python::extract<QString>(PyTuple_GetItem(args,id)))
#define getArgUnicode( id ) (boost::python::extract<QString>( PyTuple_GetItem( args, id ) ))
#define getUnicodeSize( id ) PyUnicode_GetSize( PyTuple_GetItem( args, id ) )
#define checkArgAccount( id ) ( PyTuple_Size( args ) > id && checkWpAccount( PyTuple_GetItem( args, id ) ) )
#define checkArgRegion( id ) ( PyTuple_Size( args ) > id && checkWpRegion( PyTuple_GetItem( args, id ) ) )
#define getArgRegion( id ) getWpRegion( PyTuple_GetItem( args, id ) )
#define getArgAccount( id ) getWpAccount( PyTuple_GetItem( args, id ) )
#define checkArgMulti( id ) ( PyTuple_Size( args ) > id && checkWpMulti( PyTuple_GetItem( args, id ) ) )
#define getArgMulti( id ) getWpMulti( PyTuple_GetItem( args, id ) )

inline PyObject* QString2Python( const QString& string )
{
	return boost::python::to_python_value<const QString&>()( string );
}

class PythonFunction
{
	PyObject* pModule;
	PyObject* pFunc;
	QByteArray sModule;
	QByteArray sFunc;
	bool temp;

	static QList<PythonFunction*> instances; // list of all known instances
public:
	explicit PythonFunction( PyObject* function ) : pModule( 0 ), pFunc( 0 ), temp( false )
	{
		// No lambdas!
		if ( function ) {
			boost::python::object module ( (boost::python::handle<>( PyObject_GetAttrString(function, "__module__") )) );
			boost::python::object name ( (boost::python::handle<>( PyObject_GetAttrString(function, "__name__") )) );

			if (name && module) {
				sModule = boost::python::extract<QByteArray>(module);
				sFunc = boost::python::extract<QByteArray>(name);
				pFunc = function;
				Py_XINCREF( pFunc );
			}
		}

		instances.append(this); // Add this to the static list of instances
	}

	explicit PythonFunction( const QString& path, bool tempObject = false ) : pModule( 0 ), pFunc( 0 ), temp( tempObject )
	{
		int position = path.lastIndexOf( "." );
		sModule = path.left( position ).toLatin1();
		sFunc = path.right( path.length() - ( position + 1 ) ).toLatin1();

		// The Python string functions don't like null pointers
		if (!sModule.isEmpty()) {
			pModule = PyImport_ImportModule( const_cast<char*>( sModule.data() ) );

			if ( pModule && !sFunc.isEmpty() )
			{
				pFunc = PyObject_GetAttrString( pModule, const_cast<char*>( sFunc.data() ) );
				if ( pFunc && !PyCallable_Check( pFunc ) )
				{
					cleanUp();
				}
			}
		}

		if ( !temp )
			instances.append(this); // Add this to the static list of instances
	}

	~PythonFunction()
	{
		cleanUp();
		if ( !temp )
			instances.removeAll(this); // Remove this from the static list of instances
	}

	// Clean up all instances
	static void cleanUpAll() {
		QList<PythonFunction*>::iterator it;
		for (it = instances.begin(); it != instances.end(); ++it) {
			(*it)->cleanUp();
		}
	}

	// Recreate all pythonfunction instances
	static void recreateAll() {
		QList<PythonFunction*>::iterator it;
		for (it = instances.begin(); it != instances.end(); ++it) {
			(*it)->recreate();
		}
	}

	// Refresh the function pointers based on our sFunc and sModule strings
	void recreate() {
		cleanUp();

		// The Python string functions don't like null pointers
		if (!sModule.isEmpty()) {
			pModule = PyImport_ImportModule( const_cast<char*>( sModule.data() ) );

			if ( pModule && !sFunc.isEmpty() )
			{
				pFunc = PyObject_GetAttrString( pModule, const_cast<char*>( sFunc.data() ) );
				if ( pFunc && !PyCallable_Check( pFunc ) )
				{
					cleanUp();
				}
			}
		}
	}

	// NOTE: Returns a NEW reference
	PyObject* function() const
	{
		Py_XINCREF( pFunc );
		return pFunc;
	}

	QString functionPath() const
	{
		if (isValid()) {
			boost::python::object module( (boost::python::handle<>( PyObject_GetAttrString(pFunc, "__module__") )) );
			boost::python::object name( (boost::python::handle<>( PyObject_GetAttrString(pFunc, "__name__") )) );
			QString result = boost::python::extract<QString>(module);
			result.append(".");
			result.append( boost::python::extract<QString>(name) );
			return result;
		} else {
			return QString::null;
		}
	}

	void cleanUp()
	{
		Py_XDECREF( pFunc );
		Py_XDECREF( pModule );
		pFunc = 0;
		pModule = 0;
	}

	bool isValid() const { return pFunc != 0 && PyCallable_Check( pFunc );	}

	PyObject* operator()( PyObject* args )
	{
		try
		{
			PyObject* result = 0;
			Py_XINCREF( args );
			if ( isValid() )
				result = PyEval_CallObject( pFunc, args );
			reportPythonError( sModule );
			Py_XDECREF( args );
			return result;
		}
		catch( boost::python::error_already_set& )
		{
			return 0;
		}
	}
};

class PythonGILLocker
{
	PyGILState_STATE gstate;
public:
	PythonGILLocker()
	{
		gstate = PyGILState_Ensure();
	}
	
	~PythonGILLocker()
	{
		PyGILState_Release(gstate);
	}
};

// Helper functions for managing stealed references in lists and dictionaries
// NOTE: THIS FUNCTION DECREFS THE GIVEN PYOBJECT!!!
inline void PyDict_SetStolenItem(PyObject *dict, const char *key, PyObject *object) {
	PyDict_SetItemString(dict, key, object);
	Py_DECREF(object);
}

// NOTE: THIS FUNCTION STEALS THE REFERENCE FROM OBJECT!
inline void PyList_AppendStolen(PyObject *list, PyObject *object) {
	PyList_Append(list, object);
	Py_DECREF(object);
}

#endif
