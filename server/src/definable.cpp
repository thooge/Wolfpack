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

#include "definable.h"
#include "definitions.h"
#include "log.h"
#include "console.h"
#include "basics.h"

// Qt Includes
#include <QStringList>

void cDefinable::applyDefinition( const cElement* sectionNode )
{
	//unsigned int starttime = getNormalizedTime();

	if ( sectionNode->hasAttribute( "inherit" ) )
	{
		eDefCategory wpType = WPDT_ITEM;

		if ( sectionNode->name() == "npc" )
			wpType = WPDT_NPC;
		else if ( sectionNode->name() == "region" )
			wpType = WPDT_REGION;
		else if ( sectionNode->name() == "spawnregion" )
			wpType = WPDT_SPAWNREGION;
		else if ( sectionNode->name() == "quest" )
			wpType = WPDT_QUEST;

		const cElement* tInherit = Definitions::instance()->getDefinition( wpType, sectionNode->getAttribute( "inherit", "" ) );

		if ( tInherit == sectionNode )
		{
			Console::instance()->log( LOG_ERROR, tr( "Circulary inheriting %1.\n" ).arg( sectionNode->getAttribute( "inherit", "" ) ) );
			return;
		}

		if ( tInherit )
			applyDefinition( tInherit );
	}

	// Check for random-inherit
	if ( sectionNode->hasAttribute( "inheritlist" ) )
	{
		eDefCategory wpType = WPDT_ITEM;

		if ( sectionNode->name() == "npc" )
			wpType = WPDT_NPC;

		QString iSection = Definitions::instance()->getRandomListEntry( sectionNode->getAttribute( "inheritlist", "" ) );

		const cElement* tInherit = Definitions::instance()->getDefinition( wpType, iSection );

		if ( tInherit )
			applyDefinition( tInherit );
	}

	for ( unsigned int i = 0; i < sectionNode->childCount(); ++i )
	{
		processNode( sectionNode->getChild( i ) );
	}

	//unsigned int endtime = getNormalizedTime();

	//Console::instance()->send( QString( "applyDefinition took %1 ms\n" ).arg( (float)(endtime - starttime) / MY_CLOCKS_PER_SEC * 1000.0f ) );
}

void cDefinable::processModifierNode( const cElement* Tag )
{
	Q_UNUSED( Tag );
};
