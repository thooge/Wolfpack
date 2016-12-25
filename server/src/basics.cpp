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

// Wolfpack Includes
#include "coord.h"
#include "basics.h"
#include "inlines.h"
#include "exceptions.h"
#include "persistentobject.h"

// Library Includes
#include <QString>
#include <QStringList>
#include <QFile>
#include <QByteArray>

#include <math.h>
#include <stdlib.h>
#include <cstdio>
#include <algorithm>

#if defined( Q_OS_WIN32 )
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "mersennetwister.h"

static MTRand mtrand;

/*!
	Returns a random number between \a nLowNum
	and \a nHighNum.
*/
int RandomNum( int nLowNum, int nHighNum )
{
	if ( nLowNum > nHighNum )
	{
		std::swap( nLowNum, nHighNum );
	}
	int diff = ( nHighNum - nLowNum ) + 1;
	return nLowNum + ( mtrand.randInt() % diff );
}

/*!
	Returns a random number between \a nLowNum
	and \a nHighNum.
*/
float RandomFloatNum( float nLowNum, float nHighNum )
{
	float diff = nHighNum - nLowNum;
	return nLowNum + mtrand.rand() * diff;
}

/*!
  Returns a random number according to the supplied pattern
  \a dicePattern. The pattern is similar to the ones found
  in RPG books of Dungeons & Dragons and others. It consists
  of xdy + z, which means roll a dice with y faces x times adding
  the result. After the x rolls, add z to the result.
*/
int rollDice( const QString& dicePattern ) // roll dices d&d style
{
	// dicePattern looks like "xdy+z"
	// which equals RandomNum(x,y)+z

	int doffset = dicePattern.indexOf( "d" ), poffset = dicePattern.indexOf( "+" );
	int x = dicePattern.left( doffset ).toInt();
	int z = dicePattern.right( dicePattern.length() - 1 - poffset ).toInt();
	int y = dicePattern.mid( doffset + 1, poffset - doffset - 1 ).toInt();

	return RandomNum( x, x * y ) + z;
}

bool parseCoordinates( const QString& input, Coord& coord, bool ignoreZ )
{
	QStringList coords = input.split( "," );

	// We at least need x, y, z
	if ( coords.size() < ( ignoreZ ? 2 : 3 ) )
		return false;

	bool ok = false;

	quint16 x = coords[0].toULong( &ok );
	if ( !ok )
		return false;

	quint16 y = coords[1].toULong( &ok );
	if ( !ok )
		return false;

	qint8 z = 0;
	if ( !ignoreZ )
	{
		z = coords[2].toShort( &ok );
		if ( !ok )
			return false;
	}

	quint8 map = coord.map; // Current by default
	if ( coords.size() > 3 )
	{
		map = coords[3].toUShort( &ok );

		if ( !ok )
			return false;
	}

	// They are 100% valid now, so let's move!
	// TODO: Add Map bounds check here
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.map = map;

	return true;
}

// global
QString hex2dec( const QString& value )
{
	bool ok;
	if ( ( value.left( 2 ) == "0x" || value.left( 2 ) == "0X" ) )
		return QString::number( value.right( value.length() - 2 ).toUInt( &ok, 16 ) );
	else
		return value;
}

#if defined( Q_OS_WIN32 )

// Windows Version
// Return time in ms since system startup
static unsigned int getPlatformTime()
{
	return GetTickCount();
}

#else

// Linux Version
// Return time in ms since system startup
static unsigned int getPlatformTime()
{
	timeval tTime;

	// Error handling wouldn't have much sense here.
	gettimeofday( &tTime, NULL );

	return ( tTime.tv_sec * 1000 ) + ( unsigned int ) ( tTime.tv_usec / 1000 );
}

#endif

unsigned int getNormalizedTime()
{
	static unsigned int startTime = 0;

	if ( !startTime )
	{
		startTime = getPlatformTime();
		return 0;
	}

	return getPlatformTime() - startTime;
}

uint elfHash(const char * name)
{
	const uchar *k;
	uint h = 0;
	uint g;

	if (name) {
		k = (const uchar *) name;
		while (*k) {
			h = (h << 4) + *k++;
			if ((g = (h & 0xf0000000)) != 0)
				h ^= g >> 24;
			h &= ~g;
		}
	}
	if (!h)
		h = 1;
	return h;
}

cBufferedWriter::cBufferedWriter( const QByteArray& magic, unsigned int version )
{
	d = new cBufferedWriterPrivate;
	d->version = version;
	d->magic = magic;
	d->buffer = new char[WRITER_BUFFERSIZE];
	d->bufferpos = 0;
	d->lastStringId = 0;
	d->objectCount = 0;
	d->dictionary.insert( QByteArray(), 0 ); // Empty String
}

cBufferedWriter::~cBufferedWriter()
{
	close();
	delete d->buffer;
	delete d;
}

void cBufferedWriter::setObjectCount( unsigned int count )
{
	d->objectCount = count;
}

unsigned int cBufferedWriter::objectCount()
{
	return d->objectCount;
}

void cBufferedWriter::open( const QString& filename )
{
	close();

	d->file.setFileName( filename );
	if ( !d->file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		throw wpException( QString( "Couldn't open file %1 for writing." ).arg( filename ) );
	}

	// Reserve space for magic, filesize, version, dictionary offset, object count (in that order)
	unsigned int headerSize = d->magic.length() + 1 + sizeof( unsigned int ) * 4;

	QByteArray header( headerSize, 0 );
	d->file.write( header );

	// Start writing the object type list
	const QMap<unsigned char, QString> &typemap = BinaryTypemap::instance()->getTypemap();
	QMap<unsigned char, QString>::const_iterator it;

	d->skipmap.clear();
	writeByte( typemap.size() );
	for ( it = typemap.begin(); it != typemap.end(); ++it )
	{
		writeByte( it.key() );
		writeInt( 0 ); // SkipSize
		writeAscii( it.value().toLatin1() ); // Pre-insert into the dictionary
		d->skipmap.insert( it.key(), 0 );
		d->typemap.insert( it.key(), it.value() );
	}
}

void cBufferedWriter::close()
{
	if ( d->file.isOpen() )
	{
		unsigned int dictionary = position();

		// Flush the string dictionary at the end of the save
		writeInt( d->dictionary.count() );

		QMap<QByteArray, unsigned int>::iterator it;
		for ( it = d->dictionary.begin(); it != d->dictionary.end(); ++it )
		{
			writeInt( it.value() );
			writeInt( it.key().length() + 1 ); // Counted Strings
			if ( it.key().data() == 0 )
			{
				writeByte( 0 );
			}
			else
			{
				writeRaw( it.key().data(), it.key().length() + 1 );
			}
		}

		flush();

		// Seek to the beginning and write the file header
		d->file.seek( 0 );
		writeRawUnbuffered( d->magic.data(), d->magic.length() + 1 );
		writeIntUnbuffered( d->file.size() );
		writeIntUnbuffered( d->version );
		writeIntUnbuffered( dictionary );
		writeIntUnbuffered( d->objectCount );

		// Write new object type table
		QMap<unsigned char, QString>::const_iterator tit;

		writeByteUnbuffered( d->typemap.size() );
		for ( tit = d->typemap.begin(); tit != d->typemap.end(); ++tit )
		{
			writeByteUnbuffered( tit.key() );

			unsigned int size = 0;
			if ( d->skipmap.contains( tit.key() ) )
			{
				size = d->skipmap[tit.key()];
			}

			writeIntUnbuffered( size ); // SkipSize

			QByteArray type = tit.value().toLatin1();
			writeIntUnbuffered( d->dictionary[type] );
		}

		d->file.close();
	}
}

void cBufferedWriter::flush()
{
	if ( d->bufferpos != 0 )
	{
		d->file.write( d->buffer, d->bufferpos );
		d->file.flush();
		d->bufferpos = 0;
	}
}

unsigned int cBufferedWriter::position()
{
	return d->file.size() + d->bufferpos;
}

unsigned int cBufferedWriter::version()
{
	return d->version;
}

void cBufferedWriter::setSkipSize( unsigned char type, unsigned int skipsize )
{
	d->skipmap.insert( type, skipsize );
}

class cBufferedReaderPrivate
{
public:
	QFile file;
	unsigned int version;
	QByteArray magic;
	QByteArray buffer;
	unsigned int bufferpos;
	unsigned int buffersize;
	QMap<unsigned char, QByteArray> typemap;
	QMap<unsigned char, unsigned int> sizemap;
	QMap<unsigned int, QByteArray> dictionary;
	unsigned int objectCount;
};

cBufferedReader::cBufferedReader( const QByteArray& magic, unsigned int version )
{
	error_ = QString::null;
	d = new cBufferedReaderPrivate;
	d->buffer.resize( 4096 );
	d->bufferpos = 0;
	d->buffersize = 0; // Current amount of data in buffer
	d->magic = magic;
	d->version = version;
	d->objectCount = 0;
}

cBufferedReader::~cBufferedReader()
{
	close();
	delete d;
}

unsigned int cBufferedReader::version()
{
	return d->version;
}

void cBufferedReader::open( const QString& filename )
{
	close();

	d->file.setFileName( filename );
	if ( !d->file.open( QIODevice::ReadOnly ) )
	{
		throw wpException( QString( "Couldn't open file %1 for reading." ).arg( filename ) );
	}

	// Calculate minimum header size (includes typemap count)
	unsigned int headerSize = d->magic.length() + 1 + sizeof( unsigned int ) * 4 + 1;

	if ( d->file.size() < headerSize )
	{
		throw wpException( QString( "File doesn't have minimum size of %1 byte." ).arg( headerSize ) );
	}

	// Check the file magic
	QByteArray magic = readAscii( true );
	if ( magic != d->magic )
	{
		throw wpException( QString( "File had unexpected magic '%1'. Expected: '%2'." ).arg( QString( magic ) ).arg( QString( d->magic ) ) );
	}

	// Check if the file has been truncated or garbage has been appended
	unsigned int filesize = readInt();
	if ( filesize != d->file.size() )
	{
		throw wpException( QString( "The filesize in the header doesn't match. Expected size is %1 byte." ).arg( filesize ) );
	}

	// Check if the worldsave is newer than we can process
	unsigned int version = readInt();
	if ( version > d->version )
	{
		throw wpException( QString( "The file version exceeds the maximum version: %1." ).arg( version ) );
	}
	d->version = version; // Save file version

	unsigned int dictionary = readInt();

	if ( !dictionary || dictionary + 4 > d->file.size() )
	{
		throw wpException( "Invalid dictionary." );
	}

	d->objectCount = readInt();

	unsigned int dataStart = position();

	// Seek to the dictionary and read it
	d->bufferpos = 0;
	d->buffersize = 0;
	d->file.seek( dictionary );

	unsigned int entries = readInt();
	unsigned int i;
	for ( i = 0; i < entries; ++i )
	{
		unsigned int id = readInt();
		unsigned int size = readInt();
		if ( size <= 1 )
		{
			readByte();
			d->dictionary.insert( id, QByteArray() );
		}
		else
		{
			QByteArray data( size, 0 );
			readRaw( data.data(), size );
			d->dictionary.insert( id, data );
		}
	}

	d->bufferpos = 0;
	d->buffersize = 0;
	d->file.seek( dataStart );

	// Read the object type list
	unsigned char count = readByte();

	for ( i = 0; i < count; ++i )
	{
		unsigned char id = readByte();
		unsigned int size = readInt();
		QByteArray type = readAscii();

		d->typemap.insert( id, type );
		d->sizemap.insert( id, size );
	}
}

void cBufferedReader::close()
{
	if ( d->file.isOpen() )
	{
		d->file.close();
	}
}

unsigned int cBufferedReader::readInt()
{
	unsigned int result;
	readRaw( &result, sizeof( result ) );

#if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
	swapBytes( result );
#endif

	return result;
}

double cBufferedReader::readDouble()
{
	double result;
	readRaw( &result, sizeof( result ) );

#if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
	swapBytes( result );
#endif

	return result;
}

unsigned short cBufferedReader::readShort()
{
	unsigned short result;
	readRaw( &result, sizeof( result ) );

#if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
	swapBytes( result );
#endif

	return result;
}

bool cBufferedReader::readBool()
{
	return readByte() != 0;
}

unsigned char cBufferedReader::readByte()
{
	unsigned char result;
	readRaw( &result, sizeof( result ) );
	return result;
}

QString cBufferedReader::readUtf8()
{
	QByteArray data = readAscii();

	if ( data.length() == 0 || data.data() == 0 || *( data.data() ) == 0 )
	{
		return QString::null;
	}
	else
	{
		return QString::fromUtf8( data );
	}
}

QByteArray cBufferedReader::readAscii( bool nodictionary )
{
	if ( nodictionary )
	{
		unsigned char c;
		QByteArray result;
		do
		{
			c = readByte();
			if ( c != 0 )
			{
				result.insert( result.length(), c );
			}
		}
		while ( c != 0 );
		return result;
	}
	else
	{
		unsigned int id = readInt();

		QMap<unsigned int, QByteArray>::iterator it = d->dictionary.find( id );

		if ( it != d->dictionary.end() )
		{
			return it.value();
		}
		else
		{
			return QByteArray();
		}
	}
}

void cBufferedReader::readRaw( void* data, int size )
{
	unsigned int pos = 0;

	// Repeat this
	do
	{
		unsigned int available = d->buffersize - d->bufferpos;
		unsigned int needed = wpMin<unsigned int>( available, size );

		// Get as much data as possible
		if ( needed != 0 )
		{
			memcpy( ( ( unsigned char * ) data ) + pos, d->buffer.data() + d->bufferpos, needed );
			size -= needed;
			available -= needed;
			pos += needed;
			d->bufferpos += needed;
		}

		// Refill buffer if required
		if ( available == 0 )
		{
			int read = d->file.read( d->buffer.data(), 4096 );

			// We will never be able to satisfy the request
			if ( read != 4096 && read < size )
			{
				throw wpException( QString( "Unexpected end of file while reading file %1." ).arg( d->file.fileName() ) );
			}

			d->bufferpos = 0;
			d->buffersize = read;
		}
	}
	while ( size > 0 );
}

unsigned int cBufferedReader::position()
{
	return ( d->file.pos() - d->buffersize ) + d->bufferpos;
}

const QMap<unsigned char, QByteArray>& cBufferedReader::typemap()
{
	return d->typemap;
}

unsigned int cBufferedReader::getSkipSize( unsigned char type )
{
	if ( !d->sizemap.contains( type ) )
	{
		return 0;
	}
	else
	{
		return d->sizemap[type];
	}
}

unsigned int cBufferedReader::objectCount()
{
	return d->objectCount;
}
