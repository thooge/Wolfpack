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

#if !defined(__NETWORK_H__)
#define __NETWORK_H__

// Library Includes
#include <QObject>
#include <QListIterator>

// Wolfpack Includes
#include "../platform.h"
#include "../singleton.h"
#include "../typedefs.h"
#include "../objectdef.h"
#include "uosocket.h"

//Forward class Declaration
class QHostAddress;

class cNetwork : public QObject, public cComponent
{
	Q_OBJECT
	class cNetworkPrivate;
	cNetworkPrivate* d;
public:
	cNetwork();
	~cNetwork();

	void load();
	void unload();
	void reload();

	bool CheckForBlockedIP( const QHostAddress& ip_address );

	void lock();
	void unlock();
	quint32 count();
	QListIterator<cUOSocket*> getIterator();
	QList<cUOSocket*> sockets() const;

	void broadcast( const QString& message, quint16 color = 0x84d, quint16 font = 0 );

private slots:
	void incomingLoginServerConnection();
	void incomingGameServerConnection();
	void partingLoginServerConnection();
	void partingGameServerConnection();
};

typedef Singleton<cNetwork> Network;

#endif
