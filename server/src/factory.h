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

#if !defined(__FACTORY_H__)
#define __FACTORY_H__

// Implementation based on the book Alexandrescu, Andrei. "Modern C++ Design: Generic
// Programming and Design Patterns Applied".

// I knew buying the book was a good idea :o)

#include <map>

template <class product, typename keyType, typename productCreator = product* ( * ) ()>
class Factory
{
public:
	virtual ~Factory()
	{
	}
	bool registerType( const keyType& id, productCreator creator )
	{
		return associations_.insert( std::make_pair( id, creator ) ).second;
	}

	bool unregisterType( const keyType& id )
	{
		return associations_.erase( id ) == 1;
	}

	product* createObject( const keyType& id ) const
	{
		typename mapTypes::const_iterator it = associations_.find( id );
		if ( it != associations_.end() )
		{
			return ( it->second ) ();
		}
		return 0;
	}

private:
	typedef std::map<keyType, productCreator> mapTypes;
	mapTypes associations_;
};

#endif // __FACTORY_H__
