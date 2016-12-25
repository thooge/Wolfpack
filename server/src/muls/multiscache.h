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

#if !defined(__MULTISCACHE_H__)
#define __MULTISCACHE_H__

#include <QVector>
#include <QMap>
#include "../singleton.h"
#include "../server.h"

struct multiItem_st
{
	qint16 tile;
	qint16 x;
	qint16 y;
	qint8 z;
	bool visible;
};

class MultiDefinition
{
public:
	MultiDefinition();

	const QList<multiItem_st>& itemsAt( int x, int y );

	int getHeight()
	{
		return height;
	}

	int getWidth()
	{
		return width;
	}

	int getLeft()
	{
		return left;
	}

	int getRight()
	{
		return right;
	}

	int getBottom()
	{
		return bottom;
	}

	int getTop()
	{
		return top;
	}

	void setItems( const QList<multiItem_st>& items );
	bool inMulti( short x, short y );
	signed char multiHeight( short x, short y, short z ) const;
	QList<multiItem_st> getEntries() const;
protected:
	int width;
	int height;
	int left;
	int top;
	int right;
	int bottom;

	QList<multiItem_st> entries; // sorted list of items
	QVector< QList<multiItem_st> > grid;

};

class cMultiCache : public cComponent
{
	QMap<ushort, MultiDefinition*> multis;
public:
	cMultiCache()
	{
	}
	virtual ~cMultiCache();

	void load();
	void unload();
	void reload();
	MultiDefinition* getMulti( ushort id );
};

typedef Singleton<cMultiCache> MultiCache;

#endif // __MULTISCACHE_H__
