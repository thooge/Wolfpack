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

#include "scriptmanager.h"
#include "definitions.h"
#include "basechar.h"

#include "console.h"
#include "world.h"
#include "items.h"
#include "pythonscript.h"
#include "python/engine.h"
#include "network/uosocket.h"

// Library Includes
#include <QString>
#include <QRegExp>
#include <QFileInfo>
#include <QDir>
#include <QByteArray>

using namespace std;

cScriptManager::cScriptManager()
{
	for ( unsigned int i = 0; i < EVENT_COUNT; ++i )
		hooks[i] = 0;
}

cScriptManager::~cScriptManager()
{
	unload();
}

cPythonScript* cScriptManager::find( const QByteArray& name )
{
	cScriptManager::iterator it = scripts.find( name );

	if ( it != scripts.end() )
		return it.value();
	else
		return 0;
}

void cScriptManager::reload()
{
	cItemIterator iter_items;
	cCharIterator iter_chars;

	P_ITEM pItem;
	P_CHAR pChar;

	for ( pItem = iter_items.first(); pItem; pItem = iter_items.next() )
		pItem->freezeScriptChain();

	for ( pChar = iter_chars.first(); pChar; pChar = iter_chars.next() )
		pChar->freezeScriptChain();

	// First unload, then reload
	unload();

	PythonEngine::instance()->unload();
	PythonEngine::instance()->load();

	load();

	for ( pItem = iter_items.first(); pItem; pItem = iter_items.next() )
		pItem->unfreezeScriptChain();

	for ( pChar = iter_chars.first(); pChar; pChar = iter_chars.next() )
		pChar->unfreezeScriptChain();

	CharBaseDefs::instance()->refreshScripts();
	ItemBaseDefs::instance()->refreshScripts();

	// Refresh the PythonFunction pointers
	PythonFunction::recreateAll();
}

// Unload all scripts
void cScriptManager::unload()
{
	// Clear all packet handlers.
	cUOSocket::clearPacketHandlers();

	for ( unsigned int i = 0; i < EVENT_COUNT; ++i )
	{
		hooks[i] = 0;
	}

	QMap<QByteArray, PyObject*>::iterator itc;

	for ( itc = commandhooks.begin(); itc != commandhooks.end(); ++itc )
	{
		Py_XDECREF( itc.value() );
	}

	commandhooks.clear();

	cScriptManager::iterator it;

	for ( it = scripts.begin(); it != scripts.end(); ++it )
	{
		it.value()->unload();
		delete it.value();
	}

	scripts.clear();

	cComponent::unload();
}

void cScriptManager::load()
{
	// Each Section is a Script identifier
	const QList<cElement*>& sections = Definitions::instance()->getDefinitions( WPDT_SCRIPT );

	int i;
	for ( i = 0; i < sections.size(); ++i )
	{
		const cElement* element = sections[i];

		if ( scripts.contains( element->text().toLatin1() ) )
		{
			Console::instance()->log( LOG_WARNING, tr( "Duplicate Script: %1" ).arg( element->text() ) );
			continue;
		}

		cPythonScript* script = new cPythonScript;
		scripts.insert( element->text().toUtf8(), script );
		script->load( element->text().toLatin1() );
	}
	cComponent::load();
}

/*
\event onServerStart
\condition Triggered when the server startup is complete.
*/
void cScriptManager::onServerStart()
{
	cScriptManager::iterator it;

	for ( it = scripts.begin(); it != scripts.end(); ++it )
	{
		it.value()->callEventHandler( "onServerStart" );
	}
}

void cScriptManager::onServerStop()
{
	cScriptManager::iterator it;

	for ( it = scripts.begin(); it != scripts.end(); ++it )
	{
		it.value()->callEventHandler( "onServerStop" );
	}
}

PyObject* cScriptManager::getCommandHook( const QByteArray& command )
{
	PyObject* result = 0;

	if ( commandhooks.contains( command.toLower() ) )
	{
		result = commandhooks[command.toLower()];
	}

	return result;
}

cPythonScript* cScriptManager::getGlobalHook( ePythonEvent event )
{
	cPythonScript* result = 0;

	if ( event < EVENT_COUNT )
	{
		result = hooks[event];
	}

	return result;
}

void cScriptManager::setCommandHook( const QByteArray& command, PyObject* object )
{
	if ( commandhooks.contains( command.toLower() ) )
	{
		Py_DECREF( commandhooks[command.toLower()] );
	}

	commandhooks.insert( command.toLower(), object );
}

void cScriptManager::setGlobalHook( ePythonEvent event, cPythonScript* script )
{
	if ( event < EVENT_COUNT )
		hooks[event] = script;
}
