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

#include "accounts.h"
#include "multi.h"
#include "world.h"
#include "skills.h"
#include "commands.h"
#include "gumps.h"
#include "muls/maps.h"
#include "scriptmanager.h"
#include "network/uosocket.h"
#include "spawnregions.h"
#include "serverconfig.h"
#include "targetrequests.h"
#include "territories.h"
#include "muls/tilecache.h"
#include "console.h"
#include "definitions.h"
#include "scriptmanager.h"
#include "mapobjects.h"
#include "contextmenu.h"
#include "pythonscript.h"
#include "network/network.h"
#include "muls/multiscache.h"
#include "walking.h"
#include "pathfinding.h"
#include "dbdriver.h"

// System Includes
#include <functional>
#include <QByteArray>
#include <QList>

// Main Command processing function
void cCommands::process( cUOSocket* socket, const QString& command )
{
	if ( !socket->player() )
		return;

	P_PLAYER pChar = socket->player();
	QStringList pArgs = command.split( " ", QString::KeepEmptyParts );

	// No Command? No Processing
	if ( pArgs.isEmpty() )
		return;

	QString pCommand = pArgs[0].toUpper(); // First element should be the command

	// Remove it from the argument list
	pArgs.erase( pArgs.begin() );

	// Check if the priviledges are ok
	if ( !pChar->account()->authorized( "command", pCommand ) )
	{
		socket->sysMessage( tr( "Access to command '%1' was denied" ).arg( pCommand.toLower() ) );
		socket->log(
			tr( "Character '%1', account '%2' used command '%3', access denied\n"
				).arg( socket->player()->orgName()
				).arg( socket->player()->account()->login()
				).arg( pCommand.toLower()
			)
		);

		return;
	}

	// Dispatch the command
	socket->log(
		tr( "Character '%1', account '%2' used command '%3'\n"
			).arg( socket->player()->orgName()
			).arg( socket->player()->account()->login()
			).arg( pCommand.toLower()
		)
	);
	if ( !dispatch( socket, pCommand, pArgs ) )
	{
		socket->log(
			tr( "Character '%1', account '%2' used unknown command '%3'\n"
				).arg( socket->player()->orgName()
				).arg( socket->player()->account()->login()
				).arg( pCommand.toLower()
			)
		);

	}
}

// Selects the right command Stub
bool cCommands::dispatch( cUOSocket* socket, const QString& command, const QStringList& arguments )
{
	// Just in case we have been called directly
	if ( !socket || !socket->player() )
		return false;

	// Check for custom commands
	PyObject* function = ScriptManager::instance()->getCommandHook( command.toLatin1() );

	if ( function )
	{
		QString argString = arguments.join( " " );

		PyObject* args = Py_BuildValue( "O&NN", PyGetSocketObject, socket, QString2Python( command ), QString2Python( argString ) );

		PyObject* result = PyObject_CallObject( function, args );
		Py_XDECREF( result );
		reportPythonError();

		Py_DECREF( args );

		return true;
	}

	for ( uint index = 0; commands[index].command; ++index )
		if ( command == commands[index].name )
		{
			( commands[index].command ) ( socket, command, arguments );
			return true;
		}

	socket->sysMessage( tr( "Unknown Command" ) );
	return false;
}

void cCommands::loadACLs( void )
{
	// make sure it's clean
	qDeleteAll( _acls );
	_acls.clear();

	QStringList ScriptSections = Definitions::instance()->getSections( WPDT_PRIVLEVEL );

	if ( ScriptSections.isEmpty() )
	{
		Console::instance()->log( LOG_WARNING, tr( "No ACLs for players, counselors, gms and admins defined!\n"
												   "Check your scripts, wolfpack.xml and make sure to run Wolfpack from the proper folder\n" ) );
		return;
	}

	// We are iterating trough a list of ACLs
	// In each loop we create one acl
	for ( QStringList::iterator it = ScriptSections.begin(); it != ScriptSections.end(); ++it )
	{
		const cElement* Tag = Definitions::instance()->getDefinition( WPDT_PRIVLEVEL, *it );

		if ( !Tag )
			continue;

		QString ACLname = *it;

		// While we are in this loop we are building an ACL
		cAcl* acl = new cAcl;
		acl->name = ACLname;
		acl->rank = Tag->getAttribute( "rank", "1" ).toUShort();
		if ( acl->rank == 0 || acl->rank == 255 )
		{
			acl->rank = 1;
		}

		QMap<QString, bool> group;
		QByteArray groupName;


		for ( unsigned int i = 0; i < Tag->childCount(); ++i )
		{
			const cElement* childTag = Tag->getChild( i );
			if ( childTag->name() == "group" )
			{
				groupName = childTag->getAttribute( "name" ).toLatin1();

				for ( unsigned int j = 0; j < childTag->childCount(); ++j )
				{
					const cElement* groupTag = childTag->getChild( j );

					if ( groupTag->name() == "action" )
					{
						QString name = groupTag->getAttribute( "name", "any" );
						bool permit = groupTag->getAttribute( "permit", "false" ) == "true" ? true : false;
						group.insert( name, permit );
					}
				}

				if ( !group.isEmpty() )
				{
					acl->groups.insert( groupName, group );
					group.clear();
				}
			}
		}

		_acls.insert( ACLname, acl );
	}

	// Renew the ACL pointer for all loaded accounts
	Accounts::instance()->clearAcls();
}

/*
	\command set
	\description Change properties of characters and items.
	\usage - <code>set key value</code>
	Key is the name of the property you want to set.
	Value is the new property value.
	\notes See the object reference for <object id="char">characters</object>
	and <object id="item">items</object> for valid property keys. All integer, float, string,
	character and item properties can be set using this command as well. In addition to the
	properties you find there, you can also set skills by using skill.skillname as the key and
	the skill value multiplied by ten as the value (i.e. 100.0% = 1000).
*/
void commandSet( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( args.size() < 1 )
	{
		socket->sysMessage( tr( "Usage: set <key> <value>" ) );
		return;
	}

	QString key = args[0];
	QStringList realargs( args );
	realargs.removeFirst();
	QString value;
	if ( realargs.size() == 0 )
	{
		value = "";
	}
	else
	{
		value = realargs.join( " " );
	}

	// Alias for speed sake on setting stats.
	if ( key == "str" )
	{
		socket->sysMessage( tr( "Please select a target to 'set %1 %2' " ).arg( "strength" ).arg( value ) );
		socket->attachTarget( new cSetTarget( "strength", value ) );
	}
	else if ( key == "dex" )
	{
		socket->sysMessage( tr( "Please select a target to 'set %1 %2' " ).arg( "dexterity" ).arg( value ) );
		socket->attachTarget( new cSetTarget( "dexterity", value ) );
	}
	else if ( key == "int" )
	{
		socket->sysMessage( tr( "Please select a target to 'set %1 %2' " ).arg( "intelligence" ).arg( value ) );
		socket->attachTarget( new cSetTarget( "intelligence", value ) );
	}
	else
	{
		socket->sysMessage( tr( "Please select a target to 'set %1 %2' " ).arg( key ).arg( value ) );
		socket->attachTarget( new cSetTarget( key, value ) );
	}
}

/*
	\command save
	\description Forces the world to be saved.
*/
void commandSave( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( args );
	Q_UNUSED( socket );
	Q_UNUSED( command );

	World::instance()->save();
}

/*
	\command servertime
	\description Shows the current server uptime in miliseconds.
*/
void commandServerTime( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( args );
	Q_UNUSED( command );
	socket->sysMessage( tr( "Server time: %1" ).arg( Server::instance()->time() ) );
}

/*
	\command show
	\description Show properties of characters and items.
	\usage - <code>show key</code>
	Key is the name of the property you want to see.
	\notes See the <command id="SET">SET</command> command for more information.
*/
void commandShow( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	socket->sysMessage( tr( "Please select a target" ) );
	socket->attachTarget( new cShowTarget( args.join( " " ) ) );
}

/*
	\command spawnregion
	\description Control a spawnregion.
	\usage - <code>spawnregion respawn id</code>
	- <code>spawnregion clear id</code>
	- <code>spawnregion fill id</code>
	- <code>spawnregion info id</code>
	The respawn subcommand will initiate a respawn of the given spawnregion.
	The clear subcommand will delete all objects spawned by the given region.
	The fill subcommand will initiate a maximal respawn of the given spawnregion.
	The info subcommand will show a dialog with information about the given spawnregion.
	\notes The region id can be <i>all</i> in which case all spawnregions will be affected.
*/
void commandSpawnRegion( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	// Spawnregion respawn region_name
	// Spawnregion clear   region_name
	// Spawnregion fill    region_name
	// Spawnregion info    region_name

	// region_name can be "all"

	if ( args.count() == 0 )
	{
		socket->sysMessage( tr( "Usage: spawnregion <respawn|clear|fill|info>" ) );
		return;
	}

	QString subCommand = args[0].toLower();

	// respawn spawnregion
	if ( subCommand == "respawn" )
	{
		if ( args.count() < 2 )
		{
			socket->sysMessage( tr( "Usage: spawnregion respawn <region_name>" ) );
		}
		else if ( args[1].toLower() != "all" )
		{
			cSpawnRegion* spawnRegion = SpawnRegions::instance()->region( args[1] );
			if ( !spawnRegion )
			{
				if ( !SpawnRegions::instance()->reSpawnGroup( args[1] ) )
				{
					socket->sysMessage( tr( "Spawnregion %1 does not exist." ).arg( args[1] ) );
				}
				else
				{
					socket->sysMessage( tr( "Spawnregion group '%1' has respawned." ).arg( args[1] ) );
				}
			}
			else
			{
				spawnRegion->reSpawn();
				socket->sysMessage( tr( "Spawnregion '%1' has respawned." ).arg( args[1] ) );
			}
		}
		else if ( args[1].toLower() == "all" )
		{
			SpawnRegions::instance()->reSpawn();
			socket->sysMessage( tr( "All spawnregions have respawned." ) );
		}
	}

	// clear spawnregions (despawn)
	else if ( subCommand == "clear" )
	{
		if ( args.count() < 2 )
		{
			socket->sysMessage( tr( "Usage: spawnregion clear <region_name>" ) );
		}
		else if ( args[1].toLower() != "all" )
		{
			cSpawnRegion* spawnRegion = SpawnRegions::instance()->region( args[1] );
			if ( !spawnRegion )
			{
				if ( !SpawnRegions::instance()->deSpawnGroup( args[1] ) )
				{
					socket->sysMessage( tr( "Spawnregion %1 does not exist." ).arg( args[1] ) );
				}
				else
				{
					socket->sysMessage( tr( "Spawnregion group '%1' has been cleared." ).arg( args[1] ) );
				}
			}
			else
			{
				spawnRegion->deSpawn();
				socket->sysMessage( tr( "Spawnregion '%1' has been cleared." ).arg( args[1] ) );
			}
		}
		else if ( args[1].toLower() == "all" )
		{
			SpawnRegions::instance()->deSpawn();
			socket->sysMessage( tr( "All spawnregions have been cleared." ) );
		}
	}

	// fill spawnregions up (respawnmax)
	else if ( subCommand == "fill" )
	{
		if ( args.count() < 2 )
		{
			socket->sysMessage( tr( "Usage: spawnregion fill <region_name>" ) );
		}
		else if ( args[1].toLower() != "all" )
		{
			cSpawnRegion* spawnRegion = SpawnRegions::instance()->region( args[1] );
			if ( !spawnRegion )
			{
				if ( !SpawnRegions::instance()->reSpawnToMaxGroup( args[1] ) )
				{
					socket->sysMessage( tr( "Spawnregion %1 does not exist." ).arg( args[1] ) );
				}
				else
				{
					socket->sysMessage( tr( "Spawnregion group '%1' has been filled." ).arg( args[1] ) );
				}
			}
			else
			{
				spawnRegion->reSpawnToMax();
				socket->sysMessage( tr( "Spawnregion '%1' has been filled." ).arg( args[1] ) );
			}
		}
		else if ( args[1].toLower() == "all" )
		{
			SpawnRegions::instance()->reSpawnToMax();
			socket->sysMessage( tr( "All spawnregions have respawned to maximum." ) );
		}
	}

	// show spawnregion info
	else if ( subCommand == "info" )
	{
		if ( args.count() < 2 )
		{
			socket->sysMessage( tr( "Usage: spawnregion info <region_name>" ) );
		}
		else if ( args[1].toLower() != "all" )
		{
			cSpawnRegion* spawnRegion = SpawnRegions::instance()->region( args[1] );
			if ( !spawnRegion )
			{
				socket->sysMessage( tr( "Spawnregion %1 does not exist" ).arg( args[1] ) );
			}
			else
			{
				cSpawnRegionInfoGump* pGump = new cSpawnRegionInfoGump( spawnRegion );
				socket->send( pGump );
			}
		}
		else if ( args[1].toLower() == "all" )
		{
			// Display a gump with this information
			cGump* pGump = new cGump();

			// Basic .INFO Header
			pGump->addResizeGump( 0, 40, 0xA28, 450, 210 ); //Background
			pGump->addGump( 105, 18, 0x58B ); // Fancy top-bar
			pGump->addGump( 182, 0, 0x589 ); // "Button" like gump
			pGump->addTilePic( 202, 23, 0x14eb ); // Type of info menu

			pGump->addText( 160, 90, tr( "Spawnregion Global Info" ), 0x530 );

			// Give information about the spawnregions
			pGump->addText( 50, 120, tr( "Spawnregions: %1" ).arg( SpawnRegions::instance()->size() ), 0x834 );
			pGump->addText( 50, 140, tr( "NPCs: %1 of %2" ).arg( SpawnRegions::instance()->npcs() ).arg( SpawnRegions::instance()->maxNpcs() ), 0x834 );
			pGump->addText( 50, 160, tr( "Items: %1 of %2" ).arg( SpawnRegions::instance()->items() ).arg( SpawnRegions::instance()->maxItems() ), 0x834 );

			// OK button
			pGump->addButton( 50, 200, 0xF9, 0xF8, 0 ); // Only Exit possible

			socket->send( pGump );
		}
	}
}

/*
	\command shutdown
	\description Shutdown the Wolfpack server.
*/
void commandShutDown( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( socket );
	Q_UNUSED( command );
	// Shutdown
	if ( args.count() == 0 )
		Server::instance()->cancel();
}

/*
	\command staff
	\description Toggle the staff flag for your account.
	\notes The staff flag controls whether you are treated as a priviledged user or not.
*/
void commandStaff( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( socket->account()->isStaff() || ( args.count() > 0 && args[0].toInt() == 0 ) )
	{
		socket->account()->setStaff( false );
		socket->sysMessage( tr( "Staff is now '0'." ) );
	}
	else if ( !socket->account()->isStaff() || ( args.count() > 0 && args[0].toInt() == 1 ) )
	{
		socket->account()->setStaff( true );
		socket->sysMessage( tr( "Staff is now '1'." ) );
	}
}

/*
	\command showserials
	\description Toggle the show serials flag for your account.
	\notes It shows you the serials of a char in the tooltip.
*/
void commandShowserials( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( !socket->player() || !socket->player()->account() )
		return;

	// Switch
	if ( !args.count() )
		socket->player()->account()->setShowSerials( !socket->player()->account()->isShowSerials() );
	// Set
	else
		socket->player()->account()->setShowSerials( args[0].toInt() != 0 );

	if ( socket->player()->account()->isShowSerials() )
		socket->sysMessage( tr( "ShowSerials is [enabled]" ) );
	else
		socket->sysMessage( tr( "ShowSerials is [disabled]" ) );

	// Resend the world to us
	socket->resendWorld( true );
}

/*
	\command reload
	\description Reload certain aspects of the server.
	\usage - <code>reload accounts</code>
	- <code>reload scripts</code>
	- <code>reload python</code>
	- <code>reload all</code>
	Reload the given server component.
	\notes The <i>accounts</i> parameter will reload all accounts from the account database.
	The <i>python</i> parameter will reload all Python scripts.
	The <i>scripts</i> parameter will reload all XML definitions and Python scripts.
	The <i>all</i> parameter will reload all three.
*/
void commandReload( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	// Reload accounts
	// Reload scripts
	// Reload all

	if ( args.count() == 0 )
	{
		socket->sysMessage( tr( "Usage: reload <accounts|scripts|python|all>" ) );
		return;
	}

	QString subCommand = args[0].toLower();

	// accounts
	if ( subCommand == "accounts" )
	{
		Server::instance()->reload( "accounts" );
	}
	else if ( subCommand == "python" )
	{
		Network::instance()->broadcast( tr( "Reloading python scripts." ) );
		Server::instance()->reload( "scripts" );
		Network::instance()->broadcast( tr( "Finished reloading python scripts." ) );
	}
	else if ( subCommand == "scripts" )
	{
		Network::instance()->broadcast( tr( "Reloading definitions." ) );
		Server::instance()->reload( "definitions" );
		Network::instance()->broadcast( tr( "Finished reloading definitions." ) );
	}
	else if ( subCommand == "muls" )
	{
		Network::instance()->broadcast( tr( "Reloading mul files." ) );
		Maps::instance()->reload();
		TileCache::instance()->reload();
		MultiCache::instance()->reload();
		Network::instance()->broadcast( tr( "Finished reloading mul files." ) );
	}
	else if ( subCommand == "all" )
	{
		Network::instance()->broadcast( tr( "Reloading server configuration." ) );
		Server::instance()->reload( "configuration" ); // This will reload nearly everything
		Network::instance()->broadcast( tr( "Finished reloading server configuration." ) );
	}
}

/*
	\command allmove
	\description Toggles the allmove flag of your account.
	\notes The allmove flag determines whether you can move immovable objects.
*/
void commandAllMove( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( !socket->player() || !socket->player()->account() )
		return;

	// Switch
	if ( !args.count() )
		socket->player()->account()->setAllMove( !socket->player()->account()->isAllMove() );
	// Set
	else
		socket->player()->account()->setAllMove( args[0].toInt() != 0 );

	if ( socket->player()->account()->isAllMove() )
		socket->sysMessage( tr( "AllMove is [enabled]" ) );
	else
		socket->sysMessage( tr( "AllMove is [disabled]" ) );

	// Resend the world to us
	socket->resendWorld( true );
}

/*
	\command broadcast
	\description Broadcast a message to all connected clients.
	\usage - <code>broadcast [message]</code>
	Message is the message you want to broadcast to everyone.
*/
void commandBroadcast( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( socket );
	Q_UNUSED( command );
	Network::instance()->broadcast( args.join( " " ) );
}

/*
	\command pagenotify
	\description Toggle notification about new support tickets.
	\notes If you opt to turn this flag on, you will be notified about incoming pages.
*/
void commandPageNotify( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( socket->account()->isPageNotify() || ( args.count() > 0 && args[0].toInt() == 0 ) )
	{
		socket->account()->setPageNotify( false );
		socket->sysMessage( tr( "PageNotify is now '0'." ) );
	}
	else if ( !socket->account()->isPageNotify() || ( args.count() > 0 && args[0].toInt() == 1 ) )
	{
		socket->account()->setPageNotify( true );
		socket->sysMessage( tr( "PageNotify is now '1'." ) );
	}
}

/*
	\command gmtalk
	\description Broadcast a message to connected gamemasters.
	\usage - <code>gmtalk [message]</code>
	Send a message to all other connected gamemasters.
*/
void commandGmtalk( cUOSocket* socket, const QString& command, const QStringList& args ) throw()
{
	Q_UNUSED( command );
	if ( args.count() < 1 )
	{
		socket->sysMessage( tr( "Usage: gmtalk <message>" ) );
		return;
	}

	QString message = "<" + socket->player()->name() + ">: " + args.join( " " );

	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( cUOSocket* mSock, sockets )
	{
		if ( mSock->player() && mSock->player()->isGM() )
			mSock->sysMessage( message, 32 );
	}
}

#include "exportdefinitions.h"

/*
	\command exportdefinitions
	\description Export the definitions used by the WPGM utility.
	\notes This command will export the definitions used by the WPGM utility to
	a file called categories.db in your wolfpack directory.
*/
void commandExportDefinitions(cUOSocket* socket, const QString& /*command*/, const QStringList& /*args*/ ) throw()
{
	cDefinitionExporter exporter;
	exporter.setSocket(socket);
	exporter.generate("categories.db");
	return;
}

/*
	\command walktest
	\description Checks if the character could walk into the direction he is facing if he was a npc.
	\notes This command is very useful for testing npc movement as a staff member.
*/
void commandWalkTest( cUOSocket* socket, const QString& /*command*/, const QStringList& /*args*/ ) throw()
{
	Coord newpos = socket->player()->pos();
	newpos = Movement::instance()->calcCoordFromDir( socket->player()->direction(), newpos );

	bool result = mayWalk( socket->player(), newpos );

	if ( !result )
	{
		socket->sysMessage( tr( "You may not walk in that direction." ), 0x26 );
	}
	else
	{
		socket->sysMessage( tr( "You may walk in that direction. (New Z: %1)" ).arg( newpos.z ), 0x3a );
	}
}

/*
	\command doorgen
	\description Generate doors in passage ways.
	\notes This command is not guranteed to work correctly. Please see if
	you find any broken doors after you use this command. Don't use this command
	on custom maps.
*/
void commandDoorGenerator( cUOSocket* socket, const QString& /*command*/, const QStringList& args ) throw()
{
	Q_UNUSED( args );
	class DoorGenerator
	{
		enum DoorFacing
		{
			WestCW			= 0,
			EastCCW,
			WestCCW,
			EastCW,
			SouthCW,
			NorthCCW,
			SouthCCW,
			NorthCW
		};

		bool isFrame( int id, int frames[], int size )
		{
			id &= 0x3FFF;
			if ( id > frames[size - 1] )
				return false;

			for ( int i = 0; i < size; ++i )
			{
				int delta = id - frames[i];

				if ( delta < 0 )
					return false;
				else if ( delta == 0 )
					return true;
			}
			return false;
		}

		bool isSouthFrame( int id )
		{
			static int SouthFrames[] =
			{
			0x0006, 0x0008, 0x000B, 0x001A, 0x001B, 0x001F, 0x0038, 0x0057, 0x0059, 0x005B, 0x005D, 0x0080, 0x0081, 0x0082, 0x0084, 0x0090, 0x0091, 0x0094, 0x0096, 0x0099, 0x00A6, 0x00A7, 0x00AA, 0x00AE, 0x00B0, 0x00B3, 0x00C7, 0x00C9, 0x00F8, 0x00FA, 0x00FD, 0x00FE, 0x0100, 0x0103, 0x0104, 0x0106, 0x0109, 0x0127, 0x0129, 0x012B, 0x012D, 0x012F, 0x0131, 0x0132, 0x0134, 0x0135, 0x0137, 0x0139, 0x013B, 0x014C, 0x014E, 0x014F, 0x0151, 0x0153, 0x0155, 0x0157, 0x0158, 0x015A, 0x015D, 0x015E, 0x015F, 0x0162, 0x01CF, 0x01D1, 0x01D4, 0x01FF, 0x0204, 0x0206, 0x0208, 0x020A
			};
			return isFrame( id, SouthFrames, sizeof( SouthFrames ) );
		}

		bool isNorthFrame( int id )
		{
			static int NorthFrames[] =
			{
			0x0006, 0x0008, 0x000D, 0x001A, 0x001B, 0x0020, 0x003A, 0x0057, 0x0059, 0x005B, 0x005D, 0x0080, 0x0081, 0x0082, 0x0084, 0x0090, 0x0091, 0x0094, 0x0096, 0x0099, 0x00A6, 0x00A7, 0x00AC, 0x00AE, 0x00B0, 0x00C7, 0x00C9, 0x00F8, 0x00FA, 0x00FD, 0x00FE, 0x0100, 0x0103, 0x0104, 0x0106, 0x0109, 0x0127, 0x0129, 0x012B, 0x012D, 0x012F, 0x0131, 0x0132, 0x0134, 0x0135, 0x0137, 0x0139, 0x013B, 0x014C, 0x014E, 0x014F, 0x0151, 0x0153, 0x0155, 0x0157, 0x0158, 0x015A, 0x015D, 0x015E, 0x015F, 0x0162, 0x01CF, 0x01D1, 0x01D4, 0x01FF, 0x0201, 0x0204, 0x0208, 0x020A
			};
			return isFrame( id, NorthFrames, sizeof( NorthFrames ) );
		}

		bool isEastFrame( int id )
		{
			static int EastFrames[] =
			{
			0x0007, 0x000A, 0x001A, 0x001C, 0x001E, 0x0037, 0x0058, 0x0059, 0x005C, 0x005E, 0x0080, 0x0081, 0x0082, 0x0084, 0x0090, 0x0092, 0x0095, 0x0097, 0x0098, 0x00A6, 0x00A8, 0x00AB, 0x00AE, 0x00AF, 0x00B2, 0x00C7, 0x00C8, 0x00EA, 0x00F8, 0x00F9, 0x00FC, 0x00FE, 0x00FF, 0x0102, 0x0104, 0x0105, 0x0108, 0x0127, 0x0128, 0x012B, 0x012C, 0x012E, 0x0130, 0x0132, 0x0133, 0x0135, 0x0136, 0x0138, 0x013A, 0x014C, 0x014D, 0x014F, 0x0150, 0x0152, 0x0154, 0x0156, 0x0158, 0x0159, 0x015C, 0x015E, 0x0160, 0x0163, 0x01CF, 0x01D0, 0x01D3, 0x01FF, 0x0203, 0x0205, 0x0207, 0x0209
			};
			return isFrame( id, EastFrames, sizeof( EastFrames ) );
		}

		bool isWestFrame( int id )
		{
			static int WestFrames[] =
			{
			0x0007, 0x000C, 0x001A, 0x001C, 0x0021, 0x0039, 0x0058, 0x0059, 0x005C, 0x005E, 0x0080, 0x0081, 0x0082, 0x0084, 0x0090, 0x0092, 0x0095, 0x0097, 0x0098, 0x00A6, 0x00A8, 0x00AD, 0x00AE, 0x00AF, 0x00B5, 0x00C7, 0x00C8, 0x00EA, 0x00F8, 0x00F9, 0x00FC, 0x00FE, 0x00FF, 0x0102, 0x0104, 0x0105, 0x0108, 0x0127, 0x0128, 0x012C, 0x012E, 0x0130, 0x0132, 0x0133, 0x0135, 0x0136, 0x0138, 0x013A, 0x014C, 0x014D, 0x014F, 0x0150, 0x0152, 0x0154, 0x0156, 0x0158, 0x0159, 0x015C, 0x015E, 0x0160, 0x0163, 0x01CF, 0x01D0, 0x01D3, 0x01FF, 0x0200, 0x0203, 0x0207, 0x0209
			};
			return isFrame( id, WestFrames, sizeof( WestFrames ) );
		}

		bool coordHasEastFrame( int x, int y, int z, int map )
		{
			StaticsIterator tiles = Maps::instance()->staticsIterator( Coord( x, y, z, map ), true );
			for ( ; !tiles.atEnd(); ++tiles )
			{
				if ( tiles.data().zoff == z && isEastFrame( tiles.data().itemid ) )
				{
					return true;
				}
			}
			return false;
		}

		bool coordHasSouthFrame( int x, int y, int z, int map )
		{
			StaticsIterator tiles = Maps::instance()->staticsIterator( Coord( x, y, z, map ), true );
			for ( ; !tiles.atEnd(); ++tiles )
			{
				if ( tiles.data().zoff == z && isSouthFrame( tiles.data().itemid ) )
				{
					return true;
				}
			}
			return false;
		}

		cItem* addDoor( int x, int y, int z, int map, DoorFacing facing )
		{
			//int doorTop = z + 20;

			if ( y == 1743 && x >= 1343 && x <= 1344 )
				return 0;
			if ( y == 1679 && x >= 1392 && x <= 1393 )
				return 0;
			if ( x == 1320 && y >= 1618 && y <= 1640 )
				return 0;
			if ( x == 1383 && y >= 1642 && y <= 1643 )
				return 0;
			if ( !Maps::instance()->canFit( x, y, z, map ) )
				return 0;
			cItem* door = cItem::createFromScript( QString::number( 0x6A5 + 2 * int( facing ), 16 ) );
			door->moveTo( Coord( x, y, z, map ) );
			return door;
		}
	public:

		int generate( int region[], int map, cUOSocket* )
		{
			int count = 0;
			for ( int rx = region[0]; rx < region[2]; ++rx )
			{
				for ( int ry = region[1]; ry < region[3]; ++ry )
				{
					StaticsIterator tiles = Maps::instance()->staticsIterator( map, rx, ry, true );
					for ( ; !tiles.atEnd(); ++tiles )
					{
						int id = tiles.data().itemid;
						int z = tiles.data().zoff;
						if ( isWestFrame( id ) )
						{
							if ( coordHasEastFrame( rx + 2, ry, z, map ) )
							{
								addDoor( rx + 1, ry, z, map, WestCW );
								++count;
							}
							else if ( coordHasEastFrame( rx + 3, ry, z, map ) )
							{
								cItem* first = addDoor( rx + 1, ry, z, map, WestCW );
								cItem* second = addDoor( rx + 2, ry, z, map, EastCCW );
								count += 2;
								if ( first && second )
								{
									first->setTag( "link", second->serial() );
									second->setTag( "link", first->serial() );
								}
								else
								{
									if ( !first && second )
									{
										second->remove();
										--count;
									}
									if ( !second && first )
									{
										first->remove();
										--count;
									}
								}
							}
						}
						else if ( isNorthFrame( id ) )
						{
							if ( coordHasSouthFrame( rx, ry + 2, z, map ) )
							{
								addDoor( rx, ry + 1, z, map, SouthCW );
								++count;
							}
							else if ( coordHasSouthFrame( rx, ry + 3, z, map ) )
							{
								cItem* first = addDoor( rx, ry + 1, z, map, NorthCCW );
								cItem* second = addDoor( rx, ry + 2, z, map, SouthCW );
								count += 2;
								if ( first && second )
								{
									first->setTag( "link", second->serial() );
									second->setTag( "link", first->serial() );
								}
								else
								{
									if ( !first && second )
									{
										second->remove();
										--count;
									}
									if ( !second && first )
									{
										first->remove();
										--count;
									}
								}
							}
						}
					}
				}
			}
			return count;
		}
	};

	DoorGenerator generator;

	int BritRegions[][4] =
	{
	{  250,  750,  775, 1330 }, {  525, 2095,  925, 2430 }, { 1025, 2155, 1265, 2310 }, { 1635, 2430, 1705, 2508 }, { 1775, 2605, 2165, 2975 }, { 1055, 3520, 1570, 4075 }, { 2860, 3310, 3120, 3630 }, { 2470, 1855, 3950, 3045 }, { 3425,  990, 3900, 1455 }, { 4175,  735, 4840, 1600 }, { 2375,  330, 3100, 1045 }, { 2100, 1090, 2310, 1450 }, { 1495, 1400, 1550, 1475 }, { 1085, 1520, 1415, 1910 }, { 1410, 1500, 1745, 1795 }, { 5120, 2300, 6143, 4095 }
	};
	/*
	int IlshRegions[][4] =
	{
	{ 0, 0, 288 * 8, 200 * 8 }
	};
	int MalasRegions[][4] =
	{
	{ 0, 0, 320 * 8, 256 * 8 }
	};
	*/

	socket->sysMessage( "Generating doors, please wait ( Slow )" );
	int count = 0;
	if ( Maps::instance()->hasMap( 0 ) )
	{
		for ( int i = 0; i < 16; ++i )
		{
			socket->sysMessage( QString( "doing [%1, %2, %3, %4]" ).arg( BritRegions[i][0] ).arg( BritRegions[i][1] ).arg( BritRegions[i][2] ).arg( BritRegions[i][3] ) );
			count += generator.generate( BritRegions[i], 0, socket );
			socket->sysMessage( tr( "Doors so far: %1" ).arg( count ) );
		}
	}
}

/*
	\command exportplayer
	\description Exports a playere and all his posessions to a flat file.
	\usage - exportplayer <serial>
	\notes This command exports a player with the given serial to a flat file named
	player-<serial>.bin.
*/
void commandExportPlayer( cUOSocket* socket, const QString& /*command*/, const QStringList& args ) throw()
{
	if (args.isEmpty()) {
		socket->sysMessage(tr("Usage: exportplayer <serial>"));
		return;
	}

	SERIAL serial = hex2dec(args[0]).toInt();
	P_PLAYER player = dynamic_cast<P_PLAYER>(World::instance()->findChar(serial));

	if (!player) {
		socket->sysMessage(tr("No player with the given serial could be found."));
		return;
	}

	QString filename = QString("player-%1.bin").arg(serial, 0, 16);

	socket->sysMessage(tr("Saving player 0x%1 to player-%2.bin").arg(serial, 0, 16).arg(serial, 0, 16));
	socket->log(tr("Saving player 0x%1 to player-%2.bin\n").arg(serial, 0, 16).arg(serial, 0, 16));

	try {
		cBufferedWriter writer("PLAYEREXPORT", World::instance()->getDatabaseVersion());
		writer.open(filename);
		((cBaseChar*)player)->save(writer);
		writer.writeByte(0xFF);
		writer.close();
	} catch(...) {
		socket->log(tr("Failed to export the player data.\n"));
	}
}

/*
	\command importplayer
	\description Imports a playere and all his posessions from a flat file.
	\usage - importplayer <filename>
	\notes This command imports a player from the given file.
*/
void commandImportPlayer( cUOSocket* socket, const QString& /*command*/, const QStringList& args ) throw()
{
	if (args.isEmpty()) {
		socket->sysMessage(tr("Usage: importplayer <filename>"));
		return;
	}

	QString filename = args[0];

	if (!QFile::exists(filename)) {
		socket->sysMessage(tr("No flat file with the given name could be found."));
		return;
	}
	socket->sysMessage(tr("Loading player from %1.").arg(filename));
	socket->log(tr("Loading player from %1.\n").arg(filename));

	try {
		cBufferedReader reader("PLAYEREXPORT", World::instance()->getDatabaseVersion());
		reader.open(filename);
		socket->sysMessage(tr("Loading %1 objects from %2.").arg(reader.objectCount()).arg(filename));

		unsigned char type;
		const QMap<unsigned char, QByteArray> &typemap = reader.typemap();
		QList<PersistentObject*> objects;

		do {
			type = reader.readByte();
			if (typemap.contains(type)) {
				PersistentObject *object = PersistentFactory::instance()->createObject(typemap[type]);
				if ( object )
				{
					try
					{
						object->load( reader );

						if ( reader.hasError() )
						{
							Console::instance()->log( LOG_ERROR, reader.error() );
							reader.setError( QString::null );

							cUObject *obj = dynamic_cast<cUObject*>( object );
							if ( obj )
							{
								obj->setSpawnregion( 0 );
								MapObjects::instance()->remove( obj );
								World::instance()->unregisterObject( obj );

								if ( obj->multi() )
								{
									obj->multi()->removeObject( obj );
									obj->setMulti( 0 );
								}
							}

							P_ITEM item = dynamic_cast<P_ITEM>( object );
							if ( item )
							{
								item->removeFromCont();
								item->setOwner( 0 );
							}

							P_NPC npc = dynamic_cast<P_NPC>( object );
							if ( npc )
							{
								npc->setOwner( 0 );
								npc->setAI( 0 );
							}
							delete object;
						}
						else
						{
							objects.append( object );
						}
					}
					catch ( wpException& e )
					{
						Console::instance()->log( LOG_WARNING, e.error() + "\n" );
					}
				}
				else
				{
					// Skip an unknown object type.
				}
			}
			else if ( type == 0xFA )
			{
				QString spawnregion = reader.readUtf8();
				SERIAL serial = reader.readInt();

				cSpawnRegion *region = SpawnRegions::instance()->region( spawnregion );
				cUObject *object = World::instance()->findObject( serial );
				if ( object && region )
				{
					object->setSpawnregion( region );
				}
			}
			else if ( type == 0xFC )
			{
				Timers::instance()->load( reader );
			}
			else if ( type == 0xFE )
			{
				cUObject *object = World::instance()->findObject( reader.readInt() );
				QString name = reader.readUtf8();
				cVariant variant;
				variant.serialize( reader, reader.version() );

				if ( object )
				{
					object->setTag( name, variant );
				}
			}
			else if ( type != 0xFF )
			{
				Console::instance()->log( LOG_ERROR, tr( "Invalid worldfile, unknown and unskippable type %1.\n" ).arg( type ) );
				return;
			}
		} while (type != 0xFF);

		reader.close();

		// post process all loaded objects
		QList<PersistentObject*>::const_iterator cit( objects.begin() );
		while ( cit != objects.end() )
		{
			( *cit )->postload( reader.version() );
			++cit;
		}
	} catch(...) {
		socket->log(tr("Failed to import the player data.\n"));
	}
}

// Clear ACLs
cCommands::~cCommands()
{
	qDeleteAll( _acls );
	_acls.clear();
}

class cFindPathTarget : public cTargetRequest {
public:
	bool responsed(cUOSocket *socket, cUORxTarget *target) {
		if (target->x() == 0xFFFF || target->y() == 0xFFFF) {
			socket->sysMessage("Please select a valid target.");
			return false;
		}

        Coord to(target->x(), target->y(), target->z(), socket->player()->pos().map);
		QList<unsigned char> path = Pathfinding::instance()->find(socket->player(), socket->player()->pos(), to);

		socket->sysMessage(QString("Found path with %1 nodes.").arg(path.size()));
		QStringList dirs;

		Coord coord = socket->player()->pos();

		// Show RecallRunes where the stupid thing is
		for (int i = 0; i < path.size(); ++i) {
			quint8 dir = path[i];
			dirs.append(QString::number(path[i]));

			Coord newcoord = Movement::instance()->calcCoordFromDir(dir, coord);
			mayWalk(socket->player(), newcoord);
			newcoord.effect(0x1f14, 1, 255, 0x26);
			coord = newcoord;
		}
		socket->sysMessage("Directions: " + dirs.join(", "));

		return true;
	}
};

/*
	Find a path
*/
void commandFindPath( cUOSocket* socket, const QString& /*command*/, const QStringList& /*args*/ ) throw()
{
	// attach a target to find the target
	socket->sysMessage("Select the target for the pathfinding calculation.");
	socket->attachTarget(new cFindPathTarget);
}

// Command Table (Keep this at the end)
stCommand cCommands::commands[] =
{
{ "ALLMOVE", commandAllMove },
{ "EXPORTDEFINITIONS", commandExportDefinitions },
{ "EXPORTPLAYER", commandExportPlayer },
{ "IMPORTPLAYER", commandImportPlayer },
{ "BROADCAST", commandBroadcast },
{ "DOORGEN", commandDoorGenerator },
{ "GMTALK", commandGmtalk },
{ "PAGENOTIFY", commandPageNotify },
{ "RELOAD", commandReload },
{ "SAVE", commandSave },
{ "SERVERTIME", commandServerTime },
{ "SET", commandSet },
{ "SHOW", commandShow },
{ "SHUTDOWN", commandShutDown },
{ "STAFF", commandStaff },
{ "SHOWSERIALS", commandShowserials },
{ "SPAWNREGION", commandSpawnRegion },
{ "WALKTEST", commandWalkTest },
{ "FINDPATH", commandFindPath },
{ NULL, NULL }
};
