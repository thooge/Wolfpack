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

/*
 * Parts of cGameEncryption are based on work done in injection
 * injection.sf.net
 */

#include <QString>
#include <QDataStream>
#include <QCryptographicHash>

#include "encryption.h"
#include "../definitions.h"
#include "../console.h"
#include "../log.h"
#include "../basics.h"

/*!
	Initializes this "LoginCrypt" object.
	Seed is the DWORD sent by the client in the beginning of each connection.
	Buffer/Length should be the first 62 bytes received by the client.
*/
bool cLoginEncryption::init( unsigned int seed, const char* buffer, unsigned int length )
{
	if ( length < 62 )
		return false;

	// Try to find a valid key
	char packet[62];

	// Initialize our tables (cache them, they will be modified)
	unsigned int orgTable1 = ( ( ( ~seed ) ^ 0x00001357 ) << 16 ) | ( ( seed ^ 0xffffaaaa ) & 0x0000ffff );
	unsigned int orgTable2 = ( ( seed ^ 0x43210000 ) >> 16 ) | ( ( ( ~seed ) ^ 0xabcdffff ) & 0xffff0000 );

	for ( unsigned int i = 0; i < KeyManager::instance()->size(); ++i )
	{
		stLoginKey* key = KeyManager::instance()->key( i );

		if ( !key )
			break;

		// Check if this key works on this packet
		memcpy( packet, buffer, 62 );
		table1 = orgTable1;
		table2 = orgTable2;
		key1 = key->key1;
		key2 = key->key2;

		clientDecrypt( &packet[0], 62 );

		// Check if it decrypted correctly
		if ( packet[0] == '\x80' && packet[30] == '\x00' && packet[60] == '\x00' )
		{
			// Reestablish our current state
			table1 = orgTable1;
			table2 = orgTable2;
			key1 = key->key1;
			key2 = key->key2;
			return true;
		}
	}

	return false;
}

/*!
	Encrypts the given buffer for sending it to the client.
	The encryption method used is "LoginCrypt".
*/
void cLoginEncryption::serverEncrypt( char* buffer, unsigned int length )
{
	Q_UNUSED( buffer );
	Q_UNUSED( length );
	return; // No Server->Client Encryption done sever-side
}

/*!
	Decrypts the given buffer received from the client.
	The decryption method used is "LoginCrypt".
*/
void cLoginEncryption::clientDecrypt( char* buffer, unsigned int length )
{
	register quint32 eax, ecx, edx, esi;
	for ( quint32 i = 0; i < length; ++i )
	{
		buffer[i] = buffer[i] ^ ( quint8 ) ( table1 & 0xFF );
		edx = table2;
		esi = table1 << 31;
		eax = table2 >> 1;
		eax |= esi;
		eax ^= key1 - 1;
		edx <<= 31;
		eax >>= 1;
		ecx = table1 >> 1;
		eax |= esi;
		ecx |= edx;
		eax ^= key1;
		ecx ^= key2;
		table1 = ecx;
		table2 = eax;
	}
}

/*!
	Initializes the gameserver encryption using the given seed
*/
void cGameEncryption::init( unsigned int seed )
{
	// The seed is actually little endian and needs to be swapped
	seed = ((seed >> 24) & 0xFF) | ((seed >> 8) & 0xFF00) | ((seed << 8) & 0xFF0000) | ((seed << 24) & 0xFF000000);

	makeKey( &ki, DIR_DECRYPT, 0x80, NULL );
	cipherInit( &ci, MODE_ECB, NULL );

	ki.key32[0] = ki.key32[1] = ki.key32[2] = ki.key32[3] = seed;
	reKey( &ki );

	for ( unsigned int i = 0; i < 256; ++i )
		cipherTable[i] = i;

	sendPos = 0x00;
	recvPos = 0;

	// We need to fill the table initially to calculate the MD5 hash of it
	unsigned char tmpBuffer[0x100];
	blockEncrypt( &ci, &ki, &cipherTable[0], 0x800, &tmpBuffer[0] );
	memcpy( &cipherTable[0], &tmpBuffer[0], 0x100 );

	// Create a MD5 hash of the twofish crypt data and use it as a 16-byte xor table
	// for encrypting the server->client stream.
	memcpy( xorData, QCryptographicHash::hash( QByteArray( (char*)tmpBuffer, 0x100 ), QCryptographicHash::Md5 ).data(), 16 );
}

/*!
	Decrypts a single byte sent by the client.
*/
void cGameEncryption::decryptByte( unsigned char& byte )
{
	// Recalculate table
	if ( recvPos >= 0x100 )
	{
		unsigned char tmpBuffer[0x100];
		blockEncrypt( &ci, &ki, &cipherTable[0], 0x800, &tmpBuffer[0] );
		memcpy( &cipherTable[0], &tmpBuffer[0], 0x100 );
		recvPos = 0;
	}

	// Simple XOR operation
	byte ^= cipherTable[recvPos++];
}

/*!
	Decrypts a buffer sent by the client.
	Algorithm used is a slightly modified Twofish2 algorithm.
*/
void cGameEncryption::clientDecrypt( char* buffer, unsigned int length )
{
	for ( unsigned int i = 0; i < length; ++i )
		decryptByte( ( unsigned char & ) buffer[i] );
}

/*!
	Encrypts a buffer before sending it to the client.
	Encryption used is a table-based XOR encryption.
*/
void cGameEncryption::serverEncrypt( char* buffer, unsigned int length )
{
	for ( unsigned int i = 0; i < length; ++i ) {
		buffer[i] ^= xorData[sendPos++];
		sendPos &= 0x0F; // Maximum Value is 0xF = 15, then 0xF + 1 = 0 again
	}
}

/*!
	KeyManager constructor
*/
cKeyManager::cKeyManager()
{
	load();
}

/*!
	Load the encryption keys.
*/
void cKeyManager::load()
{
	keys.clear();

	QStringList list = Definitions::instance()->getList( "ENCRYPTION" );

	QStringList::const_iterator it;
	for ( it = list.begin(); it != list.end(); ++it )
	{
		QStringList elements = (*it).split( ";" );

		if ( elements.size() < 3 )
		{
			Log::instance()->print( LOG_WARNING, QString( "Invalid encryption key: %1" ).arg( *it ) );
			continue;
		}

		stLoginKey key;

		bool ok;

		key.key1 = hex2dec( elements[1].trimmed() ).toUInt( &ok );

		if ( !ok )
		{
			Log::instance()->print( LOG_WARNING, QString( "Couldn't parse key value: %1" ).arg( elements[1].trimmed() ) );
			continue;
		}

		key.key2 = hex2dec( elements[2].trimmed() ).toUInt( &ok );

		if ( !ok )
		{
			Log::instance()->print( LOG_WARNING, QString( "Couldn't parse key value: %1" ).arg( elements[2].trimmed() ) );
			continue;
		}

		keys.push_back( key );
	}
}
