/*
 *     Wolfpack Emu (WP)
 * UO Server Emulation Program
 *
 * Copyright 2001-2004 by holders identified in AUTHORS.txt
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
 * Wolfpack Homepage: http://wpdev.sf.net/
 */

#if !defined (__UOBJECT_H__)
#define __UOBJECT_H__

#include "exceptions.h"
#include "platform.h"
#include "typedefs.h"
#include "coord.h"
#include "persistentobject.h"
#include "definable.h"
#include "singleton.h"
#include "customtags.h"
#include "factory.h"
#include "spawnregions.h"
#include "pythonscript.h"
#include "world.h"

// System includes
#include <vector>
#include <map>

// Library includes
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>

// Forward class declarations
class Coord_cl;
class cPythonScript;
class cUOSocket;
class QSqlQuery;
class cItem;
class cUOTxTooltipList;
class cMulti;

class cBufferedReader;
class cBufferedWriter;

#pragma pack(1)
class cUObject : public PersistentObject, public cDefinable, public cPythonScriptable
{
private:
	uchar changed_ : 1;

protected:
	cCustomTags tags_;
	uint tooltip_;
	QString name_;
	Coord_cl pos_;
	SERIAL serial_;
	cMulti* multi_; // If we're in a Multi	
	cPythonScript** scriptChain; // NULL Terminated Array
	cSpawnRegion *spawnregion_;

	// Things for building the SQL string
	static void buildSqlString( QStringList& fields, QStringList& tables, QStringList& conditions );
	void init();

	enum eChanged
	{
		//		SAVE = 1,
		TOOLTIP				= 2,
		UNUSED				= 4,
		UNUSED2				= 8
	};
	void changed( uint );
public:
	// Indicates whether the object was deleted already.
	bool free : 1;

	const char* objectID() const
	{
		return "cUObject";
	}

	// Tag Management Methods
	const cVariant& getTag( const QString& key ) const;
	bool hasTag( const QString& key ) const;
	void setTag( const QString& key, const cVariant& value );
	void removeTag( const QString& key );
	void clearTags();
	QStringList getTags() const;

	// Event Management Methods
	void clearEvents();
	void addEvent( cPythonScript* Event );
	void removeEvent( const QString& Name );
	bool hasEvent( const QString& Name ) const;
	void freezeScriptChain();
	void unfreezeScriptChain();
	bool isScriptChainFrozen();
	void setEventList( const QString& events );
	QString eventList() const;
	inline cPythonScript** getEvents()
	{
		return scriptChain;
	}

	// Serialization Methods
	void load( char**, UINT16& );
	void save();
	bool del();

	// Wrapper
	virtual void load( cBufferedReader& reader ) = 0;
	virtual void save( cBufferedWriter& reader );

	// "Real" ones
	virtual void load( cBufferedReader& reader, unsigned int version );
	virtual void save( cBufferedWriter& reader, unsigned int version );
	virtual void postload( unsigned int version ) = 0;

	// Utility Methods
	void effect( UINT16 id, UINT8 speed = 10, UINT8 duration = 5, UINT16 hue = 0, UINT16 renderMode = 0 ); // Moving with this character
	void effect( UINT16 id, cUObject* target, bool fixedDirection = true, bool explodes = false, UINT8 speed = 10, UINT16 hue = 0, UINT16 renderMode = 0 );
	void effect( UINT16 id, const Coord_cl& target, bool fixedDirection = true, bool explodes = false, UINT8 speed = 10, UINT16 hue = 0, UINT16 renderMode = 0 );
	void lightning( unsigned short hue = 0 );
	bool inRange( cUObject* object, UINT32 range ) const;
	void removeFromView( bool clean = true );
	virtual void sendTooltip( cUOSocket* mSock );
	bool isItem() const
	{
		return isItemSerial( serial_ );
	}
	bool isChar() const
	{
		return isCharSerial( serial_ );
	}
	virtual void talk( const QString& message, UI16 color = 0xFFFF, UINT8 type = 0, bool autospam = false, cUOSocket* socket = NULL ) = 0;
	virtual void flagUnchanged()
	{
		changed_ = false;
	}
	void resendTooltip();
	unsigned char direction( cUObject* );
	virtual void remove();
	virtual void moveTo( const Coord_cl&, bool noRemove = false );
	unsigned int dist( cUObject* d ) const;

	// Event Methods
	virtual bool onCreate( const QString& definition );
	virtual bool onShowTooltip( P_PLAYER sender, cUOTxTooltipList* tooltip ); // Shows a tool tip for specific object
	virtual void createTooltip( cUOTxTooltipList& tooltip, cPlayer* player );

	// Constructors And Destructors
	cUObject();
	cUObject( const cUObject& );
	virtual ~cUObject();

	// Getter Methods
	virtual QCString bindmenu() = 0;
	virtual unsigned char getClassid() = 0;

	QString name() const
	{
		return name_;
	}
	Coord_cl pos() const
	{
		return pos_;
	}
	SERIAL serial() const
	{
		return serial_;
	}
	UINT32 getTooltip() const
	{
		return tooltip_;
	}
	inline cMulti* multi() const
	{
		return multi_;
	}

	inline cSpawnRegion* spawnregion() const
	{
		return spawnregion_;
	}

	// Setter Methods
	void setName( const QString& d )
	{
		name_ = d; changed_ = true; changed( TOOLTIP );
	}

	void setPos( const Coord_cl& d )
	{
		pos_ = d;	changed_ = true;
	}

	virtual void setSerial( SERIAL d )
	{
		serial_ = d; changed_ = true;
	}

	void setTooltip( const UINT32 d )
	{
		tooltip_ = d;
	}
	inline void setMulti( cMulti* multi )
	{
		multi_ = multi; changed_ = true;
	}

	void setSpawnregion( cSpawnRegion* spawnregion );

	// Definable Methods
	void processNode( const cElement* Tag );
	stError* setProperty( const QString& name, const cVariant& value );
	PyObject* getProperty( const QString& name );
};
#pragma pack()

class cUObjectFactory : public Factory<cUObject, QString>
{
public:
	cUObjectFactory()
	{
		lastid = 0;
	}

	unsigned int registerSqlQuery( const QString& type, const QString& query )
	{
		sql_queries.insert( std::make_pair( type, query ) );
		sql_keys.push_back( type );

		if ( lastid + 1 < lastid )
		{
			throw wpException( "Only 256 types can be registered with the UObject factory." );
		}

		typemap.insert( lastid, type );
		return lastid++;
	}

	QString findSqlQuery( const QString& type ) const
	{
		std::map<QString, QString>::const_iterator iter = sql_queries.find( type );

		if ( iter == sql_queries.end() )
			return QString::null;
		else
			return iter->second;
	}

	QStringList objectTypes() const
	{
		return sql_keys;
	}

	const QMap<unsigned char, QString>& getTypemap()
	{
		return typemap;
	}

private:
	std::map<QString, QString> sql_queries;
	QMap<unsigned char, QString> typemap;
	QStringList sql_keys;
	unsigned char lastid;
};

typedef SingletonHolder<cUObjectFactory> UObjectFactory;

#endif // __UOBJECT_H__
