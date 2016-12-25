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

#if !defined(__COMBAT_H__)
#define __COMBAT_H__

#include "singleton.h"
#include "typedefs.h"
#include <QList>

class Coord;
class cUOSocket;

/*!
	\brief This structure contains information about an ongoing fight.
*/
class cFightInfo
{
protected:
	/*!
		\brief A pointer to the character who attacked first.
	*/
	P_CHAR attacker_;

	/*!
		\brief A pointer to the character who was attacked.
	*/
	P_CHAR victim_;

	/*!
		\brief Specifies whether attacking the victim was not a crime.
		If this value is true, no kill is awarded to the winner of this fight.
	*/
	bool legitimate_;

	/*!
		\brief The amount of damage the attacker dealt in this fight.
		This value is used for looting rights.
	*/
	unsigned int attackerDamage_;

	/*!
		\brief The amount of damage the victim dealt in this fight.
		This value is used for looting rights.
	*/
	unsigned int victimDamage_;

	/*!
		\brief The servertime the last action in this fight was taken.
		This value is used to time out the fight after no action was taken
		for a while.
	*/
	unsigned int lastaction_;
public:
	/*!
		\brief Destructor for the fight object.
	*/
	~cFightInfo();

	/*!
		\returns The attacker of this fight.
	*/
	inline P_CHAR attacker()
	{
		return attacker_;
	}

	/*!
		\returns The victim of this fight.
	*/
	inline P_CHAR victim()
	{
		return victim_;
	}

	/*!
		\returns If the attacker committed a crime when attacking.
	*/
	inline bool legitimate()
	{
		return legitimate_;
	}

	/*!
		\returns The damage dealt to the attacker of this fight.
	*/
	inline unsigned int attackerDamage()
	{
		return attackerDamage_;
	}

	/*!
		\returns The damage dealt to the victim of this fight.
	*/
	inline unsigned int victimDamage()
	{
		return victimDamage_;
	}

	/*!
		\returns The time of the last action in this fight.
	*/
	inline unsigned int lastaction()
	{
		return lastaction_;
	}

	/*!
		\brief Constructor for the fight object.
		Creates a fight information object and registers it with the combat system.
		\param attacker The attacker of the fight.
		\param victim The victim of this fight.
		\param legitimate If the fight was legitimate or not.
	*/
	cFightInfo( P_CHAR attacker, P_CHAR victim, bool legitimate );

	/*!
		\brief Refresh this fights lastaction value.
	*/
	void refresh();

	/*!
		\brief Clear this info structure so it can be deleted
		by the next combat check.
	*/
	void clear();
};

/*!
	\brief This class encapsulates all combat related information and methods.
*/
class cCombat
{
protected:
	/*!
		\brief This member saves all ongoing fights and is used to time out fights.
	*/
	QList<cFightInfo*> fights_;

public:
	/*!
		\returns A reference to the list containing all ongoing fights.
	*/
	inline QList<cFightInfo*>& fights()
	{
		return fights_;
	}

	// Sounds + Animations
	void playGetHitSoundEffect( P_CHAR pChar );
	void playGetHitAnimation( P_CHAR pChar );
	void spawnGuard( P_CHAR pOffender, P_CHAR pCaller, const Coord& pos );
};

typedef Singleton<cCombat> Combat;

#endif // __COMBAT_H__

