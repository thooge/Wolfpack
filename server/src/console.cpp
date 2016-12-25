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

// Wolfpack Includes
#include "console.h"
#include "pythonscript.h"
#include "log.h"
#include "profile.h"

#include "world.h"
#include "network/network.h"
#include "network/uosocket.h"
#include "serverconfig.h"
#include "player.h"
#include "accounts.h"
#include "inlines.h"

// Library Includes
#include <QString>
#include <QtGlobal>
#include <QThread>
#include <QMutex>

using namespace std;

// Constructor
cConsole::cConsole()
{
	progress = QString::null;
}

// Destuctor
cConsole::~cConsole()
{
	// Clean up any terminal settings
}

void cConsole::log( uchar logLevel, const QString& message, bool timestamp )
{
	// Legacy Code
	QString msg = message;

	if ( msg.endsWith( "\n" ) )
		msg = msg.left( msg.length() - 1 );

	Log::instance()->print( ( eLogLevel ) logLevel, msg + "\n", timestamp );
}

// Prepare a "progess" line
void cConsole::sendProgress( const QString& sMessage )
{
	send( sMessage + "... " );
	changeColor( WPC_NORMAL );
	progress = sMessage + "... ";
}

// Print Progress Done
void cConsole::sendDone()
{
	progress = QString::null;
	changeColor( WPC_GREEN );
	send( tr( "Done\n" ) );
	changeColor( WPC_NORMAL );
}

// Print "Fail"
void cConsole::sendFail()
{
	progress = QString::null;
	changeColor( WPC_RED );
	send( tr( "Failed\n" ) );
	changeColor( WPC_NORMAL );
}

// Print "Skip" (maps etc.)
void cConsole::sendSkip()
{
	progress = QString::null;
	changeColor( WPC_YELLOW );
	send( tr( "Skipped\n" ) );
	changeColor( WPC_NORMAL );
}

bool cConsole::handleCommand( const QString& command )
{
	cUOSocket* mSock;
	QList<cUOSocket*> socketList;
	int i;
	char c = command.toLatin1()[0];
	c = toupper( c );

	if ( c == 'S' )
	{
		Server::instance()->setSecure( !Server::instance()->getSecure() );

		if ( !Server::instance()->getSecure() )
			Console::instance()->send( tr( "WOLFPACK: Secure mode disabled. Press ? for a commands list.\n" ) );
		else
			Console::instance()->send( tr( "WOLFPACK: Secure mode re-enabled.\n" ) );

		return true;
	}

	// Allow Help in Secure Mode
	if ( Server::instance()->getSecure() && c != '?' )
	{
		Console::instance()->send( tr( "WOLFPACK: Secure mode prevents keyboard commands! Press 'S' to disable.\n" ) );
		return false;
	}

	switch ( c )
	{
	case 'Q':
		Console::instance()->send( tr( "WOLFPACK: Immediate Shutdown initialized!\n" ) );
		Server::instance()->cancel();
		break;

	case '#':
		World::instance()->save();
		Config::instance()->flush();
		break;

	case 'P':
		dumpProfilingInfo();
		break;

	case 'W':
		Console::instance()->send( tr( "Current Users in the World:\n" ) );

		socketList = Network::instance()->sockets();
		i = 0;

		foreach ( mSock, socketList )
		{
			if ( mSock->player() )
				Console::instance()->send( QString( "%1) %2 [%3]\n" ).arg( ++i ).arg( mSock->player()->name() ).arg( QString::number( mSock->player()->serial(), 16 ) ) );
		}

		Console::instance()->send( tr( "Total Users Online: %1\n" ).arg( Network::instance()->count() ) );
		break;
	case 'A':
		//reload the accounts file
		Server::instance()->queueAction( RELOAD_ACCOUNTS );
		break;
	case 'R':
		Server::instance()->queueAction( RELOAD_SCRIPTS );
		break;
	case '?':
		Console::instance()->send( tr( "Console commands:\n" ) );
		Console::instance()->send( tr( "	Q: Shutdown the server.\n" ) );
		Console::instance()->send( tr( "	# - Save world\n" ) );
		Console::instance()->send( tr( "	W - Display logged in characters\n" ) );
		Console::instance()->send( tr( "	A - Reload accounts\n" ) );
		Console::instance()->send( tr( "	R - Reload scripts\n" ) );
		Console::instance()->send( tr( "	S - Toggle Secure mode " ) );
		if ( Server::instance()->getSecure() )
			Console::instance()->send( tr( "[enabled]\n" ) );
		else
			Console::instance()->send( tr( "[disabled]\n" ) );
		Console::instance()->send( tr( "	? - Commands list (this)\n" ) );
		Console::instance()->send( tr( "End of commands list.\n" ) );
		break;
	default:
		break;
	}

	return true;
}

void cConsole::queueCommand( const QString& command )
{
	QMutexLocker lock( &commandMutex );
	commandQueue.push_back( command );
}
