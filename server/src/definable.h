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

#if !defined (__DEFINABLE_H__)
#define __DEFINABLE_H__

#include <QtGlobal>

// Library includes
class cElement;
class QString;

class cDefinable
{
public:
	virtual ~cDefinable()
	{
	}
	virtual void applyDefinition( const cElement* sectionNode );
protected:
	virtual void processNode( const cElement* Tag, uint hash = 0 ) = 0;
	void processModifierNode( const cElement* Tag );
};

#endif
