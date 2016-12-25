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

/*!
	The content of this file is exposed to wolfpack.cpp
	which uses it for startup of the python engine
*/

#if !defined(__PYTHON_ENGINE_H__)
#define __PYTHON_ENGINE_H__

#include <QtGlobal>
#include <QString>

// Stupid Python developers
#if defined(_POSIX_C_SOURCE)
#	undef _POSIX_C_SOURCE
#endif

//#define DEBUG_PYTHON
#undef slots
#if defined(_DEBUG) && !defined(DEBUG_PYTHON)
#	undef _DEBUG
#	include <Python.h>
#	define _DEBUG
#else
#	include <Python.h>
#endif
#define slots

#if !defined(Py_RETURN_NONE) // New in Python 2.4
inline PyObject* doPy_RETURN_NONE()
{
	Py_INCREF( Py_None ); return Py_None;
}
#define Py_RETURN_NONE return doPy_RETURN_NONE()
#endif

#if !defined(Py_RETURN_TRUE) // New in Python 2.4
inline PyObject* doPy_RETURN_TRUE()
{
	Py_INCREF( Py_True ); return Py_True;
}
#	define Py_RETURN_TRUE return doPy_RETURN_TRUE()
#endif

#if !defined(Py_RETURN_FALSE) // New in Python 2.4
inline PyObject* doPy_RETURN_FALSE()
{
	Py_INCREF( Py_False ); return Py_False;
}
#define Py_RETURN_FALSE return doPy_RETURN_FALSE()
#endif

#include "../server.h"
typedef void ( *fnCleanupHandler )();

class CleanupAutoRegister
{
public:
	CleanupAutoRegister( fnCleanupHandler );
};

class cPythonEngine : public cComponent
{
public:
	cPythonEngine();
	virtual ~cPythonEngine();

	void load();
	void unload();

	static QString version();
};

typedef Singleton<cPythonEngine> PythonEngine;

void registerCleanupHandler( fnCleanupHandler );
void reportPythonError( const QString& moduleName = QString::null );

#endif // __PYTHON_ENGINE_H__
