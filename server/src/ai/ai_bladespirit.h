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

#if !defined(__AI_BLADESPIRIT_H__)

class Monster_BladeSpirit : public Monster_Aggressive
{
protected:
	Monster_BladeSpirit() : Monster_Aggressive()
	{
	}

public:
	Monster_BladeSpirit( P_NPC npc );

	static AbstractAI* create()
	{
		return new Monster_BladeSpirit( 0 );
	}

	static void registerInFactory()
	{
		AIFactory::instance()->registerType( "Monster_BladeSpirit", create );
	}

	virtual QString name() const
	{
		return "Monster_BladeSpirit";
	}

	void check();
	void NPCscheck();
	void ITEMscheck();

protected:
	virtual void selectVictim();
	P_CHAR findBestTarget();
};

#endif
