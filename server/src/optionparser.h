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

#if !defined( __WPOPTIONPARSER_H__ )
#define __WPOPTIONPARSER_H__

// System Includes
#include <QString>
#include <QMap>
#include <QVariant>
#include <QStringList>
#include <QCoreApplication>

// Wolfpack Includes
#include "singleton.h"

// Class definitions
class OptionParser
{
	class Option
	{
	public:
		QString dest;
		QString help;
		bool requireArgument;
	};

public:

	enum PlatformSpecific
	{
		Windows,
		Posix
	};

	void addOption( const QString& opt, const QString& dest, const QString& help = QString(), bool hasArguments = false );
	void addOption( const QString& opt, const QString& dest, PlatformSpecific plat, const QString& help = QString(), bool hasArguments = false );
	QVariant value( const QString& variable ) const { return options[variable]; }
	void parse();

private:
	void printUsage();
	QMap< QString, QVariant > options;
	QMap< QString, Option > optionSet;
};

typedef Singleton<OptionParser> CommandLineOptions;

#endif // __WPOPTIONPARSER_H__
