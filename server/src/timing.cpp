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

// Platform Includes
#include "platform.h"

#include "timing.h"
#include "profile.h"

#include "basics.h"
#include "timers.h"
#include "combat.h"
#include "console.h"
#include "mapobjects.h"
#include "serverconfig.h"
#include "network/network.h"
#include "spawnregions.h"
#include "territories.h"
#include "skills.h"
#include "typedefs.h"
#include "items.h"
#include "basechar.h"
#include "npc.h"
#include "player.h"
#include "ai/ai.h"
#include "inlines.h"
#include "walking.h"
#include "world.h"
#include "uotime.h"
#include "scriptmanager.h"

// Library Includes
#include <QDateTime>
#include <math.h>
#include <time.h>

cTiming::cTiming()
{
	unsigned int time = getNormalizedTime();

	lastWorldsave_ = 0;
	nextSpawnRegionCheck = time + Config::instance()->spawnRegionCheckTime() * MY_CLOCKS_PER_SEC;
	nextTamedCheck = ( uint ) ( time + Config::instance()->checkTamedTime() * MY_CLOCKS_PER_SEC );
	nextNpcCheck = ( uint ) ( time + Config::instance()->checkNPCTime() * MY_CLOCKS_PER_SEC );
	nextItemCheck = time + 10000; // Every 10 seconds
	nextShopRestock = time + 20 * 60 * MY_CLOCKS_PER_SEC; // Every 20 minutes
	nextCombatCheck = time + 100; // Every 100 ms
	nextUOTimeTick = 0;
	nextStormCheck = time + 5000; // Every 5s
	nextRayCheck = time + 500; // Every 500ms
	nextWeatherSound = time + Config::instance()->weatherSoundsInterval(); // Time to next Weather Sound

	currentday = 0xFF;
}

void cTiming::poll()
{
	unsigned int time = getNormalizedTime();

	// Check if Day is the same or not
	QDate today = QDate::currentDate();
	if ( currentday != today.day() )
	{
		if ( currentday != 0xFF )
		{
			currentday = today.day();
			cPythonScript* globalsh = ScriptManager::instance()->getGlobalHook( EVENT_REALDAYCHANGE );
			if ( globalsh )
				globalsh->callEventHandler( EVENT_REALDAYCHANGE );
		}
		else
			currentday = today.day();
	}

	// Check for spawn regions
	if ( nextSpawnRegionCheck <= time )
	{
		startProfiling( PF_SPAWNCHECK );
		SpawnRegions::instance()->check();
		nextSpawnRegionCheck = time + Config::instance()->spawnRegionCheckTime() * MY_CLOCKS_PER_SEC;
		stopProfiling( PF_SPAWNCHECK );
	}

	// Check for decay items
	if ( nextItemCheck <= time )
	{
		startProfiling( PF_DECAYCHECK );
		QList<SERIAL> toRemove;
		DecayIterator it = decayitems.begin();

		while ( it != decayitems.end() )
		{
			if ( ( *it ).first <= time )
			{
				toRemove.append( ( *it ).second );
			}
			++it;
		}

		foreach ( SERIAL s, toRemove )
		{
			P_ITEM item = FindItemBySerial( s );
			Coord point = item->pos();
			cTerritory *region = Territories::instance()->region( point );

			if ( item && item->isInWorld() && !item->nodecay() && ( item->corpse() || !item->multi() ) && !item->isLockedDown() && ( region && !region->isNoDecay() ) )
			{
				item->remove(); // Auto removes from the decaylist
			}
			else
			{
				removeDecaySerial( s );
			}
		}

		nextItemCheck = time + 5000;

		stopProfiling( PF_DECAYCHECK );
	}

	// Check for an automated worldsave
	if ( Config::instance()->saveInterval() )
	{
		startProfiling( PF_WORLDSAVE );

		// Calculate the next worldsave based on the last worldsave
		unsigned int nextSave = lastWorldsave() + Config::instance()->saveInterval() * MY_CLOCKS_PER_SEC;

		if ( nextSave <= time )
		{
			World::instance()->save();
		}

		stopProfiling( PF_WORLDSAVE );
	}

	unsigned int events = 0;
	bool loopitems = false;

	// Check lightlevel and time
	if ( nextUOTimeTick <= time )
	{
		startProfiling( PF_UOTIMECHECK );

		unsigned char oldhour = UoTime::instance()->hour();
		UoTime::instance()->setMinutes( UoTime::instance()->getMinutes() + 1 );
		nextUOTimeTick = time + Config::instance()->secondsPerUOMinute() * MY_CLOCKS_PER_SEC;

		unsigned char & currentLevel = Config::instance()->worldCurrentLevel();
		unsigned char newLevel = currentLevel;

		unsigned char darklevel = Config::instance()->worldDarkLevel();
		unsigned char brightlevel = Config::instance()->worldBrightLevel();
		unsigned char difference = wpMax<unsigned char>( 0, static_cast<int>( darklevel ) - brightlevel );
		unsigned char hour = UoTime::instance()->hour();

		if ( hour != oldhour )
		{
			events |= cBaseChar::EventTime;

			// OnTimeChange for Items
			if ( Config::instance()->enableTimeChangeForItems() )
				loopitems = true;

			// onServerHour Event
			cPythonScript* globalsh = ScriptManager::instance()->getGlobalHook( EVENT_SERVERHOUR );
			if ( globalsh )
				globalsh->callEventHandler( EVENT_SERVERHOUR );
		}

		// 11 to 18 = Day
		if ( hour >= 11 && hour < 18 )
		{
			newLevel = brightlevel;

			// 22 to 6 = Night
		}
		else if ( hour >= 22 || hour < 6 )
		{
			newLevel = darklevel;

			// 6 to 10 = Dawn (Scaled)
		}
		else if ( hour >= 6 && hour < 11 )
		{
			double factor = ( ( hour - 6 ) * 60 + UoTime::instance()->minute() ) / 240.0;
			newLevel = darklevel - wpMin<int>( darklevel, roundInt( factor * difference ) );

			// 18 to 22 = Nightfall (Scaled)
		}
		else
		{
			double factor = ( ( hour - 18 ) * 60 + UoTime::instance()->minute() ) / 240.0;
			newLevel = brightlevel + roundInt( factor * difference );
		}

		if ( newLevel != currentLevel )
		{
			events |= cBaseChar::EventLight;
			currentLevel = newLevel;
		}

		stopProfiling( PF_UOTIMECHECK );
	}

	if ( nextCombatCheck <= time )
	{
		startProfiling( PF_COMBATCHECK );

		nextCombatCheck = time + 250;

		// Check for timed out fights
		QList<cFightInfo*> fights = Combat::instance()->fights();
		QList<cFightInfo*> todelete;
		foreach ( cFightInfo* info, fights )
		{
			P_CHAR attacker = info->attacker();
			P_CHAR victim = info->victim();

			// These checks indicate that the fight is over.
			if ( !victim || victim->free || victim->isDead() || !attacker || attacker->free || attacker->isDead() || ( info->lastaction() + 60000 <= time && !attacker->inRange( victim, Config::instance()->attack_distance() ) ) )
			{
				todelete.append( info );
				continue;
			}

			attacker->poll( time, cBaseChar::EventCombat );

			// Maybe the victim got deleted already.
			if ( !victim->free )
			{
				victim->poll( time, cBaseChar::EventCombat );
			}
		}

		foreach ( cFightInfo* info, todelete )
		{
			delete info;
		}

		stopProfiling( PF_COMBATCHECK );
	}

	// Save the positions of connected players
	QList<Coord> positions;

	// Storms
	if ( ( Config::instance()->enableWeather() ) && ( nextStormCheck <= time ) )
	{
		// Loop to Clear old Storm Stuff
		QList<cUOSocket*> sockets = Network::instance()->sockets();
		foreach ( cUOSocket* socket, sockets )
		{
			socket->poll();
			if ( !socket )
			{
				continue;
			}

			if ( !socket->player() )
			{
				continue;
			}

			// Lets try TopRegion of Player
			cTerritory* region = socket->player()->region();

			if ( socket->player()->region()->parent() )
				region = dynamic_cast<cTerritory*>( socket->player()->region()->parent() );

			// Clear old Storm Stuff
			if ( region->stormchecked() )
				region->setStormChecked( 0 );
		}

		// Loop checking all Climatic Stuff
		foreach ( cUOSocket* socket, sockets )
		{
			socket->poll();
			if ( !socket )
			{
				continue;
			}

			if ( !socket->player() )
			{
				continue;
			}

			// Lets try TopRegion of Player
			cTerritory* region = socket->player()->region();

			if ( socket->player()->region()->parent() )
				region = dynamic_cast<cTerritory*>( socket->player()->region()->parent() );

			// Raining ?
			if ( region->isRaining() )
			{
				// Storm ?
				if ( region->intensity() > Config::instance()->intensityBecomesStorm() )
				{
					// Chances
					if ( !region->stormchecked() )
						region->setStormChecked( RandomNum( 1, 100 ) );

					// Get Actual Chance
					int chances = region->stormchecked();

					// % of Chances to a Thunder Sound
					if ( chances <= Config::instance()->defaultThunderChance() )
					{
						switch ( RandomNum( 0, 2 ) ) // Random Sound
						{
							case 0:
								socket->player()->soundEffect( 0x28, false );
								break;
							case 1:
								socket->player()->soundEffect( 0x29, false );
								break;
							case 2:
								socket->player()->soundEffect( 0x206, false );
								break;
						}
						// Ray Chance on Thunder
						if ( chances <= Config::instance()->rayChanceonThunder() )
							socket->flashray();
					}
				}
			}
		}

		// Assign next Storm Check
		nextStormCheck = time + 5000;
	}


	// Periodic checks for connected players
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( cUOSocket* socket, sockets )
	{
		socket->poll();
		if ( !socket )
		{
			continue;
		}

		if ( !socket->player() )
		{
			continue;
		}

		// Weather Sounds
		if ( ( Config::instance()->enableWeatherSounds() ) && ( nextWeatherSound <= time ) )
		{
			// Lets try TopRegion of Player
			cTerritory* region = socket->player()->region();

			if ( socket->player()->region()->parent() )
				region = dynamic_cast<cTerritory*>( socket->player()->region()->parent() );

			// Rain Sounds
			if ( region->isRaining() )
			{
				if ( region->intensity() > Config::instance()->intensityBecomesStorm() )
					socket->player()->soundEffect( 0x10, false );
				else
					socket->player()->soundEffect( 0x11, false );
			}

			// Snow Sound
			if ( region->isSnowing() )
			{
				if ( region->intensity() > Config::instance()->intensityBecomesStorm() )
					socket->player()->soundEffect( 0x16, false );
			}

			// Next Sound time
			nextWeatherSound = time + Config::instance()->weatherSoundsInterval();
		}

		// Lets Stop FlashRay if its enabled
		if ( nextRayCheck <= time )
		{
			if ( socket->tags().has( "flashray" ) )
			{
				socket->tags().remove( "flashray" );
				socket->updateLightLevel();
			}

			nextRayCheck = time + 500;
		}

		startProfiling( PF_PLAYERCHECK );
		socket->player()->poll( time, events );
		stopProfiling( PF_PLAYERCHECK );

		checkRegeneration( socket->player(), time );
		checkPlayer( socket->player(), time );
		positions.append( socket->player()->pos() );
	}

	// Check all other characters (Implementation of OnTimeChange Event too for NPCs)
	if ( ( nextNpcCheck <= time ) || ( events & cBaseChar::EventTime ) )
	{
		cCharIterator chariter;
		for ( P_CHAR character = chariter.first(); character; character = chariter.next() )
		{
			P_NPC npc = dynamic_cast<P_NPC>(character);
			if (npc) {
				npc->poll(time, events); // Poll combat separately (Invalidate pointers!)

				if ( npc->stablemasterSerial() == INVALID_SERIAL )
				{
					// Check if we are anywhere near a player
					// all other npcs are accounted as inactive
					for ( QList<Coord>::const_iterator it = positions.begin(); it != positions.end(); ++it )
					{
						if ( ( *it ).distance( npc->pos() ) <= 24 && !npc->pos().isInternalMap() )
						{
							startProfiling( PF_NPCCHECK );
							checkRegeneration( npc, time );
							checkNpc( npc, time );
							stopProfiling( PF_NPCCHECK );
							break;
						}
					}
					continue;
				}
			}

			P_PLAYER player = dynamic_cast<P_PLAYER>( character );
			if ( player && player->logoutTime() && player->logoutTime() <= time )
			{
				player->onLogout();
				player->removeFromView( false );
				player->setSocket( 0 );
				player->setLogoutTime( 0 );
				player->resend( false );
			}
		}

		if ( nextNpcCheck <= time )
		{
			if ( nextTamedCheck <= time )
				nextTamedCheck = ( uint ) ( time + Config::instance()->checkTamedTime() * MY_CLOCKS_PER_SEC );

			if ( nextNpcCheck <= time )
				nextNpcCheck = ( uint ) ( time + Config::instance()->checkNPCTime() * MY_CLOCKS_PER_SEC );
		}
	}

	// Check items with onTimeChange event if a hour passed
	if ( loopitems )
	{
		cItemIterator itemIterator;
		P_ITEM item;
		for ( item = itemIterator.first(); item; item = itemIterator.next() )
		{
			if ( item->canHandleEvent( EVENT_TIMECHANGE ) )
			{
				PyObject* args = Py_BuildValue( "(N)", item->getPyObject() );
				item->callEventHandler( EVENT_TIMECHANGE, args );
				Py_DECREF( args );
			}
		}
	}

	// Check the Timers
	startProfiling( PF_TIMERSCHECK );
	Timers::instance()->check();
	stopProfiling( PF_TIMERSCHECK );
}

void cTiming::checkRegeneration( P_CHAR character, unsigned int time )
{
	// Dead characters dont regenerate
	if ( character->isDead() )
	{
		return;
	}

	startProfiling( PF_REGENERATION );

	if ( character->regenHitpointsTime() <= time )
	{
		// If it's not disabled hunger affects our health regeneration
		if ( character->hitpoints() < character->maxHitpoints() )
		{
			if ( !Config::instance()->hungerRate() || character->hunger() > 10 )
			{
				// get next health regeneration time
				int hitsRegenTime = character->onTimerRegenHitpoints(( uint ) ( floor( character->getHitpointRate() * 1000 ) ));
				//unsigned int hitsRegenTime = character->getHitpointRate() * 1000;

				if ( hitsRegenTime )
				{
					if ( hitsRegenTime > 0 )
					{
						character->setHitpoints( character->hitpoints() + 1 );
						character->updateHealth();
						character->setRegenHitpointsTime( Server::instance()->time() + hitsRegenTime );
					}
					else
						character->setRegenHitpointsTime( Server::instance()->time() - hitsRegenTime );
				}
			}
		}
	}

	if ( character->regenStaminaTime() <= time )
	{
		if ( character->stamina() < character->maxStamina() )
		{
			// get next stamina regeneration time
			int stamRegenTime = character->onTimerRegenStamina(( uint ) ( floor( character->getStaminaRate() * 1000 ) ));
			//unsigned int stamRegenTime = character->getStaminaRate() * 1000;

			if ( stamRegenTime )
			{
				if ( stamRegenTime > 0 )
				{
					character->setStamina( character->stamina() + 1 );
					character->setRegenStaminaTime( Server::instance()->time() + stamRegenTime );

					P_PLAYER player = dynamic_cast<P_PLAYER>( character );
					if ( player && player->socket() )
					{
						player->socket()->updateStamina();
					}
				}
				else
					character->setRegenStaminaTime( Server::instance()->time() - stamRegenTime );
			}
		}
	}

	if ( character->regenManaTime() <= time )
	{
		if ( character->mana() < character->maxMana() )
		{
			// get next mana regeneration time
			int manaRegenTime = character->onTimerRegenMana(( uint ) ( floor( character->getManaRate() * 1000 ) ));
			//unsigned int manaRegenTime = character->getManaRate() * 1000;

			if ( manaRegenTime )
			{
				if ( manaRegenTime > 0 )
				{
					character->setMana( character->mana() + 1 );
					character->setRegenManaTime( Server::instance()->time() + manaRegenTime );

					P_PLAYER player = dynamic_cast<P_PLAYER>( character );
					if ( player )
					{
						if ( player->socket() )
						{
							player->socket()->updateMana();
						}

						if ( player->isMeditating() && character->mana() >= character->maxMana() )
						{
							player->setMeditating( false );
							player->sysmessage( 501846 );
						}
					}
				}
				else
					character->setRegenManaTime( Server::instance()->time() - manaRegenTime );
			}
		}
	}

	stopProfiling( PF_REGENERATION );
}

void cTiming::checkPlayer( P_PLAYER player, unsigned int time )
{
	startProfiling( PF_PLAYERCHECK );

	cUOSocket* socket = player->socket();

	// Criminal Flagging
	if ( player->criminalTime() != 0 && player->criminalTime() <= time )
	{
		socket->sysMessage( tr( "You are no longer criminal." ) );
		player->setCriminalTime( 0 );
	}

	// Murder Decay
	if ( Config::instance()->murderdecay() > 0 )
	{
		if ( player->murdererTime() > 0 && player->murdererTime() < time )
		{
			if ( player->kills() > 0 )
				player->setKills( player->kills() - 1 );

			if ( player->kills() == Config::instance()->maxkills() && Config::instance()->maxkills() > 0 )
			{
				socket->sysMessage( tr( "You are no longer a murderer." ) );
				player->setMurdererTime( time + Config::instance()->murderdecay() * MY_CLOCKS_PER_SEC );
			} else if ( player->kills() == 0 ) {
				player->setMurdererTime( 0 );
			} else {
				player->setMurdererTime( time + Config::instance()->murderdecay() * MY_CLOCKS_PER_SEC );
			}
		}
	}

	stopProfiling( PF_PLAYERCHECK );
}

void cTiming::checkNpc( P_NPC npc, unsigned int time )
{
	// Remove summoned npcs
	if ( npc->summoned() && npc->summonTime() <= time && npc->summonTime() )
	{
		// Make pooofff and sheeesh
		npc->soundEffect( 0x1fe );
		npc->pos().effect( 0x3735, 10, 30 );
		npc->remove();
		return;
	}

	// Give the AI time to process events
	if ( npc->ai() && npc->aiCheckTime() <= time )
	{
		startProfiling( PF_AICHECK );
		npc->ai()->check();
		stopProfiling( PF_AICHECK );
	}

	// Loop to check NPCs and Items
	if ( npc->aiNpcsCheckTime() <= time )
	{
		if ( npc->ai() && npc->aiNpcsCheckTime() <= time )
		{
			startProfiling( PF_AICHECKNPCS );
			npc->ai()->NPCscheck();
			stopProfiling( PF_AICHECKNPCS );
		}
	}
	if ( npc->aiItemsCheckTime() <= time )
	{
		if ( npc->ai() && npc->aiItemsCheckTime() <= time )
		{
			startProfiling( PF_AICHECKITEMS );
			npc->ai()->ITEMscheck();
			stopProfiling( PF_AICHECKITEMS );
		}
	}
	/*
	// Hunger for npcs
	// This only applies to tamed creatures
	if ( npc->isTamed() && Config::instance()->hungerRate() && npc->hungerTime() <= time )
	{
		// Creatures owned by GMs won't hunger.
		if ( !npc->owner() || !npc->owner()->isGMorCounselor() )
		{
			if ( npc->hunger() )
			{
				npc->setHunger( npc->hunger() - 1 );
			}

			npc->setHungerTime( time + Config::instance()->hungerRate() * 60 * MY_CLOCKS_PER_SEC );

			switch ( npc->hunger() )
			{
			case 12:
				npc->emote( tr( "*%1 looks a little hungry*" ).arg( npc->name() ), 0x26 );
				break;
			case 9:
				npc->emote( tr( "*%1 looks fairly hungry*" ).arg( npc->name() ), 0x26 );
				break;
			case 6:
				npc->emote( tr( "*%1 looks extremely hungry*" ).arg( npc->name() ), 0x26 );
				break;
			case 3:
				npc->emote( tr( "*%1 looks weak from starvation*" ).arg( npc->name() ), 0x26 );
				break;
			case 0:
				npc->setWanderType( enFreely );
				npc->setTamed( false );

				if ( npc->owner() )
				{
					npc->setOwner( 0 );
				}

				npc->bark( cBaseChar::Bark_Attacking );
				npc->talk( 1043255, npc->name(), 0, false, 0x26 );

				if ( Config::instance()->tamedDisappear() == 1 )
				{
					npc->soundEffect( 0x1FE );
					npc->remove();
				}
				break;
			}
		}
	}
	*/
}

void cTiming::addDecayItem( P_ITEM item )
{
	unsigned int delay = item->decayDelay();

	if ( delay )
	{
		DecayPair pair( Server::instance()->time() + delay, item->serial() );
		decayitems.append( pair );
	}
}

void cTiming::removeDecayItem( P_ITEM item )
{
	DecayIterator it;
	register SERIAL serial = item->serial();
	for ( it = decayitems.begin(); it != decayitems.end(); ++it )
	{
		if ( ( *it ).second == serial )
		{
			decayitems.removeAll( *it );
			return;
		}
	}
}

void cTiming::removeDecaySerial( SERIAL serial )
{
	DecayIterator it;
	for ( it = decayitems.begin(); it != decayitems.end(); ++it )
	{
		if ( ( *it ).second == serial )
		{
			decayitems.removeAll( *it );
			return;
		}
	}
}
