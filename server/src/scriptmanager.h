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

#if !defined(__WPSCRIPTMANAGER_H__)
#define __WPSCRIPTMANAGER_H__

// System Includes
#include <QMap>
#include <QString>
#include <QByteArray>

// Wolfpack Includes
#include "pythonscript.h"
#include "singleton.h"
#include "server.h"

class cScriptManager : public cComponent
{
protected:
	QMap<QByteArray, cPythonScript*> scripts;
	cPythonScript* hooks[EVENT_COUNT];
	QMap<QByteArray, PyObject*> commandhooks;

	// These are for internal use only
	void clearGlobalHooks();
	void clearCommandHooks();
public:
	typedef QMap<QByteArray, cPythonScript*>::iterator iterator;

	cScriptManager();
	virtual ~cScriptManager();

	cPythonScript* find( const QByteArray& name );

	void load();
	void reload();
	void unload();

	void onServerStart(); // Call the onServerStart Event for all registered scripts
	void onServerStop(); // Call the onServerEnd Event for all registered scripts

	void setCommandHook( const QByteArray& command, PyObject* object );
	void setGlobalHook( ePythonEvent event, cPythonScript* script );

	PyObject* getCommandHook( const QByteArray& command );
	cPythonScript* getGlobalHook( ePythonEvent event );
};

typedef Singleton<cScriptManager> ScriptManager;

#endif
