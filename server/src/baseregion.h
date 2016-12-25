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

#if !defined(__BASEREGION_H__)
#define __BASEREGION_H__

#include "definable.h"
#include "platform.h"
#include "coord.h"
#include "typedefs.h"
#include "definitions.h"

// Library includes
#include <QString>
#include <QStringList>
#include <QMap>

// Forward Definitions

class cBaseRegion : public cDefinable
{
public:
	struct rect_st
	{
		ushort x1;
		ushort x2;
		ushort y1;
		ushort y2;
		uchar map;
	};

	cBaseRegion() : parent_( 0 )
	{
	}

	cBaseRegion( const cElement* tag, cBaseRegion* pParent )
	{
		this->init();
		this->name_ = tag->getAttribute( "id" );
		this->applyDefinition( tag );
		this->parent_ = pParent;
	}

	virtual ~cBaseRegion()
	{
		QList<cBaseRegion*>::iterator it = this->subregions_.begin();
		while ( it != this->subregions_.end() )
		{
			delete ( *it );
			++it;
		}
	}

	void init( void )
	{
		name_ = "the wilderness";
	}

	bool contains( ushort posx, ushort posy, uchar map ) const
	{
		QList<rect_st>::const_iterator it = this->rectangles_.begin();

		while ( it != this->rectangles_.end() )
		{
			if ( ( ( posx >= ( *it ).x1 && posx <= ( *it ).x2 ) || ( posx >= ( *it ).x2 && posx <= ( *it ).x2 ) ) && ( ( posy >= ( *it ).y1 && posy <= ( *it ).y2 ) || ( posy >= ( *it ).y2 && posy <= ( *it ).y1 ) ) && map == ( *it ).map )
				return true;
			++it;
		}
		return false;
	}

	cBaseRegion* region( const QString& regName )
	{
		if ( this->name_ == regName )
			return this;
		else
		{
			QList<cBaseRegion*>::const_iterator it = this->subregions_.begin();
			while ( it != this->subregions_.end() )
			{
				cBaseRegion* currRegion = 0;
				if ( *it != 0 )
					currRegion = ( *it )->region( regName );
				if ( currRegion != 0 )
					return currRegion;
				++it;
			}
		}
		return 0;
	}

	cBaseRegion* region( ushort posx, ushort posy, uchar map )
	{
		cBaseRegion* foundRegion = 0;
		if ( this->contains( posx, posy, map ) )
			foundRegion = this;
		else
			return 0;

		QList<cBaseRegion*>::iterator it = this->subregions_.begin();
		while ( it != this->subregions_.end() )
		{
			cBaseRegion* currRegion = 0;
			if ( *it != 0 )
				currRegion = ( *it )->region( posx, posy, map );
			if ( currRegion != 0 )
				foundRegion = currRegion;
			++it;
		}
		return foundRegion;
	}

	uint count( void ) const
	{
		uint result = 1;
		QList<cBaseRegion*>::const_iterator it( this->subregions_.begin() );
		while ( it != this->subregions_.end() )
		{
			if ( *it != NULL )
				result += ( *it )->count();
			++it;
		}
		return result;
	}

protected:
	virtual void processNode( const cElement* Tag, uint /*hash*/ = 0 )
	{
		QString TagName = Tag->name();
		QString Value = Tag->value();

		// <rectangle x1="0" x2="1000" y1="0" y2="500" />
		if ( TagName == "rectangle" )
		{
			rect_st toinsert_;
			toinsert_.x1 = Tag->getAttribute( "x1" ).toUShort();
			toinsert_.x2 = Tag->getAttribute( "x2" ).toUShort();
			toinsert_.y1 = Tag->getAttribute( "y1" ).toUShort();
			toinsert_.y2 = Tag->getAttribute( "y2" ).toUShort();
			toinsert_.map = Tag->getAttribute( "map" ).toUShort();

			if ( toinsert_.y1 > toinsert_.y2 )
			{
				std::swap( toinsert_.y1, toinsert_.y2 );
			}

			if ( toinsert_.x1 > toinsert_.x2 )
			{
				std::swap( toinsert_.x1, toinsert_.x2 );
			}

			this->rectangles_.push_back( toinsert_ );
		}
		else if ( TagName == "region" && Tag->hasAttribute( "id" ) )
			this->subregions_.push_back( new cBaseRegion( Tag, this ) );
	}

protected:
	QString name_;			// name of the region (section's name)
	QList<rect_st> rectangles_;	// vector of rectangles
	QList<cBaseRegion*> subregions_;	// list of region object references of included regions
	cBaseRegion* parent_;		// the region directly above this region
public:
	// Only getters, no setters
	cBaseRegion* parent() const
	{
		return parent_;
	}
	QList<cBaseRegion*>& children()
	{
		return subregions_;
	}
	QList<rect_st>& rectangles()
	{
		return rectangles_;
	}
};

class cAllBaseRegions
{
public:
	virtual ~cAllBaseRegions()
	{
		QMap<uint, cBaseRegion*>::const_iterator it( topregions.begin() );
		for ( ; it != topregions.end(); ++it )
			delete it.value();
	}

	cBaseRegion* region( const QString& regName )
	{
		QMap<uint, cBaseRegion*>::const_iterator it( topregions.begin() );
		for ( ; it != topregions.end(); ++it )
		{
			cBaseRegion* result = it.value()->region( regName );
			if ( result )
				return result;
		}
		return 0;
	}

	cBaseRegion* region( ushort posx, ushort posy, uchar map )
	{
		QMap<uint, cBaseRegion*>::const_iterator it( topregions.find( map ) );
		if ( it != topregions.end() )
			return it.value()->region( posx, posy, map );
		else
			return 0;
	}

	uint count( void ) const
	{
		uint i = 0;
		QMap<uint, cBaseRegion*>::const_iterator it( topregions.begin() );
		for ( ; it != topregions.end(); ++it )
			i += it.value()->count();
		return i;
	}

protected:
	QMap<uint, cBaseRegion*> topregions;
};

#endif
