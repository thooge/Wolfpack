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

#include "ai.h"
#include "ai_energyvortex.h"
#include "../npc.h"
#include "../player.h"
#include "../mapobjects.h"
#include "../combat.h"
#include "../inlines.h"
#include "../walking.h"
#include "../serverconfig.h"

P_CHAR Monster_EnergyVortex::findBestTarget()
{
	int targetAttract = -1;
	P_CHAR target = 0;

	if ( !m_npc )
	{
		return target;
	}

	// Search for targets in our list of current targets first
	QList<cFightInfo*> fights = m_npc->fights();
	foreach ( cFightInfo* info, fights )
	{
		P_CHAR victim = info->victim();
		if ( victim == m_npc )
		{
			victim = info->attacker();
		}

		// We don't already attack the target, right?
		if ( victim != target )
		{
			// See if it's a target we want
			unsigned int dist = m_npc->dist(victim);
			int attract = (victim->intelligence() + victim->skillValue(MAGERY) / 10) / (wpMin<unsigned int>(65535, dist) + 1);
			if ( attract > targetAttract && validTarget( victim, dist ) ) {
				target = victim;
				targetAttract = attract;
			}
		}
	}

	/*
		Attack the target with the highest ((Strength + Tactics) / Distance) value.
	*/

	// If we're not tamed, we attack other players as well.
	if ( !m_npc->isTamed() )
	{
		MapCharsIterator ri = MapObjects::instance()->listCharsInCircle( m_npc->pos(), 6 );
		for ( P_CHAR pChar = ri.first(); pChar; pChar = ri.next() )
		{
			// We limit ourself to players and pets owned by players.
			P_PLAYER victim = dynamic_cast<P_PLAYER>( pChar );
			P_NPC npcVictim = dynamic_cast<P_NPC>( pChar );

			// We don't already attack the target, right?
			if ( victim && victim != target )
			{
				// See if it's a target we want
				unsigned int dist = m_npc->dist(victim);
				int attract = (victim->intelligence() + victim->skillValue(MAGERY) / 10) / (wpMin<unsigned int>(65535, dist) + 1);
				if ( attract > targetAttract && validTarget( victim, dist ) ) {
					target = victim;
					targetAttract = attract;
				}
			}
			else if ( npcVictim && npcVictim->owner() && npcVictim != target )
			{
				// See if it's a target we want
				unsigned int dist = m_npc->dist(victim);
				int attract = (npcVictim->intelligence() + npcVictim->skillValue(MAGERY) / 10) / (wpMin<unsigned int>(65535, dist) + 1);
				if ( attract > targetAttract && validTarget( npcVictim, dist ) ) {
					target = npcVictim;
					targetAttract = attract;
				}
			}
		}
	}

	return target;
}

Monster_EnergyVortex::Monster_EnergyVortex( P_NPC npc ) : Monster_Aggressive( npc )
{
	m_actions.append( new Action_MoveToTarget( npc, this ) );
	m_actions.append( new Monster_Aggr_Fight( npc, this ) );
}

void Monster_EnergyVortex::check()
{
	// Our current victim
	P_CHAR m_currentVictim = World::instance()->findChar( m_currentVictimSer );
	if ( !m_currentVictim )
	{
		m_currentVictimSer = INVALID_SERIAL;
	}

	if ( m_currentVictim && invalidTarget( m_currentVictim ) ) {
		m_currentVictim = 0;
		m_currentVictimSer = INVALID_SERIAL;
		m_npc->fight( 0 );
	}

	if ( nextVictimCheck < Server::instance()->time() )
	{
		// Don't switch if we can hit it...
		if ( !m_currentVictim || m_currentVictim->dist( m_npc ) > 1 )
		{
			P_CHAR target = findBestTarget();
			if ( target )
			{
				m_currentVictim = target;
				m_currentVictimSer = target->serial();
				m_npc->fight( target );
			}
		}

		nextVictimCheck = Server::instance()->time() + 1500;
	}

	AbstractAI::check();
}

void Monster_EnergyVortex::NPCscheck()
{
	AbstractAI::NPCscheck();
}

void Monster_EnergyVortex::ITEMscheck()
{
	AbstractAI::ITEMscheck();
}

void Monster_EnergyVortex::selectVictim() {
}

