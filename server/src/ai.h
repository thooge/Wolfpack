//==================================================================================
//
//      Wolfpack Emu (WP)
//	UO Server Emulation Program
//
//  Copyright 2001-2003 by holders identified in authors.txt
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Palace - Suite 330, Boston, MA 02111-1307, USA.
//
//	* In addition to that license, if you are running this program or modified
//	* versions of it on a public system you HAVE TO make the complete source of
//	* the version used by you available or provide people with a location to
//	* download it.
//
//
//
//	Wolfpack Homepage: http://wpdev.sf.net/
//==================================================================================

/*==============================================================================
This class builds a sample state machine for NPC AI
==============================================================================*/

#ifndef AI_H_HEADER_INCLUDED
#define AI_H_HEADER_INCLUDED

// platform includes
#include "platform.h"

// library includes
#include <qstring.h>

// wolfpack includes
#include "typedefs.h"

class AbstractState;
class Coord_cl;

class cNPC_AI
{
public:
	cNPC_AI() {}
	~cNPC_AI() { delete currentState; }
	
	virtual void eventHandler() = 0;
	virtual void updateState();

	virtual QString AIType() = 0;
	P_NPC	npc;

	AbstractState*	currState() const { return currentState; }
protected:
	AbstractState*	currentState;
};

class Monster_Aggressive_L0 : public cNPC_AI
{
public:
	Monster_Aggressive_L0( P_NPC currnpc );

	virtual void eventHandler();

	virtual QString AIType() { return "Monster_Aggressive_L0"; }
};

class Actions
{
public:
	Actions() { waitForPathCalculation = 0; }
	~Actions() {}

protected:
	virtual void attack( P_NPC npc );
	virtual void reattack( P_NPC npc );
	virtual void reset() {}
	virtual void flee() {}
	virtual void greet() {}
	virtual void showTargetCursor() {}
	virtual void decline() {}
	virtual void callGuards() {}
	virtual void wanderFreely( P_NPC npc );
	virtual void moveTo( P_NPC npc, const Coord_cl &pos );
	virtual void movePath( P_NPC npc, const Coord_cl &pos );

	UINT8 waitForPathCalculation;
};

class Events
{
public:
	Events() {}
	~Events() {}

	virtual void attacked() {}
	virtual void won() {}
	virtual void combatCancelled() {}
	virtual void hitpointsCritical() {}
	virtual void hitpointsRestored() {}
	virtual void speechInput() {}
	virtual void targetCursorInput() {}
	virtual void pay() {}
	virtual void foundVictim() {}
};

class AbstractState : public Actions, public Events
{
public:
	AbstractState() {}
	AbstractState( cNPC_AI* interface_ ) { m_interface = interface_; }
	~AbstractState() {}

	virtual QString stateType() = 0;

	AbstractState* nextState;

	virtual void execute() {}

protected:
	cNPC_AI*	m_interface;
	
	virtual void setInterface( cNPC_AI* newInterface ) { m_interface = newInterface; }
};

class Monster_Aggr_L0_Wander : public AbstractState
{
public:
	Monster_Aggr_L0_Wander( cNPC_AI* interface_ ) { m_interface = interface_; }
	~Monster_Aggr_L0_Wander() {}

	virtual QString stateType() { return "Monster_Aggr_L0_Wander"; }
	virtual void execute();

	// events handled
	virtual void attacked();
	virtual void foundVictim();
};

class Monster_Aggr_L0_Combat : public AbstractState
{
public:
	Monster_Aggr_L0_Combat( cNPC_AI* interface_ ) { m_interface = interface_; }
	~Monster_Aggr_L0_Combat() {}

	virtual QString stateType() { return "Monster_Aggr_L0_Combat"; }
	virtual void execute();

	// events handled
	virtual void won();
	virtual void combatCancelled();
};

#endif /* AI_H_HEADER_INCLUDED */