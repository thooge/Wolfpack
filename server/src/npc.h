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

#ifndef CNPC_H_HEADER_INCLUDED
#define CNPC_H_HEADER_INCLUDED

// platform includes
#include "platform.h"

// library includes
#include <QList>

// wolfpack includes
#include "basechar.h"
#include "objectdef.h"

class AbstractAI;

// Class for Non Player Characters. Implements cBaseChar.
class cNPC : public cBaseChar
{
	OBJECTDEF( cNPC )
public:
	const char* objectID() const
	{
		return "cNPC";
	}

	// con-/destructors
	cNPC();
	cNPC( const cNPC& right );
	virtual ~cNPC();
	// operators
	cNPC& operator=( const cNPC& right );

	// type definitions
	struct stWanderType
	{
		// constructors
		stWanderType() : type( enHalt )
		{
			radius = x1 = x2 = y1 = y2 = 0;
			followTarget = 0;
		}
		stWanderType( enWanderTypes type_ ) : type( type_ )
		{
			radius = x1 = x2 = y1 = y2 = 0;
			followTarget = 0;
		}
		stWanderType( quint16 x1_, quint16 x2_, quint16 y1_, quint16 y2_ ) : type( enRectangle ), x1( x1_ ), x2( x2_ ), y1( y1_ ), y2( y2_ )
		{
			radius = 0;
			followTarget = 0;
		}
		stWanderType( quint16 x_, quint16 y_, quint16 radius_ ) : type( enCircle ), x1( x_ ), y1( y_ ), radius( radius_ )
		{
			x2 = y2 = 0;
			followTarget = 0;
		}

		// attributes
		enWanderTypes type;
		// rectangles and circles
		quint16 x1;
		quint16 x2;
		quint16 y1;
		quint16 y2;
		quint16 radius;

		P_CHAR followTarget;
		Coord destination;
	};

	// implementation of interfaces
	void load( QSqlQuery&, ushort& );
	void save();
	bool del();
	void load( cBufferedReader& reader, unsigned int version );
	void save( cBufferedWriter& writer, unsigned int version );
	void postload( unsigned int version );
	void load( cBufferedReader& reader );

	bool isOverloaded();
	unsigned int maxWeight();
	virtual enCharTypes objectType();
	virtual void update( bool excludeself = false );
	virtual void resend( bool clean = true );
	virtual void talk( const QString& message, UI16 color = 0xFFFF, quint8 type = 0, bool autospam = false, cUOSocket* socket = NULL );
	virtual void talk( uint MsgID, const QString& params = 0, const QString& affix = 0, bool prepend = false, ushort color = 0xFFFF, cUOSocket* socket = 0 );
	virtual quint8 notoriety( P_CHAR pChar = 0 );
	virtual void showName( cUOSocket* socket );
	virtual void soundEffect( UI16 soundId, bool hearAll = true );
	virtual bool inWorld();
	virtual void giveGold( quint32 amount, bool inBank = false );
	virtual quint32 takeGold( quint32 amount, bool inBank = false );
	virtual void applyDefinition( const cElement* );
	virtual void flagUnchanged()
	{
		cNPC::changed_ = false; cBaseChar::flagUnchanged();
	}
	void log( eLogLevel, const QString& string );
	void log( const QString& string );
	void awardFame( short amount, bool showmessage = true );
	void awardKarma( P_CHAR pKilled, short amount, bool showmessage = true );
	void vendorBuy( P_PLAYER player );
	static cNPC* createFromScript( const QString& id, const Coord& pos );
	void remove();
	void vendorSell( P_PLAYER player );
	virtual bool isInnocent();
	void createTooltip( cUOTxTooltipList& tooltip, cPlayer* player );
	unsigned int damage( eDamageType type, unsigned int amount, cUObject* source = 0 );
	void poll( unsigned int time, unsigned int events );

	// other public methods
	virtual stError* setProperty( const QString& name, const cVariant& value );
	PyObject* getProperty( const QString& name, uint hash = 0 );
	void setNextMoveTime( bool changeDirection = true );
	virtual void callGuards(); // overriding
	void makeShop();

	virtual void moveTo( const Coord& pos );

	virtual bool onReachDestination();

	// getters
	quint32 additionalFlags() const;
	quint32 nextBeggingTime() const;
	quint32 nextGuardCallTime() const;
	quint32 nextMoveTime() const;
	quint32 nextMsgTime() const;
	quint32 summonTime() const;
	P_PLAYER owner() const;
	SERIAL stablemasterSerial() const;
	AbstractAI* ai() const;
	quint32 aiCheckTime() const;
	quint32 aiNpcsCheckTime() const;
	quint32 aiItemsCheckTime() const;
	quint16 aiCheckInterval() const;
	bool summoned() const;
	// advanced getters for data structures
	// path finding
	bool hasPath( void );
	Coord nextMove();
	Coord pathDestination( void ) const;
	float pathHeuristic( const Coord& source, const Coord& destination );
	// wander type
	enWanderTypes wanderType() const;
	quint16 wanderX1() const;
	quint16 wanderX2() const;
	quint16 wanderY1() const;
	quint16 wanderY2() const;
	quint16 wanderRadius() const;
	P_CHAR wanderFollowTarget() const;
	Coord wanderDestination() const;

	// setters
	void setAdditionalFlags( quint32 data );
	void setNextBeggingTime( quint32 data );
	void setNextGuardCallTime( quint32 data );
	void setNextMoveTime( quint32 data );
	void setNextMsgTime( quint32 data );
	void setSummonTime( quint32 data );
	void setOwner( P_PLAYER data, bool nochecks = false );
	void setSummoned( bool data );
	void setStablemasterSerial( SERIAL data );
	void setGuarding( P_PLAYER data );
	void setAI( AbstractAI* ai );
	void setAICheckTime( quint32 data );
	void setAICheckInterval( quint16 data );
	void setAINpcsCheckTime( quint32 data );
	void setAIItemsCheckTime( quint32 data );

	// advanced setters for data structures
	// AI
	void setAI( const QString& data );
	// path finding
	void pushMove( const Coord& move );
	void pushMove( UI16 x, UI16 y, SI08 z );
	void popMove( void );
	void clearPath( void );
	void findPath( const Coord& goal );
	// wander type
	void setWanderType( enWanderTypes data );
	void setWanderX1( quint16 data );
	void setWanderX2( quint16 data );
	void setWanderY1( quint16 data );
	void setWanderY2( quint16 data );
	void setWanderRadius( quint16 data );
	void setWanderFollowTarget( P_CHAR data );
	void setWanderDestination( const Coord& data );

	// cPythonScriptable inherited methods
	PyObject* getPyObject();
	const char* className() const;

	static void setClassid( unsigned char id )
	{
		cNPC::classid = id;
	}

	unsigned char getClassid()
	{
		return cNPC::classid;
	}

	static void buildSqlString( const char* objectid, QStringList& fields, QStringList& tables, QStringList& conditions );

	static void setInsertQuery( QSqlQuery* q )
	{
		cNPC::insertQuery_ = q;
	}
	static QSqlQuery* getInsertQuery()
	{
		return cNPC::insertQuery_;
	}
	static void setUpdateQuery( QSqlQuery* q )
	{
		cNPC::updateQuery_ = q;
	}
	static QSqlQuery* getUpdateQuery()
	{
		return cNPC::updateQuery_;
	}

private:
	bool changed_;
	static unsigned char classid;
	static QSqlQuery * insertQuery_;
	static QSqlQuery * updateQuery_;

protected:
	// interface implementation
	virtual void processNode( const cElement* Tag, uint hash = 0 );

	// Time till NPC talks again.
	// cOldChar::antispamtimer_
	quint32 nextMsgTime_;

	// Time till the NPC calls another guard.
	// cOldChar::antiguardstimer_
	quint32 nextGuardCallTime_;

	// Time till the NPC handles another begging attempt.
	// cOldChar::begging_timer_
	quint32 nextBeggingTime_;

	// Time till npc moves next.
	// cOldChar::npcmovetime_
	quint32 nextMoveTime_;

	// Stores information about how the npc wanders. uses the struct
	// stWanderType with attributes for rectangles, circles and more...
	//
	// cOldChar::npcWander_
	// cOldChar::fx1_ ...
	stWanderType wanderType_;

	// Time till summoned creature disappears.
	// cOldChar::summontimer_
	quint32 summonTime_;

	// Additional property flags
	//
	// Bits:
	// 0x00000001 Creature is summoned
	quint32 additionalFlags_;

	// Owner of this NPC.
	P_PLAYER owner_;

	// Serial of the owner of this NPC
	SERIAL ownerSerial_;

	// Serial of the stablemaster that stables the NPC.
	SERIAL stablemasterSerial_;

	// A* calculated path which the NPC walks on.
	QList<Coord> path_;

	// NPC AI interface
	AbstractAI* ai_;

	// NPC AI ID
	QString aiid_;

	// NPC AI check timer
	quint32 aiCheckTime_;

	// NPC Check for Items and NPCs
	quint32 aiNpcsCheckTime_;
	quint32 aiItemsCheckTime_;

	// NPC AI check time intervall in msec
	quint16 aiCheckInterval_;

	// NPC AI check time intervall for Loop for NPCs and Items (in msec)
	ushort aiCheckNPCsInterval_;
	ushort aiCheckITEMsInterval_;
};

inline quint32 cNPC::additionalFlags() const
{
	return additionalFlags_;
}

inline void cNPC::setAdditionalFlags( quint32 data )
{
	additionalFlags_ = data;
	changed_ = true;
}

inline quint32 cNPC::nextBeggingTime() const
{
	return nextBeggingTime_;
}

inline void cNPC::setNextBeggingTime( quint32 data )
{
	nextBeggingTime_ = data;
	changed_ = true;
}

inline quint32 cNPC::nextGuardCallTime() const
{
	return nextGuardCallTime_;
}

inline void cNPC::setNextGuardCallTime( quint32 data )
{
	nextGuardCallTime_ = data;
	changed_ = true;
}

inline quint32 cNPC::nextMoveTime() const
{
	return nextMoveTime_;
}

inline void cNPC::setNextMoveTime( quint32 data )
{
	nextMoveTime_ = data;
}

inline quint32 cNPC::nextMsgTime() const
{
	return nextMsgTime_;
}

inline void cNPC::setNextMsgTime( quint32 data )
{
	nextMsgTime_ = data;
}

inline quint32 cNPC::summonTime() const
{
	return summonTime_;
}

inline void cNPC::setSummonTime( quint32 data )
{
	summonTime_ = data;
	changed_ = true;
}

inline P_PLAYER cNPC::owner() const
{
	return owner_;
}

inline SERIAL cNPC::stablemasterSerial() const
{
	return stablemasterSerial_;
}

inline AbstractAI* cNPC::ai() const
{
	return ai_;
}

inline void cNPC::setAI( AbstractAI* ai )
{
	ai_ = ai;
	changed_ = true;
}

inline quint32 cNPC::aiCheckTime() const
{
	return aiCheckTime_;
}

inline quint32 cNPC::aiNpcsCheckTime() const
{
	return aiNpcsCheckTime_;
}

inline quint32 cNPC::aiItemsCheckTime() const
{
	return aiItemsCheckTime_;
}

inline void cNPC::setAICheckTime( quint32 data )
{
	aiCheckTime_ = data;
}

inline void cNPC::setAINpcsCheckTime( quint32 data )
{
	aiNpcsCheckTime_ = data;
}

inline void cNPC::setAIItemsCheckTime( quint32 data )
{
	aiItemsCheckTime_ = data;
}

inline quint16 cNPC::aiCheckInterval() const
{
	return aiCheckInterval_;
}

inline void cNPC::setAICheckInterval( quint16 data )
{
	aiCheckInterval_ = data;
	changed_ = true;
}

inline enCharTypes cNPC::objectType()
{
	return enNPC;
}

inline enWanderTypes cNPC::wanderType() const
{
	return wanderType_.type;
}

inline quint16 cNPC::wanderX1() const
{
	return wanderType_.x1;
}

inline quint16 cNPC::wanderX2() const
{
	return wanderType_.x2;
}

inline quint16 cNPC::wanderY1() const
{
	return wanderType_.y1;
}

inline quint16 cNPC::wanderY2() const
{
	return wanderType_.y2;
}

inline quint16 cNPC::wanderRadius() const
{
	return wanderType_.radius;
}

inline P_CHAR cNPC::wanderFollowTarget() const
{
	return wanderType_.followTarget;
}

inline Coord cNPC::wanderDestination() const
{
	return wanderType_.destination;
}

inline void cNPC::setWanderType( enWanderTypes data )
{
	wanderType_.type = data;
}

inline void cNPC::setWanderX1( quint16 data )
{
	wanderType_.x1 = data;
}

inline void cNPC::setWanderX2( quint16 data )
{
	wanderType_.x2 = data;
}

inline void cNPC::setWanderY1( quint16 data )
{
	wanderType_.y1 = data;
}

inline void cNPC::setWanderY2( quint16 data )
{
	wanderType_.y2 = data;
}

inline void cNPC::setWanderRadius( quint16 data )
{
	wanderType_.radius = data;
}

inline void cNPC::setWanderFollowTarget( P_CHAR data )
{
	wanderType_.followTarget = data;
}

inline void cNPC::setWanderDestination( const Coord& data )
{
	wanderType_.destination = data;
}

inline bool cNPC::summoned() const
{
	return ( additionalFlags_ & 0x01 ) != 0;
}

inline void cNPC::setSummoned( bool data )
{
	if ( data )
		additionalFlags_ |= 0x01;
	else
		additionalFlags_ &= ~0x01;
	changed_ = true;
}

#endif /* CNPC_H_HEADER_INCLUDED */
