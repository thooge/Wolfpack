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

#include "basics.h"
#include "muls/tilecache.h"
#include "speech.h"
#include "mapobjects.h"
#include "serverconfig.h"
#include "skills.h"
#include "multi.h"
#include "muls/maps.h"
#include "network/network.h"
#include "network/uosocket.h"
#include "network/uorxpackets.h"
#include "network/uotxpackets.h"
#include "dragdrop.h"
#include "player.h"
#include "npc.h"
#include "items.h"
#include "world.h"
#include "inlines.h"

// New Class implementation
void DragAndDrop::grabItem( cUOSocket* socket, cUORxDragItem* packet )
{
	// Get our character
	P_PLAYER pChar = socket->player();
	if ( !pChar )
		return;

	float weight = pChar->weight();

	// Fetch the grab information
	UI16 amount = packet->amount();
	if ( !amount )
		amount = 1;

	P_ITEM pItem = FindItemBySerial( packet->serial() );

	// If it's an invalid pointer we can't even bounce
	if ( !pItem )
		return;

	// Are we already dragging an item ?
	// Bounce it and reject the move
	// (Logged out while dragging an item)
	if ( socket->dragging() )
	{
		socket->bounceItem( socket->dragging(), BR_ALREADY_DRAGGING );
		return;
	}

	// No Drag & Drop possible while in jail
	if (pChar->isJailed()) {
		socket->sysMessage( tr("While you are jailed, you cannot pick up items.") );
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Check if the item can be reached
	if ( !pChar->isGM() && pItem->getOutmostChar() != pChar && !pChar->lineOfSight( pItem ) ) {
		socket->bounceItem( pItem, BR_OUT_OF_REACH );
		return;
	}

	// Check recursively if the item is in a locked container.
	if (pItem->isInLockedItem()) {
		socket->sysMessage(tr( "You cannot grab items in locked containers."));
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	P_ITEM outmostCont = pItem->getOutmostItem();

	cMulti *multi = outmostCont->multi();
	// Check security if using items within a multi
	if ( multi && multi->canHandleEvent( EVENT_CHECKSECURITY ) )
	{
		PyObject *args = Py_BuildValue( "(NNN)", pChar->getPyObject(), multi->getPyObject(), outmostCont->getPyObject() );
		bool result = multi->callEventHandler( EVENT_CHECKSECURITY, args );
		Py_DECREF( args );

		if ( result )
		{
			socket->bounceItem( pItem, BR_NO_REASON );
			return; // Access Denied
		}
	}

	if ( pItem->onPickup( pChar ) )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	if ( pChar->onPickup( pItem ) )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	if ( pItem->container() )
	{
		if ( pItem->container()->isItem() )
		{
			P_ITEM pContainer = dynamic_cast<P_ITEM>( pItem->container() );
			if ( pContainer->onPickupFromContainer( pChar, pItem ) )
				return;
		}
	}

	// Do we really want to let him break his meditation
	// When he picks up an item ?
	// Maybe a meditation check here ?!?
	pChar->disturbMed(); // Meditation

	P_CHAR itemOwner = pItem->getOutmostChar();

	// Try to pick something out of another characters posessions
	if ( !pChar->isGM() && itemOwner && ( itemOwner != pChar ) && !( itemOwner->objectType() == enNPC && dynamic_cast<P_NPC>( itemOwner )->owner() == pChar ) )
	{
		socket->bounceItem( pItem, BR_BELONGS_TO_SOMEONE_ELSE );
		return;
	}

	// Check if the user can grab the item
	if ( !pChar->canPickUp( pItem ) )
	{
		socket->bounceItem( pItem, BR_CANNOT_PICK_THAT_UP );
		return;
	}

	// The user can't see the item
	// Basically thats impossible as the socket should deny moving the item
	// if it's not in line of sight but to prevent exploits
	/*if( !line_of_sight( socket->socket(), pChar->pos, pItem->pos, TREES_BUSHES|WALLS_CHIMNEYS|DOORS|ROOFING_SLANTED|FLOORS_FLAT_ROOFING|LAVA_WATER ) )
	{
		socket->sysMessage( "You can't see the item." );
		bounceItem( socket, pItem, true );
		return;
	}*/

	// If the top-most container ( thats important ) is a corpse
	// and looting is a crime, flag the character criminal.
	if ( !Config::instance()->disableKarma() && !pChar->isGM() && outmostCont && outmostCont->corpse() )
	{
		// For each item we take out we loose carma
		// if the corpse is innocent and not in our guild
		bool sameGuild = true;//( GuildCompare( pChar, outmostCont->owner() ) != 0 );

		if ( outmostCont->hasTag( "notoriety" ) && outmostCont->getTag( "notoriety" ).toInt() == 1 && !pChar->owns( outmostCont ) && !sameGuild )
		{
			//			pChar->karma -= 5;
			pChar->setKarma( pChar->karma() - 5 );
			// Calling Become Criminal Event
			if (pChar->onBecomeCriminal(3, NULL, outmostCont ))
				pChar->makeCriminal();
			socket->sysMessage( 1019064 ); // You have lost some karma.
		}
	}

	// Check if the item is too heavy
	//if( !pc_currchar->isGMorCounselor() )
	//{
	//} << Deactivated (DarkStorm)

	// ==== Grabbing the Item is allowed here ====

	// Log that the item has been picked up,
	// if we're a gamemaster (This is here because gms tend to be untrusted)
	if ( pChar->isGM() && itemOwner && itemOwner != pChar ) {
		P_PLAYER owner = dynamic_cast<P_PLAYER>(itemOwner);
		if (owner) {
			pChar->log(LOG_TRACE, tr("Picking up item '%1' (0x%2, %3) from player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->orgName() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
		}
	}
	if ( pChar->isGM() && !itemOwner && pItem->container() ) {
		P_ITEM pCont = dynamic_cast<P_ITEM>(pItem->container());
		if (pCont) {
			pChar->log(LOG_TRACE, tr("Grabbing item '%1' (0x%2, %3) out of container 0x%4 (Outmost: 0x%5)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( pCont->serial(), 0, 16 ).arg( outmostCont->serial(), 0, 16 ));
		}
	}

	// Send the user a pickup sound if we're picking it up
	// From a container/paperdoll
	if ( !pItem->isInWorld() )
		socket->soundEffect( 0x57, pItem );

	// If we're picking up a specific amount of what we got
	// Take that into account
	if ( amount < pItem->amount() )
	{
		UI32 pickedAmount = wpMin<UI32>( amount, pItem->amount() );

		// We only have to split if we're not taking it all
		if ( pickedAmount != pItem->amount() )
		{
			P_ITEM splitItem = pItem->dupe(); // Create a new item to pick that up
			splitItem->setAmount( pItem->amount() - pickedAmount );

			// Add tags to the splitted item
			QStringList keys = pItem->getTags();
			QStringList::const_iterator it = keys.begin();
			for ( ; it != keys.end(); ++it )
			{
				splitItem->setTag( *it, pItem->getTag( *it ) );
			}

			P_ITEM pContainer = dynamic_cast<P_ITEM>( pItem->container() );
			if ( pContainer )
				pContainer->addItem( splitItem, false );
			splitItem->SetOwnSerial( pItem->ownSerial() );

			splitItem->setSpawnregion( pItem->spawnregion() );

			// He needs to see the new item
			splitItem->update();

			// If we're taking something out of a spawn-region it's spawning "flag" is removed isn't it?
			pItem->setAmount( pickedAmount );
		}
	}

	// *normally* we should exclude the dragging socket here. but it works so as well.
	pItem->removeFromView( true );

	// Remove from spawnregion
	pItem->setSpawnregion( 0 );

	pChar->addItem( cBaseChar::Dragging, pItem );

	if ( weight != pChar->weight() )
	{
		socket->sendStatWindow();
	}

	// If the logmask contains LOG_TRACE,
	// Log pickups
	if (outmostCont && outmostCont != pItem) {
		if (outmostCont->corpse()) {
			cCorpse *corpse = static_cast<cCorpse*>(outmostCont);
			P_PLAYER owner = dynamic_cast<P_PLAYER>(corpse->owner());
			if (owner && owner != pChar) {
				pChar->log(LOG_TRACE, tr("Picking up item '%1' (0x%2, %3) from corpse of player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->orgName() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
			}
		}
	}
}

// Tries to equip an item
// if that fails it tries to put the item in the users backpack
// if *that* fails it puts it at the characters feet
// That works for NPCs as well
void equipItem( P_CHAR wearer, P_ITEM item )
{
	tile_st tile = TileCache::instance()->getTile( item->id() );

	// User cannot wear the item
	if ( tile.layer == 0 )
	{
		if ( wearer->objectType() == enPlayer )
		{
			P_PLAYER pp = dynamic_cast<P_PLAYER>( wearer );
			if ( pp->socket() )
				pp->socket()->sysMessage( tr( "You cannot wear that item." ) );
		}

		item->toBackpack( wearer );
		return;
	}

	cBaseChar::ItemContainer container = wearer->content();
	cBaseChar::ItemContainer::const_iterator it( container.begin() );
	for ( ; it != container.end(); ++it )
	{
		P_ITEM equip = *it;

		// Unequip the item and free the layer that way
		if ( equip && ( equip->layer() == tile.layer ) )
			equip->toBackpack( wearer );
	}

	// *finally* equip the item
	wearer->addItem( static_cast<cBaseChar::enLayer>( item->layer() ), item );
}

void DragAndDrop::bounceItem( cUOSocket* socket, P_ITEM pItem, bool /*denyMove*/ )
{
	// Reject the move of the item
	socket->bounceItem( pItem, BR_NO_REASON );

	// If the Client is *not* dragging the item we don't need to reset it to it's original location
	if ( !socket->dragging() )
		return;

	// Sends the item to the backpack of char (client)
	pItem->toBackpack( socket->player() );

	// When we're dropping the item to the ground let's play a nice sound-effect
	// to all in-range sockets
	if ( pItem->isInWorld() )
	{
		QList<cUOSocket*> sockets = Network::instance()->sockets();
		foreach ( cUOSocket* mSock, sockets )
		{
			if ( mSock->inRange( socket ) )
				mSock->soundEffect( 0x42, pItem );
		}
	}
	else
		socket->soundEffect( 0x57, pItem );
}

void DragAndDrop::equipItem( cUOSocket* socket, cUORxWearItem* packet )
{
	P_ITEM pItem = FindItemBySerial( packet->serial() );
	P_CHAR pWearer = FindCharBySerial( packet->wearer() );

	if ( !pItem || !pWearer )
		return;

	P_PLAYER pChar = socket->player();

	// If the item is not dragged by us, dont even bother
	if ( pItem->container() != pChar )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// We're dead and can't do that
	if ( pChar->isDead() )
	{
		socket->clilocMessage( 0x7A4D5, "", 0x3b2 ); // You can't do that when you're dead.
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// No Special Layer Equipping
	if ( ( packet->layer() > cBaseChar::InnerLegs || packet->layer() <= cBaseChar::TradeWindow ) && !pChar->isGM() )
	{
		socket->sysMessage( tr( "You can't equip on that layer." ) );
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Our target is dead
	if ( ( pWearer != pChar ) && pWearer->isDead() )
	{
		socket->sysMessage( tr( "You can't equip dead players." ) );
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Only GM's can equip other People
	if ( pWearer != pChar && !pChar->isGM() )
	{
		P_NPC pNpc = dynamic_cast<P_NPC>( pWearer );

		// But we are allowed to equip our own humans
		if ( !pNpc || ( pNpc->owner() != pChar && pWearer->isHuman() ) )
			socket->sysMessage( tr( "You can't equip other players." ) );

		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Get our tile-information
	tile_st pTile = TileCache::instance()->getTile( pItem->id() );

	// Is the item wearable ? ( layer == 0 | equip-flag not set )
	// Multis are not wearable are they :o)
	if ( pTile.layer == 0 || !( pTile.flag3 & 0x40 ) || pItem->isMulti() )
	{
		socket->sysMessage( tr( "This item cannot be equipped." ) );
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Check the Script for it
	if ( pItem->onWearItem( pChar, pWearer, packet->layer() ) )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	if ( pWearer->onWearItem( pChar, pItem, packet->layer() ) )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Males can't wear female armor
	if ( ( pChar->body() == 0x0190 ) && ( pItem->id() >= 0x1C00 ) && ( pItem->id() <= 0x1C0D ) )
	{
		socket->sysMessage( 1010388 ); // Only females can wear this
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// Needs a check (!)
	// Checks for equipment on the same layer
	// If there is any it tries to unequip it
	// If that fails it cancels
	// we also need to check if there is a twohanded weapon if we want to equip another weapon.
	UI08 layer = pTile.layer;

	P_ITEM equippedLayerItem = pWearer->atLayer( static_cast<cBaseChar::enLayer>( layer ) );

	// we're equipping so we do the check
	if ( equippedLayerItem )
	{
		if ( pChar->canPickUp( equippedLayerItem ) )
		{
			equippedLayerItem->toBackpack( pWearer );
		}
		else
		{
			socket->sysMessage( tr( "You can't wear another item there!" ) );
			socket->bounceItem( pItem, BR_NO_REASON );
			return;
		}
	}

	// Check other layers if neccesary
	bool occupied = false;

	if ( pItem->twohanded() && layer == 1 )
	{
		occupied = pWearer->leftHandItem() != 0;  // Twohanded weapon on layer 1 forbids item on layer 2
	}
	else if ( pItem->twohanded() && layer == 2 )
	{
		occupied = pWearer->rightHandItem() != 0;  // Twohanded weapon on layer 2 forbids item on layer 1
	}
	else if ( layer == 1 )
	{
		P_ITEM lefthand = pWearer->leftHandItem();
		occupied = lefthand && lefthand->twohanded();
	}
	else if ( layer == 2 )
	{
		P_ITEM righthand = pWearer->rightHandItem();
		occupied = righthand && righthand->twohanded();
	}

	if ( occupied )
	{
		socket->sysMessage( tr( "You can't hold another item while wearing a twohanded item!" ) );
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	// At this point we're certain that we can wear the item
	pWearer->addItem( static_cast<cBaseChar::enLayer>( pTile.layer ), pItem );

	if ( pWearer->objectType() == enPlayer )
	{
		P_PLAYER pp = dynamic_cast<P_PLAYER>( pWearer );
		if ( pp->socket() )
			pp->socket()->sendStatWindow();
	}

	// I don't think we need to remove the item
	// as it's only visible to the current char
	// And he looses contact anyway

	// Build our packets
	cUOTxCharEquipment wearItem;
	wearItem.fromItem( pItem );

	cUOTxSoundEffect soundEffect;
	soundEffect.setSound( 0x57 );
	soundEffect.setCoord( pWearer->pos() );

	// Send to all sockets in range
	// ONLY the new equipped item and the sound-effect
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( cUOSocket* mSock, sockets )
	{
		if ( mSock->player() && ( mSock->player()->dist( pWearer ) <= mSock->player()->visualRange() ) )
		{
			mSock->send( &wearItem );
			mSock->send( &soundEffect );
		}
	}
}

void DragAndDrop::dropItem( cUOSocket* socket, cUORxDropItem* packet )
{
	P_PLAYER pChar = socket->player();

	if ( !pChar )
		return;

	// Get the data
	//SERIAL contId = packet->cont();

	Coord dropPos = pChar->pos(); // plane
	dropPos.x = packet->x();
	dropPos.y = packet->y();
	dropPos.z = packet->z();

	// Get possible containers
	P_ITEM pItem = FindItemBySerial( packet->serial() );

	if ( !pItem )
		return;

	// If the item is not dragged by us, dont even bother
	if ( pItem->container() != pChar )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	P_ITEM iCont = FindItemBySerial( packet->cont() );
	P_CHAR cCont = FindCharBySerial( packet->cont() );

	// A completely invalid Drop packet
	if ( !iCont && !cCont && ( dropPos.x == 0xFFFF ) && ( dropPos.y == 0xFFFF ) )
	{
		socket->bounceItem( pItem, BR_NO_REASON );
		return;
	}

	float weight = pChar->weight();

	// Item dropped on Ground
	if ( !iCont && !cCont )
		dropOnGround( socket, pItem, dropPos );

	// Item dropped on another item
	else if ( iCont )
		dropOnItem( socket, pItem, iCont, dropPos );

	// Item dropped on char
	else if ( cCont )
		dropOnChar( socket, pItem, cCont );

	// Handle the sound-effect
	if ( pItem->id() == 0xEED )
		pChar->goldSound( pItem->amount() );

	// Update our weight.
	if ( weight != pChar->weight() )
		socket->sendStatWindow();
}

void DragAndDrop::dropOnChar( cUOSocket* socket, P_ITEM pItem, P_CHAR pOtherChar )
{
	// Three possibilities:
	// If we're dropping it on ourself: packintobackpack
	// If we're dropping it on some other player: trade-window
	// If we're dropping it on some NPC: checkBehaviours
	// If not handeled: Equip the item if the NPC is owned by us

	P_CHAR pChar = socket->player();

	if ( pItem->onDropOnChar( pOtherChar ) )
	{
		// Still dragging? Bounce!
		if ( socket->dragging() == pItem )
			socket->bounceItem( pItem, BR_NO_REASON );

		return;
	}

	if ( pOtherChar->onDropOnChar( pItem ) )
	{
		// Still dragging? Bounce!
		if ( socket->dragging() == pItem )
			socket->bounceItem( pItem, BR_NO_REASON );

		return;
	}

	// Dropped on ourself
	if ( pChar == pOtherChar )
	{
		pItem->toBackpack( pChar );
		return;
	}

	// Are we in range of our target
	if ( !pChar->inRange( pOtherChar, 3 ) )
	{
		socket->bounceItem( pItem, BR_OUT_OF_REACH );
		return;
	}

	// Can wee see our target
	if ( !pChar->lineOfSight( pOtherChar ) )
	{
		socket->bounceItem( pItem, BR_OUT_OF_SIGHT );
		return;
	}

	// Open a secure trading window
	if ( pOtherChar->objectType() == enPlayer && dynamic_cast<P_PLAYER>( pOtherChar )->socket() )
	{
		dynamic_cast<P_PLAYER>( pChar )->onTradeStart( dynamic_cast<P_PLAYER>( pOtherChar ), pItem );
		return;
	}

	socket->sysMessage( tr( "The character does not seem to want the item." ) );
	socket->bounceItem( pItem, BR_NO_REASON );
	return;
}

void DragAndDrop::dropOnGround( cUOSocket* socket, P_ITEM pItem, const Coord& pos )
{
	P_PLAYER pChar = socket->player();

	// Check if the destination is in line of sight
	if ( !pChar->isGM() && !pChar->lineOfSight( pos.losItemPoint( pItem->id() ) ) )
	{
		socket->bounceItem( pItem, BR_OUT_OF_SIGHT );
		return;
	}

	if ( !pChar->canPickUp( pItem ) )
	{
		socket->bounceItem( pItem, BR_CANNOT_PICK_THAT_UP );
		return;
	}

	if ( pItem->onDropOnGround( pos ) )
	{
		// We're still dragging something
		if ( socket->dragging() )
			socket->bounceItem( socket->dragging(), BR_NO_REASON );

		return;
	}

	pItem->removeFromCont();
	pItem->moveTo( pos );
	pItem->update();

	// Play Sounds for non gold items
	if ( pItem->id() != 0xEED )
	{
		pItem->soundEffect( 0x42 );
	}
}

inline char calcSpellId( cItem* item )
{
	tile_st tile = TileCache::instance()->getTile( item->id() );

	if ( tile.unknown1 == 0 )
		return -1;
	else
		return tile.unknown1 - 1;
}

void DragAndDrop::dropOnItem( cUOSocket* socket, P_ITEM pItem, P_ITEM pCont, const Coord& dropPos )
{
	P_PLAYER pChar = socket->player();

	if ( pItem->isMulti() )
	{
		socket->sysMessage( tr( "You cannot put houses in containers" ) );
		cUOTxBounceItem bounce;
		bounce.setReason( BR_NO_REASON );
		socket->send( &bounce );
		pItem->remove();
		return;
	}

	P_ITEM outmostCont = pCont->getOutmostItem();
	cMulti *multi = outmostCont->multi();
	// Check security if using items within a multi
	if ( multi && multi->canHandleEvent( EVENT_CHECKSECURITY ) )
	{
		PyObject *args = Py_BuildValue( "(NNN)", pChar->getPyObject(), multi->getPyObject(), outmostCont->getPyObject() );
		bool result = multi->callEventHandler( EVENT_CHECKSECURITY, args );
		Py_DECREF( args );

		if ( result )
		{
			socket->bounceItem( socket->dragging(), BR_NO_REASON );
			return; // Access Denied
		}
	}

	if ( pItem->onDropOnItem( pCont ) )
	{
		if ( pItem->free )
			return;

		if ( socket->dragging() )
			socket->bounceItem( socket->dragging(), BR_NO_REASON );

		return;
	}
	else if ( pCont->onDropOnItem( pItem ) )
	{
		if ( pItem->free )
			return;

		if ( socket->dragging() )
			socket->bounceItem( socket->dragging(), BR_NO_REASON );

		return;
	}

	//It is possible that a script deletes the item, but do not return a value
	//if this is not checked, the function adds a deleted item to a container
	if ( pItem->free )
			return;

	// If the target belongs to another character
	// It needs to be our vendor or else it's denied
	P_CHAR packOwner = pCont->getOutmostChar();

	if ( ( packOwner ) && ( packOwner != pChar ) && !pChar->isGM() )
	{
		// For each item someone puts into there
		// He needs to do a snoop-check
		if ( pChar->maySnoop() )
		{
			if ( !pChar->checkSkill( SNOOPING, 0, 1000 ) )
			{
				socket->sysMessage( tr( "You fail to put that into %1's pack" ).arg( packOwner->name() ) );
				socket->bounceItem( pItem, BR_NO_REASON );
				return;
			}
		}

		if ( packOwner->objectType() == enPlayer || ( packOwner->objectType() == enNPC && dynamic_cast<P_NPC>( packOwner )->owner() != pChar ) )
		{
			socket->sysMessage( tr( "You cannot put that into the belongings of another player" ) );
			socket->bounceItem( pItem, BR_NO_REASON );
			return;
		}
	}

	if ( !pChar->canPickUp( pItem ) )
	{
		socket->bounceItem( pItem, BR_CANNOT_PICK_THAT_UP );
		return;
	}

	// We drop something on the belongings of one of our playervendors
	/*	if( ( packOwner != NULL ) && ( packOwner->npcaitype() == 17 ) && packOwner->owner() == pChar )
	{
	socket->sysMessage( tr( "You drop something into your playervendor (unimplemented)" ) );
	socket->bounceItem( pItem, BR_NO_REASON );
	return;
	}*/

	// Playervendors (chest equipped by the vendor - opened to the client)

	/*if( !( pCont->pileable() && pItem->pileable() && pCont->id() == pItem->id() || ( pCont->type() != 1 && pCont->type() != 9 ) ) )
	{
	P_CHAR pc_j = GetPackOwner(pCont);
	if (pc_j != NULL)
	{
	if (pc_j->npcaitype() == 17 && pc_j->isNpc() && pChar->Owns(pc_j))
	{
	pChar->inputitem = pItem->serial;
	pChar->inputmode = cChar::enPricing;
	sysmessage(s, "Set a price for this item.");
	}
	}
	*/

	// We may also drop into *any* locked chest
	// So we can have post-boxes ;o)
	if ( pCont->type() == 1 )
	{
		//if ( pCont->layer() == 29 )
		//{
		//	pCont->addItem(pItem)
		//}
		// Check if we can carry the item or if it's too heavy
		if (packOwner && ( pItem->totalweight() > packOwner->maxWeight() ) && !pChar->isGM() && !(pCont->getOutmostItem()->layer() == 29))
		{
			pItem->removeFromCont();
			pItem->moveTo( packOwner->pos() );
			pItem->update();

			// Play Sounds for non gold items
			if ( pItem->id() != 0xEED )
			{
				pItem->soundEffect( 0x42 );
			}
			return;
		}

		// If we're dropping it onto the closed container
		if ( dropPos.x == 0xFFFF && dropPos.y == 0xFFFF )
		{
			pCont->addItem( pItem );
		}
		else
		{
			pCont->addItem( pItem, false );
			pItem->moveTo( dropPos );
		}

		// Dropped on another Container/in another Container
		if ( pCont->corpse() )
		{
			pChar->soundEffect( 0x42 );
		}
		else
		{
			pChar->soundEffect( 0x57 );
		}
		pItem->update();

		// If the logmask contains LOG_TRACE, log drops
		if (outmostCont && outmostCont != pItem)
		{
			if (outmostCont->corpse())
			{
				cCorpse *corpse = static_cast<cCorpse*>(outmostCont);
				P_PLAYER owner = dynamic_cast<P_PLAYER>(corpse->owner());
				if (owner && owner != pChar)
				{
					pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) onto corpse of player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->name() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
				}
			}
		}
		if ( pChar->isGM() && packOwner && packOwner != pChar )
		{
			P_PLAYER owner = dynamic_cast<P_PLAYER>(packOwner);
			if (owner)
			{
				pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) to player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->orgName() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
			}
		}
		if ( pChar->isGM() && !packOwner )
		{
			pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) into container 0x%4 (Outmost: 0x%5)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( pCont->serial(), 0, 16 ).arg( outmostCont->serial(), 0, 16 ));
		}
		return;
	}
	else if ( pCont->canStack( pItem ) )
	{
		if ( pCont->amount() + pItem->amount() <= 65535 )
		{
			pCont->setAmount( pCont->amount() + pItem->amount() );

			pItem->remove();
			pCont->update(); // Need to update the amount
			pCont->resendTooltip();
			return;
		}
		else
		{
			// The delta between 65535 and pCont->amount() sub our Amount is the
			// new amount
			pItem->setAmount( pItem->amount() - ( 65535 - pCont->amount() ) );
			pItem->resendTooltip();

			pCont->setAmount( 65535 ); // Max out the amount
			pCont->update();
			pCont->resendTooltip();
		}
	}

	// We dropped the item NOT on a container
	// And were *un*able to stack it (!)
	// >> Set it to the location of the item we dropped it on and stack it up by 2
	if ( pCont->container() )
	{
		P_ITEM pNewCont = dynamic_cast<P_ITEM>( pCont->container() );

		if ( pNewCont )
		{
			pNewCont->addItem( pItem, false );
			pItem->moveTo( pCont->pos() + Coord( 0, 0, 2 ) );

			// If the logmask contains LOG_TRACE, log drops
			if (outmostCont && outmostCont != pItem)
			{
				if (outmostCont->corpse())
				{
					cCorpse *corpse = static_cast<cCorpse*>(outmostCont);
					P_PLAYER owner = dynamic_cast<P_PLAYER>(corpse->owner());
					if (owner && owner != pChar)
					{
						pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) onto corpse of player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->name() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
					}
				}
			}
			if ( pChar->isGM() && packOwner && packOwner != pChar )
			{
				P_PLAYER owner = dynamic_cast<P_PLAYER>(packOwner);
				if (owner)
				{
					pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) to player '%4' ('%5', 0x%6)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( owner->orgName() ).arg( owner->account() ? owner->account()->login() : QString( "unknown" ) ).arg( owner->serial(), 0, 16 ));
				}
			}
			if ( pChar->isGM() && !packOwner ) {
				pChar->log(LOG_TRACE, tr("Dropping item '%1' (0x%2, %3) into container 0x%4 (Outmost: 0x%5)\n").arg(QString(pItem->baseid())).arg(pItem->serial(), 0, 16).arg(pItem->amount()).arg( pNewCont->serial(), 0, 16 ).arg( outmostCont->serial(), 0, 16 ));
			}
		}
		else
		{
			pChar->getBackpack()->addItem( pItem );
		}
	}
	else
	{
		pItem->removeFromCont();
		pItem->moveTo( pCont->pos() + Coord( 0, 0, 2 ) );
	}

	pItem->update();
}
