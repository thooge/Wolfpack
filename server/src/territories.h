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

#if !defined(__TERRITORIES_H__)
#define __TERRITORIES_H__

#include "baseregion.h"
#include "singleton.h"
#include "objectdef.h"

#include <map>
#include <QMap>

struct good_st
{
	uint sellable;
	uint buyable;
	uint rndmin;
	uint rndmax;
};

class cTerritory : public cBaseRegion
{
public:
	cTerritory( const cElement* Tag, cBaseRegion* parent );
	cTerritory();

	void init( void );

	// Getters
	QString name( void ) const
	{
		return name_;
	}
	QString resores( void ) const
	{
		return resores_;
	}
	QString midilist( void ) const
	{
		return midilist_;
	}
	bool isGuarded( void ) const
	{
		return flags_ & 0x00000001;
	}
	bool isNoMark( void ) const
	{
		return flags_ & 0x00000002;
	}
	bool isNoGate( void ) const
	{
		return flags_ & 0x00000004;
	}
	bool isNoRecallOut( void ) const
	{
		return flags_ & 0x00000008;
	}
	bool isNoRecallIn( void ) const
	{
		return flags_ & 0x00000010;
	}
	bool isRecallShield( void ) const
	{
		return flags_ & 0x00000020;
	}
	bool isNoAgressiveMagic( void ) const
	{
		return flags_ & 0x00000040;
	}
	bool isAntiMagic( void ) const
	{
		return flags_ & 0x00000080;
	}
	bool isValidEscortRegion( void ) const
	{
		return flags_ & 0x00000100;
	}
	bool isCave( void ) const
	{
		return flags_ & 0x00000200;
	}
	bool isNoMusic( void ) const
	{
		return flags_ & 0x00000400;
	}
	bool isNoGuardMessage( void ) const
	{
		return flags_ & 0x00000800;
	}
	bool isNoEnterMessage( void ) const
	{
		return flags_ & 0x00001000;
	}
	bool isNoHousing( void ) const
	{
		return flags_ & 0x00002000;
	}
	bool isNoDecay( void ) const
	{
		return flags_ & 0x00004000;
	}
	bool isInstaLogout( void ) const
	{
		return flags_ & 0x00008000;
	}
	bool isNoTeleport( void ) const
	{
		return flags_ & 0x00010000;
	}
	bool isSafe( void ) const
	{
		return flags_ & 0x00020000;
	}
	bool isNoCriminalCombat( void ) const
	{
		return flags_ & 0x00040000;
	}
	bool isNoKillCount( void ) const
	{
		return flags_ & 0x00080000;
	}
	QString guardOwner( void ) const
	{
		return guardowner_;
	}
	UI08 snowChance( void ) const
	{
		return snowchance_;
	}
	UI08 rainChance( void ) const
	{
		return rainchance_;
	}
	int fixedlight( void ) const
	{
		return fixedlight_;
	}
	int lightmodifier( void ) const
	{
		return lightmodifier_;
	}
	QString firstcoin( void ) const
	{
		return firstcoin_;
	}
	QString secondcoin( void ) const
	{
		return secondcoin_;
	}
	QString thirdcoin( void ) const
	{
		return thirdcoin_;
	}
	int weatherday( void ) const
	{
		return weatherday_;
	}
	int weatherhour( void ) const
	{
		return weatherhour_;
	}
	// Duration things for Weather
	int rainduration( void ) const
	{
		return rainduration_;
	}
	int snowduration( void ) const
	{
		return snowduration_;
	}
	int dryduration( void ) const
	{
		return dryduration_;
	}
	int rainrangeduration( void ) const
	{
		return rainrangeduration_;
	}
	int snowrangeduration( void ) const
	{
		return snowrangeduration_;
	}
	int dryrangeduration( void ) const
	{
		return dryrangeduration_;
	}
	// Intensity
	int minintensity( void ) const
	{
		return minintensity_;
	}
	int maxintensity( void ) const
	{
		return maxintensity_;
	}
	int intensity( void ) const
	{
		return intensity_;
	}
	// Storm Parameters
	int stormchecked( void ) const
	{
		return stormchecked_;
	}
	// Extra Flags
	quint16 extraflags( void ) const
	{
		return extraflags_;
	}

	bool haveTeleporters() const;
	bool findTeleporterSpot( Coord& ) const;

	bool isSnowing() const;
	bool isRaining() const;

	QString getGuardSect( void ) const;

	// Setters
	void setLightModifier( int light );

	void setIsRaining( bool data );
	void setIsSnowing( bool data );

	void setWeatherDay( int wday );
	void setWeatherHour( int whour );

	void setWeatherIntensity( int intensity );
	void setStormChecked( int data );

private:
	// Setters to ease up the flag meanings
	void setGuarded( bool data )
	{
		if ( data )
			flags_ |= 0x00000001;
		else
			flags_ &= ~0x00000001;
	}

	void setNoMark( bool data )
	{
		if ( data )
			flags_ |= 0x00000002;
		else
			flags_ &= ~0x00000002;
	}
	void setNoGate( bool data )
	{
		if ( data )
			flags_ |= 0x00000004;
		else
			flags_ &= ~0x00000004;
	}
	void setNoRecallOut( bool data )
	{
		if ( data )
			flags_ |= 0x00000008;
		else
			flags_ &= ~0x00000008;
	}
	void setNoRecallIn( bool data )
	{
		if ( data )
			flags_ |= 0x00000010;
		else
			flags_ &= ~0x00000010;
	}
	void setRecallShield( bool data )
	{
		if ( data )
			flags_ |= 0x00000020;
		else
			flags_ &= ~0x00000020;
	}
	void setNoAgressiveMagic( bool data )
	{
		if ( data )
			flags_ |= 0x00000040;
		else
			flags_ &= ~0x00000040;
	}
	void setAntiMagic( bool data )
	{
		if ( data )
			flags_ |= 0x00000080;
		else
			flags_ &= ~0x00000080;
	}
	void setValidEscortRegion( bool data )
	{
		if ( data )
			flags_ |= 0x00000100;
		else
			flags_ &= ~0x00000100;
	}
	void setCave( bool data )
	{
		if ( data )
			flags_ |= 0x00000200;
		else
			flags_ &= ~0x00000200;
	}
	void setNoMusic( bool data )
	{
		if ( data )
			flags_ |= 0x00000400;
		else
			flags_ &= ~0x00000400;
	}
	void setNoGuardMessage( bool data )
	{
		if ( data )
			flags_ |= 0x00000800;
		else
			flags_ &= ~0x00000800;
	}
	void setNoEnterMessage( bool data )
	{
		if ( data )
			flags_ |= 0x00001000;
		else
			flags_ &= ~0x00001000;
	}
	void setNoHousing( bool data )
	{
		if ( data )
			flags_ |= 0x00002000;
		else
			flags_ &= ~0x00002000;
	}
	void setNoDecay( bool data )
	{
		if ( data )
			flags_ |= 0x00004000;
		else
			flags_ &= ~0x00004000;
	}
	void setInstaLogout( bool data )
	{
		if ( data )
			flags_ |= 0x00008000;
		else
			flags_ &= ~0x00008000;
	}
	void setNoTeleport( bool data )
	{
		if ( data )
			flags_ |= 0x00010000;
		else
			flags_ &= ~0x00010000;
	}
	void setSafe( bool data )
	{
		if ( data )
			flags_ |= 0x00020000;
		else
			flags_ &= ~0x00020000;
	}
	void setNoCriminalCombat( bool data )
	{
		if ( data )
			flags_ |= 0x00040000;
		else
			flags_ &= ~0x00040000;
	}
	void setNoKillCount( bool data )
	{
		if ( data )
			flags_ |= 0x00080000;
		else
			flags_ &= ~0x00080000;
	}

	virtual void processNode( const cElement* Tag, uint hash = 0 );

private:
	QString midilist_; // midilist to play

	QString resores_; // Resources Ores (Mining)

	QString firstcoin_; // First Coin for this Region
	QString secondcoin_; // Second Coin for this Region
	QString thirdcoin_; // Third Coin for this Region

	quint32 flags_; // flags like guarded, mark allowed, etc. (see getters)
	quint16 extraflags_; // Extra Flags for Regions

	QString guardowner_;
	UI08 snowchance_;
	UI08 rainchance_;

	int fixedlight_;	// The fixed Light Level for this place
	int lightmodifier_; // Modifier for Light Level in this Region

	int weatherday_;	// The Day for Next Weather Update
	int weatherhour_;	// The Hour for Next Weather Update

	int rainduration_;			// Default Rain duration
	int rainrangeduration_;		// Range for Rain duration
	int snowduration_;			// Snow duration
	int snowrangeduration_;		// Range for Snow duration
	int dryduration_;			// Dry duration
	int dryrangeduration_;		// Range for Dry duration

	int minintensity_;			// Default Minimum Intensity for Weather
	int maxintensity_;			// Default Maximum Intensity for Weather
	int intensity_;				// Actual Intensity of Weather for this Region

	int stormchecked_;			// A Internal Parameter to Check Storm Feats

	QStringList guardSections_;

	struct teleporters_st
	{
		Coord source;
		Coord destination;
	};

	QList<teleporters_st> teleporters;
public:
	std::map<uint, good_st> tradesystem_;

	inline void addTeleporter( const Coord& from, const Coord& to )
	{
		teleporters_st t;
		t.source = from;
		t.destination = to;
		teleporters.append( t );
	}

protected:

	bool isRaining_;	// Is Raining
	bool isSnowing_;	// Is Snowing
};

inline bool cTerritory::isRaining() const
{
	return isRaining_;
}

inline bool cTerritory::isSnowing() const
{
	return isSnowing_;
}

inline void cTerritory::setLightModifier( int light )
{
	lightmodifier_ = light;
}

inline void cTerritory::setIsRaining( bool data )
{
	isRaining_ = data;
}

inline void cTerritory::setIsSnowing( bool data )
{
	isSnowing_ = data;
}

inline void cTerritory::setWeatherDay( int wday )
{
	weatherday_ = wday;
}

inline void cTerritory::setWeatherHour( int whour )
{
	weatherhour_ = whour;
}

inline void cTerritory::setWeatherIntensity( int intensity )
{
	intensity_ = intensity;
}

inline void cTerritory::setStormChecked( int data )
{
	stormchecked_ = data;
}

class cTerritories : public cComponent
{
	OBJECTDEF( cTerritories )
private:
	QMap<uint, QList<cTerritory*> > topregions;
public:
	void reload();
	void load();
	void unload();
	void check( P_CHAR pc );

	cTerritory* region( const QString& regName );
	cTerritory* region( UI16 posx, UI16 posy, UI08 map );

	inline cTerritory* region( const Coord& pos )
	{
		return region( pos.x, pos.y, pos.map );
	}
};

typedef Singleton<cTerritories> Territories;

#endif
