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

#if !defined(__TIMING_H__)
#define __TIMING_H__

#include "singleton.h"
#include "typedefs.h"
#include "objectdef.h"

#include <QPair>
#include <QList>

class cTiming
{
	OBJECTDEF( cTiming )
private:
	unsigned int nextSpawnRegionCheck;
	unsigned int nextLightCheck;
	unsigned int nextTamedCheck;
	unsigned int nextNpcCheck;
	unsigned int nextItemCheck;
	unsigned int nextShopRestock;
	unsigned int nextCombatCheck;
	unsigned int nextUOTimeTick;
	unsigned int nextStormCheck;
	unsigned int nextRayCheck;
	unsigned int nextWeatherSound;

	unsigned char currentday;

protected:
	unsigned int lastWorldsave_;
	void checkRegeneration( P_CHAR character, unsigned int time );
	void checkPlayer( P_PLAYER player, unsigned int time );
	void checkNpc( P_NPC npc, unsigned int time );

	typedef QPair<unsigned int, SERIAL> DecayPair;
	typedef QList<DecayPair> DecayContainer;
	typedef DecayContainer::iterator DecayIterator;

	DecayContainer decayitems;
public:
	cTiming();

	/*!
		\brief Process periodic events.
	*/
	void poll();

	/*!
		\returns The time the world was saved last.
	*/
	inline unsigned int lastWorldsave()
	{
		return lastWorldsave_;
	}

	/*!
		\brief Sets the time the world was saved last.
		\param data The new time.
	*/
	inline void setLastWorldsave( unsigned int data )
	{
		lastWorldsave_ = data;
	}

	// Let an item decay
	void addDecayItem( P_ITEM item );
	void removeDecayItem( P_ITEM item );
	void removeDecaySerial( SERIAL item );
};

typedef Singleton<cTiming> Timing;

#endif
