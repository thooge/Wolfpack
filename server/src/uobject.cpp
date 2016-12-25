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

// Platform Specifics
#include "platform.h"

// Wolfpack includes
#include "coord.h"
#include "uobject.h"

#include "network/network.h"
#include "console.h"
#include "defines.h"
#include "pythonscript.h"
#include "scriptmanager.h"
#include "network/uosocket.h"
#include "network/uotxpackets.h"
#include "definitions.h"
#include "muls/maps.h"
#include "persistentbroker.h"
#include "dbdriver.h"
#include "basechar.h"
#include "multi.h"
#include "mapobjects.h"
#include "player.h"
#include "basics.h"
#include "items.h"
#include "serverconfig.h"
#include "basics.h"
#include "world.h"
#include "typedefs.h"

#include <QByteArray>
#include <QSqlQuery>
#include <QVariant>

// Library Includes

cUObject::cUObject() : serial_( INVALID_SERIAL ), multi_( 0 ), free( false ), changed_( true ), tooltip_( 0xFFFFFFFF ), name_( QString::null ), scriptChain( 0 ), spawnregion_( 0 )
{
	pos_.setInternalMap(); // We're not in the map objects yet
}

cUObject::~cUObject()
{
	if ( scriptChain )
	{
		if ( isScriptChainFrozen() )
		{
			size_t count = reinterpret_cast<size_t>( scriptChain[0] );
			for ( size_t i = 1; i <= count; ++i )
			{
				QByteArray* str = reinterpret_cast<QByteArray*>( scriptChain[i] );
				delete str;
			}
		}

		delete[] scriptChain;
	}
}

cUObject::cUObject( const cUObject& src ) : cDefinable( src ), cPythonScriptable( src ), PersistentObject( src )
{
	// Copy Events
	if ( src.scriptChain )
	{
		size_t count = reinterpret_cast<size_t>( src.scriptChain[0] );
		scriptChain = new cPythonScript * [count + 1];
		memcpy( scriptChain, src.scriptChain, ( count + 1 ) * sizeof( cPythonScript * ) );
	}
	else
	{
		scriptChain = 0;
	}

	if ( src.multi_ )
	{
		src.multi_->addObject( this );
	}
	this->name_ = src.name_;
	this->pos_ = src.pos_;
	pos_.setInternalMap();
	this->tags_ = src.tags_;
	this->spawnregion_ = 0; // SpawnRegion references aren't transferred
	changed_ = true;
}

// static definitions
QSqlQuery * cUObject::insertQuery_ = NULL;
QSqlQuery * cUObject::updateQuery_ = NULL;
QSqlQuery * cUObject::uobjectmapQuery_ = NULL;

void cUObject::moveTo( const Coord& newpos )
{
	if ( pos_ == newpos )
	{
		return; // Nothing changed
	}

	// See if the map is valid
	if ( !newpos.isInternalMap() && !Maps::instance()->hasMap( newpos.map ) )
	{
		return;
	}

	// We're moved to the internal map although we're not on the internal map
	if ( newpos.isInternalMap() && !pos_.isInternalMap() )
	{
		MapObjects::instance()->remove( this ); // Remove from the sectors
		if ( multi_ )
		{
			multi_->removeObject( this );
			multi_ = 0;
		}
	}
	else if ( pos_.isInternalMap() && !newpos.isInternalMap() )
	{
		pos_ = newpos; // Add uses this coordinate internally
		MapObjects::instance()->add( this ); // Add to the sectors
	}
	else if ( !newpos.isInternalMap() )
	{
		MapObjects::instance()->update( this, newpos );
	}

	pos_ = newpos;
	changed_ = true;

	// We're not on an internal map
	if ( !pos_.isInternalMap() )
	{
		// Position Changed
		cMulti* multi = cMulti::find( newpos );
		// Don't put multis into themselves
		if ( multi != this && multi_ != multi )
		{
			if ( multi_ )
			{
				multi_->removeObject( this );
			}

			if ( multi )
			{
				multi->addObject( this );
			}

			multi_ = multi;
		}
	}
}

/*!
	Returns the distance between this object and \a d
*/
unsigned int cUObject::dist( cUObject* d ) const
{
	if ( !d )
		return static_cast<uint>( ~0 );
	return pos_.distance( d->pos_ );
}

/*!
	Performs persistency layer loads.
*/
void cUObject::load( QSqlQuery& result, ushort& offset )
{
	name_ = ( result.value(offset).toByteArray().isEmpty() ) ? QString::null : QString::fromUtf8( result.value(offset).toByteArray() );
	offset++;
	serial_ = result.value( offset++ ).toInt();
	multi_ = reinterpret_cast<cMulti*>( static_cast<size_t>( result.value(offset++).toInt() ) );
	pos_.x = result.value( offset++ ).toInt();
	pos_.y = result.value( offset++ ).toInt();
	pos_.z = result.value( offset++ ).toInt();
	pos_.map = result.value( offset++ ).toInt();
	QByteArray scriptList = result.value( offset++ ).toByteArray();
	bool havetags_ = result.value( offset++ ).toBool();

	setScriptList( scriptList );

	if ( havetags_ )
		tags_.load( serial_ );

	PersistentObject::load( result, offset );
}

/*!
	Performs persistency layer saves.
*/
void cUObject::save()
{
	bool havetags_ = ( tags_.size() > 0 );

	// If the type is changed somewhere in the code
	// That part needs to take care of delete/recreate
	// So we never update the type EVER here..
	if ( !isPersistent )
	{
		QSqlQuery * q = cUObject::getUObjectmapQuery();
		q->addBindValue( serial_ );
		q->addBindValue( QString( objectID() ) );
		q->exec();
	}

	if ( changed_ )
	{
		QSqlQuery * q;
		if ( isPersistent )
			q = cUObject::getUpdateQuery();
		else
			q = cUObject::getInsertQuery();

		q->addBindValue( name_ );
		q->addBindValue( serial_ );
		q->addBindValue( multi_ ? multi_->serial() : INVALID_SERIAL );
		q->addBindValue( pos_.x );
		q->addBindValue( pos_.y );
		q->addBindValue( pos_.z );
		q->addBindValue( pos_.map );
		QString scriptList = this->scriptList();
		q->addBindValue( scriptList == QString::null ? QString( "" ) : scriptList );
		q->addBindValue( havetags_ );
		if ( isPersistent )
			q->addBindValue( serial_ );
		q->exec();
	}

	if ( havetags_ )
	{
		tags_.save( serial_ );
	}

	PersistentObject::save();
	flagUnchanged(); // This is the botton of the chain, now go up and flag everyone unchanged.
}

void cUObject::save( cBufferedWriter& writer, unsigned int /*version*/ )
{
	writer.writeUtf8( name_ );
	writer.writeInt( serial_ );
	writer.writeInt( multi_ ? multi_->serial() : INVALID_SERIAL );
	writer.writeShort( pos_.x );
	writer.writeShort( pos_.y );
	writer.writeByte( pos_.z );
	writer.writeByte( pos_.map );
	writer.writeAscii( scriptList() );
}

void cUObject::load( cBufferedReader& reader, unsigned int /*version*/ )
{
	name_ = reader.readUtf8();
	serial_ = reader.readInt();
	setMulti( dynamic_cast<cMulti*>( World::instance()->findItem( reader.readInt() ) ) );
	if ( multi_ )
	{
		multi_->addObject( this );
	}
	pos_.x = reader.readShort();
	pos_.y = reader.readShort();
	pos_.z = reader.readByte();
	pos_.map = reader.readByte();
	setScriptList( reader.readAscii() );
}

/*!
	Performs persistency layer deletion.
*/
bool cUObject::del()
{
	if ( !isPersistent )
		return false; // We didn't need to delete the object

	PersistentBroker::instance()->addToDeleteQueue( "uobject", QString( "serial = '%1'" ).arg( serial_ ) );
	PersistentBroker::instance()->addToDeleteQueue( "uobjectmap", QString( "serial = '%1'" ).arg( serial_ ) );

	if ( tags_.size() > 0 )
		tags_.del( serial_ );

	changed_ = true;

	return PersistentObject::del();
}

/*!
	Builds the SQL string needed to retrieve all objects of this type.
*/
void cUObject::buildSqlString( const char* objectid, QStringList& fields, QStringList& tables, QStringList& conditions )
{
	// We are requiring fixed order by now, so this *is* possible
	fields.push_back( "uobject.name,uobject.serial,uobject.multis,uobject.pos_x,uobject.pos_y,uobject.pos_z,uobject.pos_map,uobject.events,uobject.havetags" );
	tables.push_back( "uobject" );
	tables.push_back( "uobjectmap" );
	conditions.push_back( "uobjectmap.serial = uobject.serial" );
	conditions.push_back( QString( "uobjectmap.type = '%1'" ).arg( objectid ) );
}

/*!
	Clears the script-chain
*/
void cUObject::clearScripts()
{
	if ( scriptChain )
	{
		cPythonScript** myChain = scriptChain;
		bool frozen = isScriptChainFrozen();
		scriptChain = 0;
		changed_ = true;

		if ( frozen && myChain )
		{
			size_t count = reinterpret_cast<size_t>( myChain[0] );
			for ( size_t i = 1; i <= count; ++i )
			{
				QByteArray* str = reinterpret_cast<QByteArray*>( myChain[i] );
				delete str;
			}
		}
		else if ( !frozen )
		{
			if ( cPythonScript::canChainHandleEvent( EVENT_DETACH, myChain ) )
			{
				PyObject* args = Py_BuildValue( "(N)", getPyObject() );
				cPythonScript::callChainedEventHandler( EVENT_DETACH, myChain, args );
				Py_DECREF( args );
			}
		}
		delete[] myChain;
	}
}

/*!
	Checks if the object has a specific event \a name
	\sa addEvent
*/
bool cUObject::hasScript( const QByteArray& name )
{
	if ( scriptChain )
	{
		size_t count = reinterpret_cast<size_t>( scriptChain[0] );

		for ( size_t i = 1; i <= count; ++i )
		{
			if ( scriptChain[i]->name() == name )
				return true;
		}
	}

	return false;
}

/*!
	Adds an event handler to this object
*/
void cUObject::addScript( cPythonScript* event, bool append )
{
	if ( isScriptChainFrozen() )
	{
		return;
	}

	if ( hasScript( event->name() ) )
	{
		return;
	}

	// Reallocate the ScriptChain
	if ( scriptChain )
	{
		size_t count = reinterpret_cast<size_t>( *scriptChain );

		cPythonScript** newScriptChain = new cPythonScript* [count + 2];
		if ( append || count == 0 )
		{
			memcpy( newScriptChain, scriptChain, ( count + 1 ) * sizeof( cPythonScript * ) );
			newScriptChain[count + 1] = event;
		}
		else
		{
			// make room for the 1st event
			memcpy( &newScriptChain[2], &scriptChain[1], ( count ) * sizeof( cPythonScript * ) );
			newScriptChain[1] = event;
		}
		newScriptChain[0] = reinterpret_cast<cPythonScript*>( count + 1 );

		delete[] scriptChain;
		scriptChain = newScriptChain;
	}
	else
	{
		scriptChain = new cPythonScript * [2];
		scriptChain[0] = reinterpret_cast<cPythonScript*>( 1 );
		scriptChain[1] = event;
	}

	changed_ = true;

	if ( event->canHandleEvent( EVENT_ATTACH ) )
	{
		PyObject* args = Py_BuildValue( "(N)", getPyObject() );
		event->callEvent( EVENT_ATTACH, args );
		Py_DECREF( args );
	}
}

/*!
	Removes an event handler from the object
*/
void cUObject::removeScript( const QByteArray& name )
{
	if ( isScriptChainFrozen() )
	{
		return;
	}

	bool found = false;
	if ( scriptChain )
	{
		size_t count = reinterpret_cast<size_t>( scriptChain[0] );

		for ( size_t i = 1; i <= count; ++i )
		{
			if ( scriptChain[i]->name() == name )
			{
				found = true;
				break;
			}
		}
	}

	if ( !found )
	{
		return;
	}

	cPythonScript* event = 0;

	if ( scriptChain )
	{
		size_t count = reinterpret_cast<size_t>( scriptChain[0] );

		if ( count == 1 )
		{
			clearScripts();
		}
		else
		{
			unsigned int pos = 1;

			cPythonScript** newScriptChain = new cPythonScript*[count];
			newScriptChain[0] = reinterpret_cast<cPythonScript*>( count - 1 );

			for ( size_t i = 1; i <= count; ++i )
			{
				if ( scriptChain[i]->name() != name )
				{
					newScriptChain[pos++] = scriptChain[i];
				}
				else
				{
					event = scriptChain[i];
				}
			}

			delete[] scriptChain;
			scriptChain = newScriptChain;
		}
	}

	changed_ = true;

	if ( event && event->canHandleEvent( EVENT_DETACH ) )
	{
		PyObject* args = Py_BuildValue( "(N)", getPyObject() );
		event->callEvent( EVENT_DETACH, args );
		Py_DECREF( args );
	}
}

void cUObject::processNode( const cElement* Tag, uint /*hash*/ )
{
	QString TagName( Tag->name() );
	QString Value( Tag->value() );

	if ( TagName == "name" )
	{
		name_ = Value;
	}

	// <tag type="string"> also type="value"
	//		<key>multisection</key>
	//		<value>smallboat</value>
	// </tag>
	else if ( TagName == "tag" )
	{
		QString name = Tag->getAttribute( "name" );
		QString value = Tag->getAttribute( "value" );

		if ( !name.isNull() )
		{
			// If there is no value attribute, use the
			// tag content instead.
			if ( value.isNull() )
			{
				value = Tag->text();

				if ( value.isNull() )
				{
					value = "";
				}
			}

			QString type = Tag->getAttribute( "type", "string" );

			if ( type == "int" )
			{
				// If the value is separated by a ,
				// we assume it's a random gradient.
				// If it's separated by ; we assume it's a list of values
				// we should choose from randomly.
				int sep = value.indexOf( ',' );

				if ( sep != -1 )
				{
					int min = hex2dec( value.left( sep ) ).toInt();
					int max = hex2dec( value.mid( sep + 1 ) ).toInt();

					int value = RandomNum( min, max );
					tags_.set( name, cVariant( ( int ) value ) );
				}
				else
				{
					// Choose a random value from the list.
					if ( value.contains( ';' ) )
					{
						QStringList values = value.split( ';' );
						if ( values.size() > 0 )
						{
							value = values[RandomNum( 0, values.size() - 1 )];
						}
					}

					tags_.set( name, cVariant( hex2dec( value ).toInt() ) );
				}
			}
			else if ( type == "float" )
			{
				tags_.set( name, cVariant( value.toFloat() ) );
			}
			else
			{
				tags_.set( name, cVariant( value ) );
			}
		}
	}
	// <scripts>a,b,c</scripts>
	else if ( TagName == "scripts" )
	{
		setScriptList( Value.toLatin1() );
	}
	else
	{
		if ( Value.isEmpty() )
		{
			Value = "1";
		}
		cVariant variant( Value );
		setProperty( TagName, variant );
	}
}

/*!
	Remove it from all in-range sockets
*/
void cUObject::removeFromView( bool clean )
{
	// Get Real pos
	Coord mPos = pos_;

	if ( isItemSerial( serial_ ) )
	{
		P_ITEM pItem = dynamic_cast<P_ITEM>( this );
		P_ITEM pCont = pItem->getOutmostItem();
		if ( pCont )
		{
			mPos = pCont->pos();
			P_CHAR pOwner = dynamic_cast<P_CHAR>( pCont->container() );
			if ( pOwner )
				mPos = pOwner->pos();
		}
	}

	cUOTxRemoveObject remove;
	remove.setSerial( serial_ );
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( cUOSocket* socket, sockets )
	{
		if ( socket->player() != this && ( clean || socket->canSee( this ) ) )
		{
			socket->send( &remove );
		}
	}
}

/*!
	Checks if the specified object is in given range
*/
bool cUObject::inRange( cUObject* object, quint32 range ) const
{
	if ( !object )
		return false;

	Coord pos = object->pos_;

	if ( object->isItem() )
	{
		P_ITEM pItem = dynamic_cast<P_ITEM>( object );

		if ( pItem )
		{
			P_ITEM pCont = pItem->getOutmostItem();
			P_CHAR pEquipped = pItem->getOutmostChar();

			if ( pEquipped )
			{
				pos = pEquipped->pos();
			}
			else if ( pCont )
			{
				pos = pCont->pos();
			}
		}
	}

	return pos_.distance( pos ) <= range;
}

void cUObject::lightning( unsigned short hue )
{
	cUOTxEffect effect;
	effect.setType( ET_LIGHTNING );
	effect.setSource( serial_ );
	effect.setSourcePos( pos_ );
	effect.setTargetPos( pos_ );
	effect.setHue( hue );

	cUOSocket* mSock = 0;
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( mSock, sockets )
	{
		if ( mSock->player() && dist( mSock->player() ) < mSock->player()->visualRange() )
			mSock->send( &effect );
	}
}

/*!
	Displays an effect emitting from this object toward another item or character
*/
void cUObject::effect( quint16 id, cUObject* target, bool fixedDirection, bool explodes, quint8 speed, quint16 hue, quint16 renderMode )
{
	if ( !target )
		return;

	cUOTxEffect effect;
	effect.setType( ET_MOVING );
	effect.setSource( serial_ );
	effect.setTarget( target->serial_ );
	effect.setSourcePos( pos_ );
	effect.setTargetPos( target->pos_ );
	effect.setId( id );
	effect.setSpeed( speed );
	effect.setDuration( 1 );
	effect.setExplodes( explodes );
	effect.setFixedDirection( fixedDirection );
	effect.setHue( hue );
	effect.setRenderMode( renderMode );

	cUOSocket* mSock = 0;
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( mSock, sockets )
	{
		if ( !mSock->player() )
			continue;

		// The Socket has to be either in range of Source or Target
		if ( mSock->player()->inRange( this, mSock->player()->visualRange() ) || mSock->player()->inRange( target, mSock->player()->visualRange() ) )
			mSock->send( &effect );
	}
}

/*!
	Displays an effect emitting from this object and moving towards a specific location.
*/
void cUObject::effect( quint16 id, const Coord& target, bool fixedDirection, bool explodes, quint8 speed, quint16 hue, quint16 renderMode )
{
	cUOTxEffect effect;
	effect.setType( ET_MOVING );
	effect.setSource( serial_ );
	effect.setSourcePos( pos_ );
	effect.setTargetPos( target );
	effect.setId( id );
	effect.setSpeed( speed );
	effect.setDuration( 1 );
	effect.setExplodes( explodes );
	effect.setFixedDirection( fixedDirection );
	effect.setHue( hue );
	effect.setRenderMode( renderMode );

	cUOSocket* mSock = 0;
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( mSock, sockets )
	{
		if ( !mSock->player() )
			continue;

		// The Socket has to be either in range of Source or Target
		if ( mSock->player()->inRange( this, mSock->player()->visualRange() ) || ( mSock->player()->pos().distance( target ) <= mSock->player()->visualRange() ) )
			mSock->send( &effect );
	}
}

/*!
	Displays an effect moving with this object.
*/
void cUObject::effect( quint16 id, quint8 speed, quint8 duration, quint16 hue, quint16 renderMode )
{
	cUOTxEffect effect;
	effect.setType( ET_STAYSOURCESER );
	effect.setSource( serial_ );
	effect.setSourcePos( pos_ );
	effect.setTargetPos( pos_ );
	effect.setId( id );
	effect.setSpeed( speed );
	effect.setDuration( duration );
	effect.setFixedDirection( true );
	effect.setHue( hue );
	effect.setRenderMode( renderMode );

	cUOSocket* mSock = 0;
	QList<cUOSocket*> sockets = Network::instance()->sockets();
	foreach ( mSock, sockets )
	{
		if ( mSock->player() && mSock->player()->inRange( this, mSock->player()->visualRange() ) )
			mSock->send( &effect );
	}
}

// Simple setting and getting of properties for scripts and the set command.
stError* cUObject::setProperty( const QString& name, const cVariant& value )
{
	changed( TOOLTIP );
	changed_ = true;
	// \rproperty object.serial This integer property contains the serial for this object.
	if ( name == "serial" )
	{
		if ( !value.canCast( cVariant::IntType ) )
		{
			PROPERTY_ERROR( -3, QString( "Invalid integer value: '%1'" ).arg( value.toString() ) );
		}
		setSerial( value.toInt() );
		return 0;
	}

	// \property object.free This boolean property indicates that the object has been freed and is awaiting deletion.
	else
		SET_BOOL_PROPERTY( "free", free )
		// \property object.name This string property contains the name of the object.
	else
		SET_STR_PROPERTY( "name", this->name_ )
		// \property object.pos This property is a <object id="coord">coord</object> object (Python) or a string representation (Show/Set) of the objects position.
	else if ( name == "pos" )
	{
		Coord pos;
		if ( !parseCoordinates( value.toString(), pos ) )
			PROPERTY_ERROR( -3, QString( "Invalid coordinate value: '%1'" ).arg( value.toString() ) )
			moveTo( pos );
		return 0;
	}

	// \property object.eventlist This string property contains a comma separated list of the names of the scripts that are assigned to this object.
	else if ( name == "scriptlist" )
	{
		clearScripts();
		QStringList list = value.toString().split( "," );
		for ( QStringList::const_iterator it( list.begin() ); it != list.end(); ++it )
		{
			cPythonScript* script = ScriptManager::instance()->find( ( *it ).toLatin1() );
			if ( script )
				addScript( script );
			else
				PROPERTY_ERROR( -3, QString( "Script not found: '%1'" ).arg( *it ) )
		}
		return 0;
	}

	return cPythonScriptable::setProperty( name, value );
}

PyObject* cUObject::getProperty( const QString& name, uint hash )
{
	/*
	#define OUTPUT_HASH(x, y) QString("case 0x%2: // %1\n\treturn createPyObject( "#y" );\n").arg(x).arg( elfHash( x ), 0, 16)
	Console::instance()->send(
	OUTPUT_HASH( "bindmenu", bindmenu() ) +
	OUTPUT_HASH( "spawnregion", spawnregion_ ? spawnregion_->id() : QString() ) +
	OUTPUT_HASH( "serial", serial_ ) +
	OUTPUT_HASH( "free", free ? 1 : 0 ) +
	OUTPUT_HASH( "name", this->name() ) +
	OUTPUT_HASH( "pos", pos() ) +
	OUTPUT_HASH( "scriptlist", scriptList() ) +
	OUTPUT_HASH( "multi", multi_ ) );
	#undef OUTPUT_HASH
	*/

	if ( !hash )
		hash = elfHash( name.toLatin1() );

	switch ( hash )
	{
	case 0x4b3ec5: // bindmenu
		/*
		\rproperty object.bindmenu This string property contains a comma separated list of context menu ids for this object.
		This property is inherited by the baseid property of this object.
		*/
		return createPyObject( bindmenu() );
	case 0x5f1b8be: // spawnregion
		/*
		\rproperty object.spawnregion The name of the spawnregion this object was spawned in. This is an empty string
		if the object wasn't spawned or was removed from the spawnregion.
		*/
		return createPyObject( spawnregion_ ? spawnregion_->id() : QString() );
	case 0x79c8f7c: // serial
		return createPyObject( serial_ );
	case 0x6d8b5: // free
		return createPyObject( free ? 1 : 0 );
	case 0x74835: // name
		return createPyObject( this->name() );
	case 0x7763: // pos
		return createPyObject( pos() );
	case 0x7eca34: // scriptlist
		return createPyObject( scriptList() );
	case 0x74c3a9: // multi
		// \rproperty object.multi This item property contains the multi this object is contained in.
		return createPyObject( multi_ );
	default:
		return cPythonScriptable::getProperty( name, hash );
	}
}

void cUObject::sendTooltip( cUOSocket* mSock )
{
	if ( tooltip_ == 0xFFFFFFFF )
	{
		tooltip_ = World::instance()->getUnusedTooltip();
		setTooltip( tooltip_ );
	}

	if (Config::instance()->newTooltipPackets()) {
		cUOTxNewAttachTooltip tooltip;
		tooltip.setId( tooltip_ );
		tooltip.setSerial( serial() );
		mSock->addTooltip( tooltip_ );
		mSock->send( &tooltip );
	} else {
		cUOTxAttachTooltip tooltip;
		tooltip.setId( tooltip_ );
		tooltip.setSerial( serial() );
		mSock->addTooltip( tooltip_ );
		mSock->send( &tooltip );
	}
}

void cUObject::changed( uint state )
{
	//	if( state & SAVE ) changed_ = true;
	if ( state & TOOLTIP )
		tooltip_ = 0xFFFFFFFF;
}

/*!
	Returns the direction from this object to another \s d object
*/
unsigned char cUObject::direction( cUObject* d )
{
	int dir = -1;
	int xdif = d->pos().x - this->pos().x;
	int ydif = d->pos().y - this->pos().y;

	if ( ( xdif == 0 ) && ( ydif < 0 ) )
		dir = 0;
	else if ( ( xdif > 0 ) && ( ydif < 0 ) )
		dir = 1;
	else if ( ( xdif > 0 ) && ( ydif == 0 ) )
		dir = 2;
	else if ( ( xdif > 0 ) && ( ydif > 0 ) )
		dir = 3;
	else if ( ( xdif == 0 ) && ( ydif > 0 ) )
		dir = 4;
	else if ( ( xdif < 0 ) && ( ydif > 0 ) )
		dir = 5;
	else if ( ( xdif < 0 ) && ( ydif == 0 ) )
		dir = 6;
	else if ( ( xdif < 0 ) && ( ydif < 0 ) )
		dir = 7;
	else
		dir = 0;

	return dir;
}

void cUObject::setSpawnregion( cSpawnRegion* spawnregion )
{
	if ( spawnregion_ && spawnregion_ != spawnregion )
	{
		spawnregion_->remove( this );
	}

	spawnregion_ = spawnregion;

	if ( spawnregion )
	{
		spawnregion->add( this );
	}
}

const cVariant& cUObject::getTag( const QString& key ) const
{
	return tags_.get( key );
}

bool cUObject::hasTag( const QString& key ) const
{
	//	changed_ = true;
	return tags_.has( key );
}

void cUObject::setTag( const QString& key, const cVariant& value )
{
	tags_.set( key, value );
	changed_ = true;
}

void cUObject::removeTag( const QString& key )
{
	changed_ = true;
	tags_.remove( key );
}

QStringList cUObject::getTags() const
{
	return tags_.getKeys();
}

void cUObject::resendTooltip()
{
	// Either Attach or Refresh the Data
	if ( tooltip_ == 0xFFFFFFFF )
	{
		tooltip_ = World::instance()->getUnusedTooltip();

		if (Config::instance()->newTooltipPackets()) {
			cUOTxNewAttachTooltip tooltip;
			tooltip.setId( tooltip_ );
			tooltip.setSerial( serial() );

			QList<cUOSocket*> sockets = Network::instance()->sockets();
			foreach ( cUOSocket* s, sockets )
			{
				if ( s->player() && s->player()->inRange( this, s->player()->visualRange() ) )
				{
					s->addTooltip( tooltip_ );
					s->send( &tooltip );
				}
			}
		} else {
			cUOTxAttachTooltip tooltip;
			tooltip.setId( tooltip_ );
			tooltip.setSerial( serial() );

			QList<cUOSocket*> sockets = Network::instance()->sockets();
			foreach ( cUOSocket* s, sockets )
			{
				if ( s->player() && s->player()->inRange( this, s->player()->visualRange() ) )
				{
					s->addTooltip( tooltip_ );
					s->send( &tooltip );
				}
			}
		}
	}
	else
	{
		cUOTxTooltipList tooltip;

		QList<cUOSocket*> sockets = Network::instance()->sockets();
		foreach ( cUOSocket* s, sockets )
		{
			if ( s->player() && s->player()->inRange( this, s->player()->visualRange() ) )
			{
				createTooltip( tooltip, s->player() );
				s->send( &tooltip );
			}
		}
	}
}

/****************************
 *
 * Scripting events
 *
 ****************************/
bool cUObject::onCreate( const QString& definition )
{
	bool result = false;
	if ( canHandleEvent( EVENT_CREATE ) )
	{
		PyObject* args = Py_BuildValue( "(Ns)", getPyObject(), definition.toLatin1().data() );
		result = callEventHandler( EVENT_CREATE, args );
		Py_DECREF( args );
	}
	return result;
}

bool cUObject::onShowTooltip( P_PLAYER sender, cUOTxTooltipList* tooltip )
{
	bool result = false;
	if ( canHandleEvent( EVENT_SHOWTOOLTIP ) )
	{
		PyObject* args = Py_BuildValue( "(NNN)", PyGetCharObject( sender ), getPyObject(), PyGetTooltipObject( tooltip ) );
		result = callEventHandler( EVENT_SHOWTOOLTIP, args );
		Py_DECREF( args );
	}
	return result;
}

void cUObject::createTooltip( cUOTxTooltipList& tooltip, cPlayer* /*player*/ )
{
	if ( tooltip.size() != 19 )
	{
		tooltip.resize( 19 ); // Resize to the original size
	}

	tooltip.setSerial( serial_ );
	tooltip.setId( tooltip_ );
}

void cUObject::remove()
{
	setSpawnregion( 0 ); // Remove from a spawnregion if applicable

	if ( multi_ )
	{
		multi_->removeObject( this );
		multi_ = 0;
	}

	// Remove from the sectors if we previously were on the map
	if ( !pos_.isInternalMap() )
	{
		MapObjects::instance()->remove( this );
		pos_.setInternalMap();
	}

	// Queue up for deletion from worldfile
	World::instance()->deleteObject( this );
}

void cUObject::freezeScriptChain()
{
	if ( isScriptChainFrozen() || !scriptChain )
	{
		return;
	}

	size_t count = reinterpret_cast<size_t>( scriptChain[0] );
	for ( size_t i = 1; i <= count; ++i )
	{
		QByteArray* name = new QByteArray( scriptChain[i]->name() );
		scriptChain[i] = reinterpret_cast<cPythonScript*>( name );
	}
	scriptChain[0] = reinterpret_cast<cPythonScript*>( count | 0x80000000 );
}

void cUObject::unfreezeScriptChain()
{
	if ( !isScriptChainFrozen() || !scriptChain )
	{
		return;
	}

	size_t count = reinterpret_cast<size_t>( scriptChain[0] ) & ~0x80000000;
	size_t pos = 1;

	scriptChain[0] = 0;
	for ( size_t i = 1; i <= count; ++i )
	{
		QByteArray* name = reinterpret_cast<QByteArray*>( scriptChain[i] );
		cPythonScript* script = ScriptManager::instance()->find( *name );
		if ( script )
		{
			scriptChain[0] = reinterpret_cast<cPythonScript*>( pos );
			scriptChain[pos++] = script;
		}
		delete name;
	}

	if ( scriptChain && cPythonScript::canChainHandleEvent( EVENT_ATTACH, scriptChain ) )
	{
		PyObject* args = Py_BuildValue( "(N)", getPyObject() );
		cPythonScript::callChainedEventHandler( EVENT_ATTACH, scriptChain, args );
		Py_DECREF( args );
	}
}

bool cUObject::isScriptChainFrozen()
{
	if ( !scriptChain )
	{
		return false;
	}

	size_t count = reinterpret_cast<size_t>( scriptChain[0] );
	return ( count & 0x80000000 ) != 0;
}

QByteArray cUObject::scriptList() const
{
	if ( !scriptChain )
	{
		return QByteArray();
	}

	QByteArray result;
	size_t count = reinterpret_cast<size_t>( scriptChain[0] );
	for ( size_t i = 1; i <= count; ++i )
	{
		if ( i != count )
		{
			result.append( scriptChain[i]->name() );
			result.append( "," );
		}
		else
		{
			result.append( scriptChain[i]->name() );
		}
	}

	return result;
}

void cUObject::setScriptList( const QByteArray& eventlist )
{
	if ( isScriptChainFrozen() )
	{
		return;
	}

	QList<QByteArray> events = eventlist.split( ',' );
	size_t i = 1;

	clearScripts();
	scriptChain = new cPythonScript * [1 + events.count()];
	scriptChain[0] = reinterpret_cast<cPythonScript*>( 0 );

	foreach ( QByteArray it, events )
	{
		cPythonScript* script = ScriptManager::instance()->find( it );

		if ( script )
		{
			scriptChain[0] = reinterpret_cast<cPythonScript*>( i );
			scriptChain[i++] = script;
		}
	}

	if ( scriptChain && cPythonScript::canChainHandleEvent( EVENT_ATTACH, scriptChain ) )
	{
		PyObject* args = Py_BuildValue( "(N)", getPyObject() );
		cPythonScript::callChainedEventHandler( EVENT_ATTACH, scriptChain, args );
		Py_DECREF( args );
	}
}

void cUObject::save( cBufferedWriter& writer )
{
	writer.setObjectCount( writer.objectCount() + 1 );
	writer.writeByte( getClassid() );

	//unsigned int length = writer.position();
	save( writer, writer.version() );
	//length = writer.position() - length;
	writer.setSkipSize( getClassid(), 0 ); // This is no longer used and will be removed.

	// Save the spawnregion association
	if ( spawnregion_ )
	{
		writer.writeByte( 0xFA );
		writer.writeUtf8( spawnregion_->id() );
		writer.writeInt( serial_ );
	}

	// Save tags for this object
	tags_.save( serial_, writer );
}
