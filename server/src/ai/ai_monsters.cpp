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
#include "../combat.h"
#include "../npc.h"
#include "../factory.h"
#include "../player.h"
#include "../mapobjects.h"
#include "../serverconfig.h"
#include "../basics.h"
#include "../items.h"

// library includes
#include <math.h>

// Find the best target for this NPC
P_CHAR Monster_Aggressive::findBestTarget()
{
	unsigned int distance = ~0;
	P_CHAR target = 0;

	if ( !m_npc )
	{
		return target;
	}

	// Search for targets in our list of current targets first
	QList<cFightInfo*> fights( m_npc->fights() );
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
			unsigned int dist = m_npc->dist( victim );
			if ( dist < distance && validTarget( victim, dist, false ) )
			{
				target = victim;
				distance = dist;
			}
		}
	}

	// If we're not tamed, we attack other players as well.
	if ( !m_npc->isTamed() )
	{
		MapCharsIterator ri = MapObjects::instance()->listCharsInCircle( m_npc->pos(), VISRANGE );
		for ( P_CHAR pChar = ri.first(); pChar; pChar = ri.next() )
		{
			// We limit ourself to players and pets owned by players.
			P_PLAYER victim = dynamic_cast<P_PLAYER>( pChar );
			P_NPC npcVictim = dynamic_cast<P_NPC>( pChar );

			// We don't already attack the target, right?
			if ( victim && victim != target )
			{
				// See if it's a target we want
				unsigned int dist = m_npc->dist( victim );
				if ( dist < distance && validTarget( victim, dist, true ) )
				{
					target = victim;
					distance = dist;
				}
			}
			else if ( npcVictim && npcVictim->owner() && npcVictim != target )
			{
				// See if it's a target we want
				unsigned int dist = m_npc->dist( npcVictim );
				if ( dist < distance && validTarget( npcVictim, dist, true ) )
				{
					target = npcVictim;
					distance = dist;
				}
			}
		}
	}

	return target;
}

void Monster_Aggressive::check()
{
	// Our current victim
	P_CHAR m_currentVictim = World::instance()->findChar( m_currentVictimSer );
	if ( !m_currentVictim )
	{
		m_currentVictimSer = INVALID_SERIAL;
	}

	if ( m_currentVictim && invalidTarget( m_currentVictim ) )
	{
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

void Monster_Aggressive::NPCscheck()
{
	AbstractAI::NPCscheck();
}

void Monster_Aggressive::ITEMscheck()
{
	AbstractAI::ITEMscheck();
}

void Monster_Aggressive_L0::registerInFactory()
{
#ifndef __VC6
	AIFactory::instance()->registerType( "Monster_Aggressive_L0", productCreatorFunctor<Monster_Aggressive_L0> );
#else
	AIFactory::instance()->registerType( "Monster_Aggressive_L0", productCreatorFunctor_Monster_Aggressive_L0 );
#endif
}

void Monster_Aggressive_L0::selectVictim()
{
}

void Monster_Berserk::registerInFactory()
{
#ifndef __VC6
	AIFactory::instance()->registerType( "Monster_Berserk", productCreatorFunctor<Monster_Berserk> );
#else
	AIFactory::instance()->registerType( "Monster_Berserk", productCreatorFunctor_Monster_Berserk );
#endif
}

void Monster_Berserk::selectVictim()
{
}

void Monster_Aggressive_L1::registerInFactory()
{
#ifndef __VC6
	AIFactory::instance()->registerType( "Monster_Aggressive_L1", productCreatorFunctor<Monster_Aggressive_L1> );
#else
	AIFactory::instance()->registerType( "Monster_Aggressive_L1", productCreatorFunctor_Monster_Aggressive_L1 );
#endif
}

void Monster_Aggressive_L1::selectVictim()
{
}

float Monster_Aggr_Fight::preCondition()
{
	/*
	 * Fighting the target has the following preconditions:
	 * - A target has been set.
	 * - The target is not dead.
	 * - The NPC is in combat range.
	 *
	 * Here we take the fuzzy logic into account.
	 * If the npc is injured, the chance of fighting will decrease.
	 */

	Monster_Aggressive* pAI = dynamic_cast<Monster_Aggressive*>( m_ai );

	if ( !pAI || !pAI->currentVictim() || pAI->currentVictim()->isDead() )
		return 0.0f;

	quint8 range = 1;
	P_ITEM weapon = m_npc->getWeapon();
	if ( weapon )
	{
		if ( weapon->hasTag( "range" ) )
		{
			range = weapon->getTag( "range" ).toInt();
		}
		else if ( weapon->basedef() )
		{
			range = weapon->basedef()->getIntProperty( "range", 1 );
		}
	}

	if ( !m_npc->inRange( pAI->currentVictim(), range ) )
		return 0.0f;

	// 1.0 = Full Health, 0.0 = Dead
	float diff = 1.0 - wpMax<float>( 0, ( m_npc->maxHitpoints() - m_npc->hitpoints() ) / ( float ) m_npc->maxHitpoints() );

	if ( diff <= m_npc->criticalHealth() / 100.0 )
	{
		return 0.0;
	}

	return 1.0;
}

float Monster_Aggr_Fight::postCondition()
{
	/*
	 * Fighting the target has the following postconditions:
	 * - The target is not set anymore.
	 * - The NPC is not within fight range.
	 * - The target is dead.
	 * - The NPC is injured above the criticial line.
	 */

	Monster_Aggressive* pAI = dynamic_cast<Monster_Aggressive*>( m_ai );
	if ( !pAI || !pAI->currentVictim() || pAI->currentVictim()->isDead() )
		return 1.0f;

	quint8 range = 1;
	P_ITEM weapon = m_npc->getWeapon();
	if ( weapon )
	{
		if ( weapon->hasTag( "range" ) )
		{
			range = weapon->getTag( "range" ).toInt();
		}
		else if ( weapon->basedef() )
		{
			range = weapon->basedef()->getIntProperty( "range", 1 );
		}
	}

	if ( !m_npc->inRange( pAI->currentVictim(), range ) )
	{
#if defined(AIDEBUG)
		m_npc->talk( "[COMBAT: Not In Range]" );
#endif
		return 1.0f;
	}

	// 1.0 = Full Health, 0.0 = Dead
	float diff = 1.0 - wpMax<float>( 0, ( m_npc->maxHitpoints() - m_npc->hitpoints() ) / ( float ) m_npc->maxHitpoints() );

	if ( diff <= m_npc->criticalHealth() / 100.0 )
	{
		return 1.0;
	}

	return 0.0;
}

void Monster_Aggr_Fight::execute()
{
	Monster_Aggressive* ai = dynamic_cast<Monster_Aggressive*>( m_ai );

	if ( !m_npc->attackTarget() )
	{
		m_npc->fight( ai->currentVictim() );
	}
}
