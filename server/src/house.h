//==================================================================================
//
//      Wolfpack Emu (WP)
//	UO Server Emulation Program
//
//	Copyright 1997, 98 by Marcus Rating (Cironian)
//  Copyright 2001 by holders identified in authors.txt
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//	* In addition to that license, if you are running this program or modified
//	* versions of it on a public system you HAVE TO make the complete source of
//	* the version used by you available or provide people with a location to
//	* download it.
//
//
//
//	Wolfpack Homepage: http://wpdev.sf.net/
//========================================================================================

#if !defined(__HOUSE_H__)
#define __HOUSE_H__

#include "wolfpack.h"
#include "items.h"
#include "multis.h"

// Forward Class declaration
class cUOSocket;

class cHouse : public cMulti
{
public:

	unsigned int last_used;

	cHouse() 
	{
		cItem::Init( false );

		contserial = INVALID_SERIAL;
		deedsection_ = (char*)0;
		nokey_ = false;
		charpos_.x = charpos_.y = charpos_.z = 0;
	}
	virtual ~cHouse() {}
	virtual void Serialize( ISerialization &archive );
	virtual QString objectID() const;

	bool onValidPlace( void );
	void build( const QDomElement &Tag, UI16 posx, UI16 posy, SI08 posz, SERIAL senderserial, SERIAL deedserial );
	void remove( void );

	virtual void toDeed( cUOSocket* socket );


	QString	deedSection( void ) { return deedsection_; }
	void	setDeedSection( QString data ) { deedsection_ = data; }

protected:
	virtual void processNode( const QDomElement &Tag );
	void processHouseItemNode( const QDomElement &Tag );

	bool nokey_;
	
	struct posxyz_st
	{
		int x;
		int y;
		int z;
	};

	posxyz_st	charpos_;
};

#endif // __HOUSE_H__
