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

#include "accounts.h"

#include "gumps.h"
#include "serverconfig.h"
#include "definitions.h"
#include "network/network.h"
#include "network/uosocket.h"
#include "spawnregions.h"
#include "accounts.h"
#include "territories.h"
#include "basechar.h"
#include "player.h"
#include "world.h"
#include "inlines.h"

// System Includes
#include <math.h>

cGump::cGump() : serial_( INVALID_SERIAL ), type_( 1 ), x_( 50 ), y_( 50 ), noMove_( false ), noClose_( false ), noDispose_( false )
{
}

// New Single gump implementation, written by darkstorm
quint32 cGump::addRawText( const QString& data )
{
	// Do we already have the text?
	if ( !text_.contains( data ) )
		text_.push_back( data );

	return text_.indexOf( data );
}

void cGump::addButton( qint32 buttonX, qint32 buttonY, quint16 gumpUp, quint16 gumpDown, qint32 returnCode )
{
	QString button = QString( "{button %1 %2 %3 %4 1 0 %5}" ).arg( buttonX ).arg( buttonY ).arg( gumpUp ).arg( gumpDown ).arg( returnCode );
	layout_.push_back( button );
}

void cGump::addPageButton( qint32 buttonX, qint32 buttonY, quint16 gumpUp, quint16 gumpDown, qint32 pageId )
{
	QString button = QString( "{button %1 %2 %3 %4 0 %5 0}" ).arg( buttonX ).arg( buttonY ).arg( gumpUp ).arg( gumpDown ).arg( pageId );
	layout_.push_back( button );
}

void cGump::addGump( qint32 gumpX, qint32 gumpY, quint16 gumpId, qint16 hue )
{
	layout_.push_back( QString( "{gumppic %1 %2 %3%4}" ).arg( gumpX ).arg( gumpY ).arg( gumpId ).arg( ( hue != -1 ) ? QString( " hue=%1" ).arg( hue ) : QString( "" ) ) );
}

void cGump::addTiledGump( qint32 gumpX, qint32 gumpY, qint32 width, qint32 height, quint16 gumpId, qint16 hue )
{
	layout_.push_back( QString( "{gumppictiled %1 %2 %4 %5 %3%6}" ).arg( gumpX ).arg( gumpY ).arg( gumpId ).arg( width ).arg( height ).arg( ( hue != -1 ) ? QString( " hue=%1" ).arg( hue ) : QString( "" ) ) );
}

void cGump::addHtmlGump( qint32 x, qint32 y, qint32 width, qint32 height, const QString& html, bool hasBack, bool canScroll )
{
	QString layout( "{htmlgump %1 %2 %3 %4 %5 %6 %7}" );
	layout = layout.arg( x ).arg( y ).arg( width ).arg( height );
	layout = layout.arg( addRawText( html ) ).arg( hasBack ? 1 : 0 ).arg( canScroll ? 1 : 0 );
	layout_.push_back( layout );
}

void cGump::addXmfHtmlGump( qint32 x, qint32 y, qint32 width, qint32 height, quint32 clilocid, bool hasBack, bool canScroll )
{
	QString layout( "{xmfhtmlgump %1 %2 %3 %4 %5 %6 %7}" );
	layout = layout.arg( x ).arg( y ).arg( width ).arg( height );
	layout = layout.arg( clilocid ).arg( hasBack ? 1 : 0 ).arg( canScroll ? 1 : 0 );
	layout_.push_back( layout );
}

void cGump::addCheckertrans( qint32 x, qint32 y, qint32 width, qint32 height )
{
	QString layout( "{checkertrans %1 %2 %3 %4}" );
	layout = layout.arg( x ).arg( y ).arg( width ).arg( height );
	layout_.push_back( layout );
}

void cGump::addCroppedText( qint32 textX, qint32 textY, quint32 width, quint32 height, const QString& data, quint16 hue )
{
	QString layout( "{croppedtext %1 %2 %3 %4 %5 %6}" );
	layout = layout.arg( textX ).arg( textY ).arg( width ).arg( height ).arg( hue ).arg( addRawText( data ) );
	layout_.push_back( layout );
}


void cGump::handleResponse( cUOSocket* socket, const gumpChoice_st& choice )
{
	Q_UNUSED( socket );
	Q_UNUSED( choice );
}

cSpawnRegionInfoGump::cSpawnRegionInfoGump( cSpawnRegion* region )
{
	region_ = region;

	if ( region )
	{
		startPage();
		// Basic .INFO Header
		addResizeGump( 0, 40, 0xA28, 450, 420 ); //Background
		addGump( 105, 18, 0x58B ); // Fancy top-bar
		addGump( 182, 0, 0x589 ); // "Button" like gump
		addTilePic( 202, 23, 0x14eb ); // Type of info menu
		addText( 170, 90, tr( "Spawnregion Info" ), 0x530 );
		// Give information about the spawnregion
		addText( 50, 120, tr( "Name: %1" ).arg( region->id() ), 0x834 );
		addText( 50, 140, tr( "NPCs: %1 of %2" ).arg( region->npcs() ).arg( region->maxNpcs() ), 0x834 );
		addText( 50, 160, tr( "Items: %1 of %2" ).arg( region->items() ).arg( region->maxItems() ), 0x834 );
		if ( region->active() )
		{
			addText( 50, 180, tr( "Status: Active" ), 0x834 );
		}
		else
		{
			addText( 50, 180, tr( "Status: Inactive" ), 0x834 );
		}
		addText( 50, 200, tr( "Groups: %1" ).arg( region->groups().join( ", " ) ), 0x834 );

		// Next Spawn
		unsigned int nextRespawn = 0;
		if ( region->nextTime() > Server::instance()->time() )
		{
			nextRespawn = ( region->nextTime() - Server::instance()->time() ) / 1000;
		}
		addText( 50, 220, tr( "Next Respawn: %1 seconds" ).arg( nextRespawn ), 0x834 );
		addText( 50, 240, tr( "Total Points: %1" ).arg( region->countPoints() ), 0x834 );
		addText( 50, 260, tr( "Delay: %1 to %2 seconds" ).arg( region->minTime() ).arg( region->maxTime() ), 0x834 );

		//addText( 50, 180, tr( "Coordinates: %1" ).arg( allrectangles.size() ), 0x834 );

		// OK button
		addButton( 50, 410, 0xF9, 0xF8, 0 ); // Only Exit possible
	}
}

void cSpawnRegionInfoGump::handleResponse( cUOSocket* socket, const gumpChoice_st& choice )
{
	if ( choice.button == 0 )
		return;

	if ( region_ )
	{
		cSpawnRegionInfoGump* pGump = new cSpawnRegionInfoGump( region_ );
		socket->send( pGump );
	}
}
