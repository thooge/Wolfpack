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

#if !defined (CPLAYER_H_HEADER_INCLUDED)
#define CPLAYER_H_HEADER_INCLUDED

// platform includes
#include "platform.h"

// library includes

// wolfpack includes
#include "basechar.h"
#include "accounts.h"
#include "objectdef.h"

class cGuild;

// Class for player characters. Implements cBaseChar.
class cPlayer : public cBaseChar
{
	OBJECTDEF( cPlayer )
public:
	const char* objectID() const
	{
		return "cPlayer";
	}

	// con-/destructors
	cPlayer();
	cPlayer( const cPlayer& right );
	virtual ~cPlayer();

	// operators
	cPlayer& operator=( const cPlayer& right );

	// type definitions
	enum enInputMode
	{
		enNone,
		enRenameRune,
		enPricing,
		enDescription,
		enNameDeed,
		enHouseSign,
		enPageGM,
		enPageCouns
	};

	// implementation of interfaces
	void load( QSqlQuery&, ushort& );
	void save();
	bool del();
	void postload( unsigned int version );
	void load( cBufferedReader& reader, unsigned int version );
	void save( cBufferedWriter& writer, unsigned int version );
	void load( cBufferedReader& reader );

	virtual bool send( cUOPacket* packet );
	virtual enCharTypes objectType();
	virtual void update( bool excludeself = false );
	virtual void resend( bool clean = true );
	virtual void talk( const QString& message, UI16 color = 0xFFFF, quint8 type = 0, bool autospam = false, cUOSocket* socket = NULL );
	virtual void talk( uint MsgID, const QString& params = 0, const QString& affix = 0, bool prepend = false, ushort color = 0xFFFF, cUOSocket* socket = 0 );
	virtual quint8 notoriety( P_CHAR pChar );
	virtual void showName( cUOSocket* socket );
	virtual void soundEffect( ushort soundId, bool hearAll = true );
	virtual void giveGold( uint amount, bool inBank = false );
	virtual uint takeGold( uint amount, bool useBank = false );
	virtual void flagUnchanged();
	virtual bool message( const QString& message, unsigned short color = 0x3b2, cUObject* source = 0, unsigned short font = 0x03, unsigned char mode = 0x00 );
	virtual bool sysmessage( const QString& message, unsigned short color = 0x3b2, unsigned short font = 0x03 );
	virtual bool sysmessage( unsigned int message, const QString& params = QString::null, unsigned short color = 0x3b2, unsigned short font = 0x03 );
	virtual FightStatus fight( P_CHAR enemy );
	void log( eLogLevel, const QString& string );
	void log( const QString& string );
	void awardFame( short amount, bool showmessage = true );
	void awardKarma( P_CHAR pKilled, short amount, bool showmessage = true );
	bool isOverloaded();
	unsigned int maxWeight();

	// other public methods
	virtual stError* setProperty( const QString& name, const cVariant& value );
	PyObject* getProperty( const QString& name, uint hash = 0 );
	P_NPC unmount();
	bool mount( P_NPC pMount );
	bool isGM() const;
	bool isCounselor() const;
	bool isGMorCounselor() const;
	void makeCriminal();
	void disturbMed();
	int countBankGold();
	bool canPickUp( cItem* pi );
	virtual bool inWorld();
	void giveNewbieItems( quint8 skill = 0xFF );
	bool checkSkill( ushort skill, int min, int max, bool advance = true ); // override
	void createTooltip( cUOTxTooltipList& tooltip, cPlayer* player );
	virtual unsigned char controlslots() const;
	unsigned int damage( eDamageType type, unsigned int amount, cUObject* source = 0 );

	//! Returns true if there's a client (socket) attached to the character.
	bool isConnected();

	//! Returns true if the char is present in the world (may have disconnected, but be lingering).
	bool isOnline();

	// Wrapper events
	virtual bool onLogin(); // The character enters the world
	virtual bool onConnect( bool reconnecting ); // A socket attaches to a character
	virtual bool onDisconnect(); // A socket detaches from a character
	virtual bool onLogout(); // The character leaves the world
	virtual bool onHelp(); // The character wants help
	virtual bool onChat(); // The character wants to chat
	virtual bool onCastSpell( unsigned int spell );
	virtual bool onBecomeCriminal( unsigned int reason, P_CHAR sourcechar, P_ITEM sourceitem ); // The Character will becomes criminal
	virtual int onBuy( P_CHAR pVendor, P_ITEM pItem, int amount ); // Player Buying a item (Ovewrite BaseChars Method)
	void poll( unsigned int time, unsigned int events );
	bool onUse( P_ITEM pItem );
	bool onPickup( P_ITEM pItem );
	bool onTrade( unsigned int type, unsigned int buttonstate, SERIAL itemserial );
	bool onTradeStart( P_PLAYER partner, P_ITEM firstitem );
	int onStepChar( P_CHAR pChar );
	int onStepWeightPercent( int percent, bool mounted, bool running );

	// getters
	cAccount* account() const;
	quint32 additionalFlags() const;
	quint32 logoutTime() const;
	quint32 objectDelay() const;
	quint32 trackingTime() const;
	cUOSocket* socket() const;
	enInputMode inputMode() const
	{
		return inputMode_;
	}
	SERIAL inputItem() const;
	quint8 visualRange() const;
	QString profile() const;
	qint8 fixedLightLevel() const;
	quint8 maxControlSlots() const;

	// bit flag getters
	bool maySnoop() const;
	bool mayBroadcast() const;
	bool showSerials() const;
	bool karmaLock() const;
	bool isJailed() const;
	bool isSquelched() const;
	// advanced getters for data structures
	// pets
	CharContainer pets() const;
	cParty* party() const;
	cGuild* guild() const;

	// setters
	void setMaxControlSlots( unsigned char data );
	void setAccount( cAccount* data, bool moveFromAccToAcc = true );
	void setAdditionalFlags( quint32 data );
	void setLogoutTime( quint32 data );
	void setObjectDelay( quint32 data );
	void setTrackingTime( quint32 data );
	void setSocket( cUOSocket* data );
	void setInputMode( enInputMode data )
	{
		inputMode_ = data;
	}
	void setInputItem( SERIAL data );
	void setVisualRange( quint8 data );
	void setProfile( const QString& data );
	void setFixedLightLevel( quint8 data );
	// bit flag setters
	void setGuild( cGuild* data );
	void setJailed( bool data );
	void setKarmaLock( bool data );	
	void setMayBroadcast( bool data );
	void setMaySnoop( bool data );
	void setParty( cParty* data );
	void setShowSerials( bool data );
	void setSquelched( bool data );

	unsigned char strengthLock() const;
	unsigned char dexterityLock() const;
	unsigned char intelligenceLock() const;

	void setStrengthLock( unsigned char data );
	void setDexterityLock( unsigned char data );
	void setIntelligenceLock( unsigned char data );

	virtual void setStamina( qint16 data, bool notify = true );

	void remove();

	// advanced setters for data structures
	// pets
	void addPet( P_NPC pPet, bool noOwnerChange = false );
	void removePet( P_NPC pPet, bool noOwnerChange = false );
	bool canSeeChar( P_CHAR character );
	bool canSeeItem( P_ITEM item );
	virtual void moveTo( const Coord& pos );

	// cPythonScriptable inherited methods
	PyObject* getPyObject();
	const char* className() const;

	static void setClassid( unsigned char id )
	{
		cPlayer::classid = id;
	}

	unsigned char getClassid()
	{
		return cPlayer::classid;
	}

	static void buildSqlString( const char* objectid, QStringList& fields, QStringList& tables, QStringList& conditions );

	static void setInsertQuery( QSqlQuery* q ) 
	{
		cPlayer::insertQuery_ = q;
	}

	static QSqlQuery* getInsertQuery() 
	{
		return cPlayer::insertQuery_;
	}

	static void setUpdateQuery( QSqlQuery* q ) 
	{
		cPlayer::updateQuery_ = q;
	}

	static QSqlQuery* getUpdateQuery() 
	{
		return cPlayer::updateQuery_;
	}

private:
	bool changed_;
	static unsigned char classid;
	static QSqlQuery * insertQuery_;
	static QSqlQuery * updateQuery_;

protected:
	// interface implementation

	// Reference to a guild this character is in
	cGuild* guild_;

	// other protected methods
	void applyStartItemDefinition( const cElement* Tag );

	// The party this player belongs to
	cParty* party_;

	// The account object including this char.
	// cOldChar::account_
	cAccount* account_;

	// How many pets can this player control
	unsigned char maxControlSlots_;

	// time till char will be logged out
	// cOldChar::logout_
	quint32 logoutTime_;

	// Time till the player can use another object.
	quint32 objectDelay_;

	// Additional property flags.
	//
	// Bits:
	// 02 - may snoop, cOldChar::priv Bit 7
	// 03 - may broadcast, cOldChar::priv Bit 2
	// 04 - show serials, cOldChar::priv Bit 4
	// 05 - lock karma (0x10)
	// 06 - jailed (0x20)
	// 07 - squelched (0x40)
	quint32 additionalFlags_;

	// The pets that follow the char.
	CharContainer pets_;

	unsigned char strengthLock_;
	unsigned char dexterityLock_;
	unsigned char intelligenceLock_;

	// Time till the quest arrow for tracking disappears.
	quint32 trackingTime_;

	// Network socket of the player.
	cUOSocket* socket_;

	// Input mode of the entered text.
	// 0 - none
	// 4 - rune renaming
	enInputMode inputMode_;

	// Serial of the item which is affected by the current text entry.
	SERIAL inputItem_;

	// Visual range of the player
	quint8 visualRange_;

	// Paperdoll profile of the char
	QString profile_;

	// Fixed light level. is used in dungeons or for nightsight spell.
	// cOldChar:fixedlight_
	qint8 fixedLightLevel_;
};

inline cAccount* cPlayer::account() const
{
	return account_;
}

inline void cPlayer::setAccount( cAccount* data, bool moveFromAccToAcc )
{
	if ( moveFromAccToAcc && account_ != 0 )
		account_->removeCharacter( this );

	account_ = data;

	if ( account_ != 0 )
		account_->addCharacter( this );
	changed_ = true;
	changed( TOOLTIP );
}

inline void cPlayer::flagUnchanged()
{
	cPlayer::changed_ = false;
	cBaseChar::flagUnchanged();
}

inline uint cPlayer::additionalFlags() const
{
	return additionalFlags_;
}

inline void cPlayer::setAdditionalFlags( uint data )
{
	additionalFlags_ = data;
	changed_ = true;
}

inline quint32 cPlayer::logoutTime() const
{
	return logoutTime_;
}

inline void cPlayer::setLogoutTime( quint32 data )
{
	logoutTime_ = data;
	changed_ = true;
}

inline quint32 cPlayer::objectDelay() const
{
	return objectDelay_;
}

inline void cPlayer::setObjectDelay( quint32 data )
{
	objectDelay_ = data;
}

inline quint32 cPlayer::trackingTime() const
{
	return trackingTime_;
}

inline void cPlayer::setTrackingTime( quint32 data )
{
	trackingTime_ = data;
}

inline cUOSocket* cPlayer::socket() const
{
	return socket_;
}

inline void cPlayer::setSocket( cUOSocket* data )
{
	socket_ = data;
	changed_ = true;
}

inline SERIAL cPlayer::inputItem() const
{
	return inputItem_;
}

inline void cPlayer::setInputItem( SERIAL data )
{
	inputItem_ = data;
}

inline quint8 cPlayer::visualRange() const
{
	return visualRange_;
}

inline void cPlayer::setVisualRange( quint8 data )
{
	visualRange_ = data;
	changed_ = true;
}

inline QString cPlayer::profile() const
{
	return profile_;
}

inline void cPlayer::setProfile( const QString& data )
{
	profile_ = data;
	changed_ = true;
}

inline qint8 cPlayer::fixedLightLevel() const
{
	return fixedLightLevel_;
}

inline void cPlayer::setFixedLightLevel( quint8 data )
{
	fixedLightLevel_ = data;
	changed_ = true;
}

inline quint8 cPlayer::maxControlSlots() const
{
	return maxControlSlots_;
}

inline void cPlayer::setMaxControlSlots( quint8 data )
{
	maxControlSlots_ = data;
	changed_ = true;
}

inline bool cPlayer::maySnoop() const
{
	return additionalFlags_ & 0x0002;
}

inline bool cPlayer::mayBroadcast() const
{
	return additionalFlags_ & 0x0004;
}

inline bool cPlayer::showSerials() const
{
	return additionalFlags_ & 0x0008;
}

inline bool cPlayer::karmaLock() const
{
	return additionalFlags_ & 0x0010;
}

inline bool cPlayer::isJailed() const
{
	return additionalFlags_ & 0x0020;
}

inline bool cPlayer::isSquelched() const
{
	return additionalFlags_ & 0x0040;
}

inline void cPlayer::setMaySnoop( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0002;
	else
		additionalFlags_ &= ~0x0002;
	changed_ = true;
}

inline void cPlayer::setMayBroadcast( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0004;
	else
		additionalFlags_ &= ~0x0004;
	changed_ = true;
}

inline void cPlayer::setShowSerials( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0008;
	else
		additionalFlags_ &= ~0x0008;
	changed_ = true;
}

inline void cPlayer::setKarmaLock( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0010;
	else
		additionalFlags_ &= ~0x0010;
	changed_ = true;
}

inline void cPlayer::setJailed( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0020;
	else
		additionalFlags_ &= ~0x0020;
	changed_ = true;
}

inline void cPlayer::setSquelched( bool data )
{
	if ( data )
		additionalFlags_ |= 0x0040;
	else
		additionalFlags_ &= ~0x0040;
	changed_ = true;
}

inline enCharTypes cPlayer::objectType()
{
	return enPlayer;
}

inline cBaseChar::CharContainer cPlayer::pets() const
{
	return pets_;
}

inline cParty* cPlayer::party() const
{
	return party_;
}

inline void cPlayer::setParty( cParty* data )
{
	party_ = data;
}

inline void cPlayer::setGuild( cGuild* data )
{
	guild_ = data;
}

inline cGuild* cPlayer::guild() const
{
	return guild_;
}

inline unsigned char cPlayer::strengthLock() const
{
	return strengthLock_;
}

inline unsigned char cPlayer::dexterityLock() const
{
	return dexterityLock_;
}

inline unsigned char cPlayer::intelligenceLock() const
{
	return intelligenceLock_;
}

inline void cPlayer::setStrengthLock( unsigned char data )
{
	strengthLock_ = data;
	changed_ = true;
}

inline void cPlayer::setDexterityLock( unsigned char data )
{
	dexterityLock_ = data;
	changed_ = true;
}

inline void cPlayer::setIntelligenceLock( unsigned char data )
{
	intelligenceLock_ = data;
	changed_ = true;
}

inline bool cPlayer::isConnected()
{
	return socket_ != 0;
}

inline bool cPlayer::isOnline()
{
	return socket_ != 0 || logoutTime_ != 0;
}

#endif /* CPLAYER_H_HEADER_INCLUDED */
