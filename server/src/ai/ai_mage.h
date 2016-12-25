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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition to that license, if you are running this program or modified
 * versions of it on a public system you HAVE TO make the complete source of
 * the version used by you available or provide people with a location to
 * download it.
 *
 * Wolfpack Homepage: http://www.hoogi.de/wolfpack/
 */

#if !defined(__AI_MAGE_H__)
#define __AI_MAGE_H__

class Monster_Mage : public Monster_Aggressive
{
protected:
	Monster_Mage() : Monster_Aggressive()
	{
	}

public:
	Monster_Mage( P_NPC npc );

	static AbstractAI* create()
	{
		return new Monster_Mage( 0 );
	}

	static void registerInFactory()
	{
		AIFactory::instance()->registerType( "Monster_Mage", create );
	}

	virtual QString name() const
	{
		return "Monster_Mage";
	}

protected:
	virtual void selectVictim();
};

#endif
