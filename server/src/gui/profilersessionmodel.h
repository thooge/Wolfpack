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


#if !defined ( __PROFILERSESSIONMODEL_H__ )
#define __PROFILERSESSIONMODEL_H__

#include <QAbstractItemModel>
#include <QVariant>
#include "../python/pyprofiler.h"

class TreeItem;

class ProfileSessionModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ProfileSessionModel( const ProfileSession& data, QObject* parent = 0 );
	~ProfileSessionModel();

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column,
		const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
	TreeItem *rootItem;
	void setupModelData(const ProfileSession& data, TreeItem *parent);
};

#endif // __PROFILERSESSIONMODEL_H__
