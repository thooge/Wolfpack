
#include "srvparams.h"

// Library Includes
#include "preferences.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qhostaddress.h"

const char preferencesFileVersion[] = "1.0";

cSrvParams::cSrvParams( const QString& filename, const QString& format, const QString& version )  : Preferences(filename, format, version)
{
	// Load data into binary format
	// If value not found, create key.

	// Account Group
	autoAccountCreate_		= getBool("Accounts",	"Auto Create",		false, true);
	autoAccountReload_		= getNumber("Accounts",	"Auto Reload",		10, true);
	checkCharAge_			= getBool("Accounts",	"Check Delete Age", true, true);
    
	// Game Speed Group
	objectDelay_			= getNumber("Game Speed", "ObjectDelay", 1, true);
	checkItemTime_			= getDouble("Game Speed", "Items Check Time", 1.1, true);
	checkNPCTime_			= getDouble("Game Speed", "NPCs Check Time", 1.0, true);
	checkAITime_			= getDouble("Game Speed", "AI Check Time", 1.2, true);
	niceLevel_				= getNumber("Game Speed", "Nice Level", 2, true);
	skillDelay_			    = getNumber("Game Speed", "SkillDelay", 7, true);
	skillLevel_				= getNumber("Game Speed", "SkillLevel", 3, true);
	bandageDelay_			= getNumber("Game Speed", "BandageDelay", 6, true);
	maxStealthSteps_		= getNumber("Game Speed", "Max Stealth Steps", 10, true);
	runningStamSteps_		= getNumber("Game Speed", "Running Stamina Steps", 15, true);
	hungerRate_				= getNumber("Game Speed", "Hunger Rate", 6000, true);
	hungerDamageRate_		= getNumber("Game Speed", "Hunger Damage Rate", 10, true);
	boatSpeed_              = getDouble("Game Speed", "Boat Speed", 0.750000, true);
    
	// General Group
	skillcap_				= getNumber("General",	"SkillCap",			700, true);
	statcap_				= getNumber("General",	"StatsCap",			300, true);
	commandPrefix_			= getString("General",	"Command Prefix",	"#", true).latin1()[0];
	skillAdvanceModifier_	= getNumber("General",	"Skill Advance Modifier", 1000, true);
	statsAdvanceModifier_	= getNumber("General",	"Stats Advance Modifier", 500, true);
	bgSound_				= getNumber("General",	"BackGround Sound Chance", 2, true);
	stealing_				= getBool("General",	"Stealing Enabled",	true, true);			
	guardsActive_			= getBool("General",	"Guards Enabled",	true, true);
	partMsg_				= getBool("General",	"PartMessage",		true, true);
	joinMsg_				= getBool("General",	"JoinMessage",		true, true);
	saveSpawns_				= getBool("General",	"Save Spawned Regions", true, true);
	stablingFee_			= getDouble("General",	"StablingFee",		0.25, true);
	announceWorldSaves_		= getBool("General",	"Announce WorldSaves", true, true);
	port_                   = getNumber("General",    "Port", 2593, true);
	goldWeight_             = getDouble("General",    "Gold Weight", 0.001000, true);
	playercorpsedecaymultiplier_ = getNumber("General", "Player Corpse Decay Multiplier", 0, true);
	lootdecayswithcorpse_   = getNumber("General",    "Loot Decays With Corpse", 1, true);
	invisTimer_             = getDouble("General",    "InvisTimer", 60, true);
	bandageInCombat_		= getBool("General",	"Bandage In Combat",	true, true);
	gateTimer_              = getDouble("General",    "GateTimer", 30, true);
	inactivityTimeout_		= getNumber("General",  "Inactivity Timeout", 300, true);
	showDeathAnim_		    = getNumber("General",  "Show Death Animation", 1, true);
	poisonTimer_		    = getNumber("General",  "PoisonTimer", 180, true);
	serverLog_		        = getBool("General",	"Server Log", false, true);
	speechLog_		        = getBool("General",	"Speech Log", false, true);
	pvpLog_		            = getBool("General",	"PvP Log", false, true);
	gmLog_		            = getBool("General",	"GM Log", false, true);
	backupSaveRatio_		= getNumber("General",  "Backup Save Ratio", 1, true);
	hungerDamage_			= getNumber("General",  "Hunger Damage", 0, true);
	html_			        = getNumber("General",  "Html", -1, true);
	cutScrollReq_			= getNumber("General",  "Cut Scroll Requirements.", 1, true);
	persecute_              = getNumber("General",  "Persecution", 1, true);
	tamedDisappear_         = getNumber("General",  "Tamed Disappear", 1, true);
	houseInTown_            = getNumber("General",  "House In Town", 0, true);
	shopRestock_            = getNumber("General",  "Shop Restock", 1, true);
	badNpcsRed_             = getNumber("General",  "Bad Npcs Red", 1, true);
	slotAmount_             = getNumber("General",  "Slot Amount", 5, true);

	// Combat
	combatHitMessage_		= getBool("Combat", "Hit Message", true, true );
	maxAbsorbtion_		    = getNumber("Combat", "Max Absorbtion", 20, true );
	maxnohabsorbtion_		= getNumber("Combat", "Max Non Human Absorbtion", 100, true );
	monsters_vs_animals_	= getNumber("Combat", "Monsters vs Animals", 0, true );
	animals_attack_chance_	= getNumber("Combat", "Animals Attack Chance", 15, true );
	animals_guarded_	    = getNumber("Combat", "Animals Guarded", 0, true );
	npcdamage_	            = getNumber("Combat", "Npc Damage", 2, true );
	npc_base_fleeat_	    = getNumber("Combat", "Npc Base Flee At", 20, true );
	npc_base_reattackat_	= getNumber("Combat", "Npc Base Reattack At", 40, true );
	attackstamina_	        = getNumber("Combat", "Attack Stamina", -2, true );
	attack_distance_	    = getNumber("Combat", "Attack Distance", 13, true );

	// Vendor
	sellbyname_	            = getNumber("Vendor", "Sell By Name", 1, true );
	sellmaxitem_	        = getNumber("Vendor", "Sell Max Item", 5, true );
	trade_system_	        = getNumber("Vendor", "Trade System", 0, true );
	rank_system_	        = getNumber("Vendor", "Rank System", 0, true );
	checkBank_	            = getNumber("Vendor", "Check Bank", 2000, true );
	vendorGreet_	        = getNumber("Vendor", "Vendor Greet", 2000, true );

	flush(); // if any key created, save it.
}

std::vector<ServerList_st>& cSrvParams::serverList()
{
	if ( serverList_.empty() ) // Empty? Try to load
	{
		if (!containGroup("LoginServer"))
			setDefaultServerList();
		setGroup("LoginServer");
		bool bKeepLooping = true;
		unsigned int i = 1;
		do
		{
			QString tmp = getString(QString("Shard %1").arg(i++), "").simplifyWhiteSpace();
			bKeepLooping = ( tmp != "" );
			if ( bKeepLooping ) // valid data.
			{
				QStringList strList = QStringList::split("=", tmp);
				if ( strList.size() == 2 )
				{
					ServerList_st server;
					server.sServer = strList[0];
					QStringList strList2 = QStringList::split(",", strList[1].stripWhiteSpace());
					server.sIP = strList2[0];
					server.uiPort = strList2[1].toUShort();
					serverList_.push_back(server);
				}
			}
		} while ( bKeepLooping );
	}
	return serverList_;
}

std::vector<StartLocation_st>& cSrvParams::startLocation()
{
	if ( startLocation_.empty() ) // Empty? Try to load
	{
		if (!containGroup("StartLocation") )
			setDefaultStartLocation();
		setGroup("StartLocation");
		bool bKeepLooping = true;
		unsigned int i = 1;
		do
		{
			QString tmp = getString(QString("Location %1").arg(i++), "").simplifyWhiteSpace();
			bKeepLooping = ( tmp != "" );
			if ( bKeepLooping ) // valid data.
			{
				QStringList strList = QStringList::split("=", tmp);
				if ( strList.size() == 2 )
				{
					StartLocation_st loc;
					loc.name = strList[0];
					QStringList strList2 = QStringList::split(",", strList[1].stripWhiteSpace());
					if ( strList2.size() == 4 )
					{
						loc.pos.x = strList2[0].toUShort();
						loc.pos.y = strList2[1].toUShort();
						loc.pos.z = strList2[2].toUShort();
						loc.pos.map = strList2[3].toUShort();
						startLocation_.push_back(loc);
					}
				}
			}
		} while ( bKeepLooping );
	}
	return startLocation_;
}

void cSrvParams::setDefaultStartLocation()
{
	setString("StartLocation", "Location 1", "Yew=567,978,0,0");
	setString("StartLocation", "Location 2", "Minoc=2477,407,15,0");
	setString("StartLocation", "Location 3", "Britain=1496,1629,10,0");
	setString("StartLocation", "Location 4", "Moonglow=4404,1169,0,0");
	setString("StartLocation", "Location 5", "Trinsic=1844,2745,0,0");
	setString("StartLocation", "Location 6", "Magincia=3738,2223,20,0");
	setString("StartLocation", "Location 7", "Jhelom=1378,3817,0,0");
	setString("StartLocation", "Location 8", "Skara Brae=594,2227,0,0");
	setString("StartLocation", "Location 9", "Vesper=2771,977,0,0");
	flush(); // save
}

void cSrvParams::setDefaultServerList()
{
	setBool("LoginServer", "enabled", true);
	setString("LoginServer", "Shard 1", "Default=127.0.0.1,2593");
	flush(); // save.
}

