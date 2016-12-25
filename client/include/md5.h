/*
 *     Wolfpack Emu (WP)
 * UO Server Emulation Program
 *
 * Copyright 2001-2005 by holders identified in AUTHORS.txt
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

/* This file is (c) 2003 Sebastian Hartte. */

#if !defined( __CMD5_H__ )
#define __CMD5_H__

#include <string.h>
#include <qstring.h>

/*!
  \brief This class implements the Md5 digest context.
*/
class cMd5
{
private:
	unsigned int buffer[4];
	unsigned int bits[2];
	unsigned char input[64];
	bool finalized;

	void update();
public:
	/*!
	\brief The constructor for a new digest context.
	*/
	cMd5();

	/*!
	\brief The destructor for a digest context.
	*/
	~cMd5();

	/*!
	\brief Resets the context to create a new digest.
	*/
	void reset();

	/*!
	\brief Updates the digest with new data.
	\param data The data to update the digest width.
	\param length The length of the data.
	*/
	void update( unsigned char* data, unsigned int length );

	/*!
	\brief Finalize the context to get
	*/
	void finalize();

	/*!
	\brief Retrieve the digest of this context.
	\param digest The 33 byte digest we should write to.
	*/
	void digest( char* digest );

	/*!
	\brief Retrieve the binary digest for this hash.
	\param digest A 16 byte array of unsigned chars that should contain the raw digest.
	*/
	void rawDigest( unsigned char* digest );

	/*!
	\brief This function creates a hash for the given message.
	\param digest The target digest. Has to be at least 33 bytes long.
	\param message A null terminated string containing the message.
	*/
	inline static void fastDigest( char* digest, const char* message )
	{
		cMd5 ctx;
		ctx.update( ( unsigned char * ) message, strlen( message ) );
		ctx.finalize();
		ctx.digest( digest );
	}

	/*!
	\brief This function creates a hash for the given text.
	\param text The text that should be hashed. It's converted to UTF-8 before hashing.
	\returns The hash digest in hex notation.
	*/
	inline static QString fastDigest( const QString& text )
	{
		QByteArray data = text.toUtf8();
		QByteArray result( 33, 0 );
		fastDigest( result.data(), data.data() );
		return QString( result );
	}
};

#endif
