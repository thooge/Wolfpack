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

#if !defined( __WORLD_H__ )
#define __WORLD_H__

// Wolfpack Includes
#include "defines.h"
#include "typedefs.h"
#include "singleton.h"
#include "objectdef.h"

// Library Includes
#include <QMap>
#include <QThread>

#include "server.h"

class PersistentObject;
class cBufferedReader;

class cCharIterator
{
private:
	struct stCharIteratorPrivate* p;
public:
	cCharIterator();
	virtual ~cCharIterator();

	P_CHAR first();
	P_CHAR next();
};

class cItemIterator
{
private:
	struct stItemIteratorPrivate* p;
public:
	cItemIterator();
	virtual ~cItemIterator();

	P_ITEM first();
	P_ITEM next();
};

class cBackupThread : public QThread
{
private:
	QString filename;
public:
	void run();

	void setFilename( const QString& name )
	{
		filename = name;
	}
};

class cWorld : public cComponent
{
	OBJECTDEF( cWorld )
	friend class cCharIterator;
	friend class cItemIterator;

private:
	// Everything that doesn't need to be accessed via a getter or setter
	// is implemented in this private structure for compile reasons.
	class cWorldPrivate* p;
	unsigned int _charCount;
	unsigned int _itemCount;
	unsigned int lastTooltip;
	SERIAL _lastCharSerial, _lastItemSerial;
	unsigned int _playerCount, _npcCount;
	void loadTag( cBufferedReader& reader, unsigned int version );
	QMap<QString, QString> options;
	void backupWorld( const QString& filename, int count, bool compress );
	QList<cBackupThread*> backupThreads;

public:
	// Constructor/Destructor
	cWorld();
	virtual ~cWorld();

	// WorldLoader interface
	void load();
	void loadBinary( QList<PersistentObject*>& objects );
	void loadSQL( QList<PersistentObject*>& objects );
	void unload();
	void save();

	// For the "settings" table
	void getOption( const QString& name, QString& value, const QString fallback );
	void setOption( const QString& name, const QString& value );

	// Get the database version
	unsigned int getDatabaseVersion() const;

	// Book-keeping functions
	void registerObject( cUObject* object );
	void registerObject( SERIAL serial, cUObject* object );
	void unregisterObject( cUObject* object );
	void unregisterObject( SERIAL serial );

	// Register an object to be deleted with the next save
	void deleteObject( cUObject* object );
	void purge();

	// Lookup Functions
	P_ITEM findItem( SERIAL serial ) const;
	P_CHAR findChar( SERIAL serial ) const;
	cUObject* findObject( SERIAL serial ) const;

	SERIAL findCharSerial() const
	{
		return _lastCharSerial + 1;
	}

	SERIAL findItemSerial() const
	{
		return _lastItemSerial + 1;
	}

	unsigned int charCount() const
	{
		return _charCount;
	}
	unsigned int itemCount() const
	{
		return _itemCount;
	}

	unsigned int npcCount() const
	{
		return _npcCount;
	}

	unsigned int playerCount() const
	{
		return _playerCount;
	}

	unsigned int getUnusedTooltip()
	{
		return ++lastTooltip;
	}
};

typedef Singleton<cWorld> World;

#define FindCharBySerial( serial ) World::instance()->findChar( serial )
#define FindItemBySerial( serial ) World::instance()->findItem( serial )
inline bool isItemSerial( SERIAL serial )
{
	return ( serial > 0x40000000 );
};
inline bool isCharSerial( SERIAL serial )
{
	return ( serial < 0x40000000 );
};

#endif
